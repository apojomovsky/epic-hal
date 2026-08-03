/**
 * @file    pic8_modbus.c
 * @brief   Modbus RTU slave core: CRC-16, T3.5 silence-delimited framing,
 *          function-code dispatch, optional RS-485 direction control.
 *
 * @details
 *   Single-instance module (one slave per firmware image), state lives
 *   in file-scope statics, not a caller-owned handle, same model
 *   pic8-serial/pic8-tick use. Family-neutral: the only family-specific
 *   surface is GPIO for the optional RS-485 pin, already neutral via
 *   pic8_hal.h's GPIO_TypeDef/HAL_GPIO_* contract, no #if needed here.
 */

#include "pic8_modbus.h"
#include "pic8_hal.h"
#include "pic8_serial.h"
#include "pic8_tick.h"

#include <stdbool.h>

/* ─── Modbus function codes (the "core" set this module implements) ──── */
#define MB_FC_READ_COILS            0x01u
#define MB_FC_READ_DISCRETE_INPUTS  0x02u
#define MB_FC_READ_HOLDING_REGS     0x03u
#define MB_FC_READ_INPUT_REGS       0x04u
#define MB_FC_WRITE_SINGLE_COIL     0x05u
#define MB_FC_WRITE_SINGLE_REG      0x06u
#define MB_FC_WRITE_MULTIPLE_COILS  0x0Fu
#define MB_FC_WRITE_MULTIPLE_REGS   0x10u

#define MB_EXC_ILLEGAL_FUNCTION      0x01u
#define MB_EXC_ILLEGAL_DATA_ADDRESS  0x02u
#define MB_EXC_ILLEGAL_DATA_VALUE    0x03u

/* ─── module state ────────────────────────────────────────────────────── */
static uint8_t  s_frame[PIC8_MODBUS_MAX_ADU];
static uint16_t s_frame_len;
static uint32_t s_last_rx_tick;

static uint8_t                        s_slave_addr;
static const pic8_modbus_slave_map_t *s_map;
static uint32_t                       s_t3_5_ms;

static uint8_t s_dir_port;
static uint8_t s_dir_pin;
static bool    s_dir_configured;

/* ─── CRC-16 (Modbus/ANSI, poly 0xA001, init 0xFFFF), bit-loop ────────── */
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    for (uint16_t i = 0; i < len; i++) {
        crc = (uint16_t)(crc ^ buf[i]);
        for (uint8_t bit = 0; bit < 8u; bit++) {
            if (crc & 0x0001u) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            } else {
                crc = (uint16_t)(crc >> 1);
            }
        }
    }
    return crc;
}

/* ─── T3.5 inter-frame silence timeout, see docs/ARCHITECTURE.md ─────── */
static uint32_t compute_t3_5_ms(uint32_t baud)
{
    if (baud > 19200u) {
        return 2u; /* spec fixes T3.5 = 1.75 ms above 19200 baud; pic8-tick's
                       1 ms resolution rounds that up to 2 ticks. */
    }
    /* ceil(3.5 chars * 11 bits/char * 1000 ms/s / baud); round up so the
     * timeout never falls short of the true silence requirement. */
    return (uint32_t)((38500ul + baud - 1ul) / baud);
}

/* ─── bit-packed coil/discrete-input helpers ──────────────────────────── */
static bool bit_get(const uint8_t *arr, uint16_t idx)
{
    return (bool)((arr[idx >> 3] >> (idx & 7u)) & 1u);
}

static void bit_set(uint8_t *arr, uint16_t idx, bool v)
{
    if (v) {
        arr[idx >> 3] = (uint8_t)(arr[idx >> 3] | (uint8_t)(1u << (idx & 7u)));
    } else {
        arr[idx >> 3] = (uint8_t)(arr[idx >> 3] & (uint8_t)~(1u << (idx & 7u)));
    }
}

static uint16_t be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void put_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

/* ─── response builders. Return the PDU length (addr+fc+payload, before
 * CRC) written into resp[], or 0 for "drop, no response" (only used for a
 * frame whose length doesn't match its own function code, which passing
 * CRC makes vanishingly unlikely, this is defensive, not spec-driven). ─── */

static uint16_t build_exception(uint8_t *resp, uint8_t fc, uint8_t exc_code)
{
    resp[0] = s_slave_addr;
    resp[1] = (uint8_t)(fc | 0x80u);
    resp[2] = exc_code;
    return 3u;
}

static uint16_t handle_read_bits(uint8_t fc, const uint8_t *table, uint16_t table_len,
                                  uint8_t *resp)
{
    if (s_frame_len != 8u) { /* addr+fc+4 payload+crc16 */
        return 0u;
    }
    uint16_t start = be16(&s_frame[2]);
    uint16_t qty   = be16(&s_frame[4]);

    if (qty == 0u || qty > 2000u) {
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_VALUE);
    }
    if (table == NULL || (uint32_t)start + qty > table_len) {
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }
    uint16_t byte_count = (uint16_t)((qty + 7u) / 8u);
    if ((uint32_t)byte_count + 5u > PIC8_MODBUS_MAX_ADU) { /* +5 = addr+fc+bytecount+crc16 */
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_VALUE);
    }

    resp[0] = s_slave_addr;
    resp[1] = fc;
    resp[2] = (uint8_t)byte_count;
    for (uint16_t i = 0; i < byte_count; i++) {
        resp[3 + i] = 0u;
    }
    for (uint16_t i = 0; i < qty; i++) {
        if (bit_get(table, (uint16_t)(start + i))) {
            resp[3 + (i >> 3)] = (uint8_t)(resp[3 + (i >> 3)] | (uint8_t)(1u << (i & 7u)));
        }
    }
    return (uint16_t)(3u + byte_count);
}

static uint16_t handle_read_regs(uint8_t fc, const uint16_t *table, uint16_t table_len,
                                   uint8_t *resp)
{
    if (s_frame_len != 8u) {
        return 0u;
    }
    uint16_t start = be16(&s_frame[2]);
    uint16_t qty   = be16(&s_frame[4]);

    if (qty == 0u || qty > 125u) {
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_VALUE);
    }
    if (table == NULL || (uint32_t)start + qty > table_len) {
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }
    uint16_t byte_count = (uint16_t)(qty * 2u);
    if ((uint32_t)byte_count + 5u > PIC8_MODBUS_MAX_ADU) { /* +5 = addr+fc+bytecount+crc16 */
        return build_exception(resp, fc, MB_EXC_ILLEGAL_DATA_VALUE);
    }

    resp[0] = s_slave_addr;
    resp[1] = fc;
    resp[2] = (uint8_t)byte_count;
    for (uint16_t i = 0; i < qty; i++) {
        put_be16(&resp[3 + 2u * i], table[start + i]);
    }
    return (uint16_t)(3u + byte_count);
}

static uint16_t handle_write_single_coil(uint8_t *resp)
{
    if (s_frame_len != 8u) {
        return 0u;
    }
    uint16_t addr  = be16(&s_frame[2]);
    uint16_t value = be16(&s_frame[4]);

    if (value != 0xFF00u && value != 0x0000u) {
        return build_exception(resp, MB_FC_WRITE_SINGLE_COIL, MB_EXC_ILLEGAL_DATA_VALUE);
    }
    if (s_map->coils == NULL || addr >= s_map->num_coils) {
        return build_exception(resp, MB_FC_WRITE_SINGLE_COIL, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }
    bit_set(s_map->coils, addr, value == 0xFF00u);

    /* success response echoes the request verbatim */
    resp[0] = s_slave_addr;
    resp[1] = MB_FC_WRITE_SINGLE_COIL;
    resp[2] = s_frame[2];
    resp[3] = s_frame[3];
    resp[4] = s_frame[4];
    resp[5] = s_frame[5];
    return 6u;
}

static uint16_t handle_write_single_reg(uint8_t *resp)
{
    if (s_frame_len != 8u) {
        return 0u;
    }
    uint16_t addr  = be16(&s_frame[2]);
    uint16_t value = be16(&s_frame[4]);

    if (s_map->holding_regs == NULL || addr >= s_map->num_holding_regs) {
        return build_exception(resp, MB_FC_WRITE_SINGLE_REG, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }
    s_map->holding_regs[addr] = value;

    resp[0] = s_slave_addr;
    resp[1] = MB_FC_WRITE_SINGLE_REG;
    resp[2] = s_frame[2];
    resp[3] = s_frame[3];
    resp[4] = s_frame[4];
    resp[5] = s_frame[5];
    return 6u;
}

static uint16_t handle_write_multiple_coils(uint8_t *resp)
{
    if (s_frame_len < 9u) { /* addr+fc+start(2)+qty(2)+bytecount(1)+>=1 data+crc(2) */
        return 0u;
    }
    uint16_t start      = be16(&s_frame[2]);
    uint16_t qty        = be16(&s_frame[4]);
    uint8_t  byte_count = s_frame[6];
    const uint8_t *data = &s_frame[7];

    if (qty == 0u || qty > 1968u || byte_count != (uint16_t)((qty + 7u) / 8u)) {
        return build_exception(resp, MB_FC_WRITE_MULTIPLE_COILS, MB_EXC_ILLEGAL_DATA_VALUE);
    }
    if ((uint16_t)(7u + byte_count + 2u) != s_frame_len) {
        return 0u; /* declared byte count doesn't match the received frame length */
    }
    if (s_map->coils == NULL || (uint32_t)start + qty > s_map->num_coils) {
        return build_exception(resp, MB_FC_WRITE_MULTIPLE_COILS, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }

    for (uint16_t i = 0; i < qty; i++) {
        bool v = (bool)((data[i >> 3] >> (i & 7u)) & 1u);
        bit_set(s_map->coils, (uint16_t)(start + i), v);
    }

    resp[0] = s_slave_addr;
    resp[1] = MB_FC_WRITE_MULTIPLE_COILS;
    resp[2] = s_frame[2];
    resp[3] = s_frame[3];
    resp[4] = s_frame[4];
    resp[5] = s_frame[5];
    return 6u;
}

static uint16_t handle_write_multiple_regs(uint8_t *resp)
{
    if (s_frame_len < 9u) {
        return 0u;
    }
    uint16_t start      = be16(&s_frame[2]);
    uint16_t qty        = be16(&s_frame[4]);
    uint8_t  byte_count = s_frame[6];
    const uint8_t *data = &s_frame[7];

    if (qty == 0u || qty > 123u || byte_count != (uint16_t)(qty * 2u)) {
        return build_exception(resp, MB_FC_WRITE_MULTIPLE_REGS, MB_EXC_ILLEGAL_DATA_VALUE);
    }
    if ((uint16_t)(7u + byte_count + 2u) != s_frame_len) {
        return 0u;
    }
    if (s_map->holding_regs == NULL || (uint32_t)start + qty > s_map->num_holding_regs) {
        return build_exception(resp, MB_FC_WRITE_MULTIPLE_REGS, MB_EXC_ILLEGAL_DATA_ADDRESS);
    }

    for (uint16_t i = 0; i < qty; i++) {
        s_map->holding_regs[start + i] = be16(&data[2u * i]);
    }

    resp[0] = s_slave_addr;
    resp[1] = MB_FC_WRITE_MULTIPLE_REGS;
    resp[2] = s_frame[2];
    resp[3] = s_frame[3];
    resp[4] = s_frame[4];
    resp[5] = s_frame[5];
    return 6u;
}

/* ─── RS-485 direction control + transmit ─────────────────────────────── */
static void send_response(const uint8_t *resp, uint16_t len)
{
    if (s_dir_configured) {
        HAL_GPIO_WritePin((GPIO_TypeDef)s_dir_port, (uint16_t)PIC8_BIT(s_dir_pin), GPIO_PIN_SET);
    }
    pic8_serial_write(resp, (int)len);
    if (s_dir_configured) {
        pic8_serial_flush(); /* wait for the ring AND the shift register to drain
                                 before dropping the driver enable */
        HAL_GPIO_WritePin((GPIO_TypeDef)s_dir_port, (uint16_t)PIC8_BIT(s_dir_pin), GPIO_PIN_RESET);
    }
}

/* ─── frame validation + dispatch ─────────────────────────────────────── */
static void process_frame(void)
{
    if (s_frame_len < 4u) {
        return; /* shorter than addr+fc+crc16, can't be a real ADU */
    }

    uint16_t crc_calc = modbus_crc16(s_frame, (uint16_t)(s_frame_len - 2u));
    if (s_frame[s_frame_len - 2u] != (uint8_t)(crc_calc & 0xFFu) ||
        s_frame[s_frame_len - 1u] != (uint8_t)(crc_calc >> 8)) {
        return; /* bad CRC, drop silently like every RTU slave does */
    }

    uint8_t addr      = s_frame[0];
    bool    broadcast = (addr == 0u);
    if (!broadcast && addr != s_slave_addr) {
        return; /* frame is for a different slave */
    }

    uint8_t  fc = s_frame[1];
    uint8_t  resp[PIC8_MODBUS_MAX_ADU];
    uint16_t pdu_len;

    switch (fc) {
    case MB_FC_READ_COILS:
        pdu_len = handle_read_bits(fc, s_map->coils, s_map->num_coils, resp);
        break;
    case MB_FC_READ_DISCRETE_INPUTS:
        pdu_len = handle_read_bits(fc, s_map->discrete_inputs, s_map->num_discrete_inputs, resp);
        break;
    case MB_FC_READ_HOLDING_REGS:
        pdu_len = handle_read_regs(fc, s_map->holding_regs, s_map->num_holding_regs, resp);
        break;
    case MB_FC_READ_INPUT_REGS:
        pdu_len = handle_read_regs(fc, s_map->input_regs, s_map->num_input_regs, resp);
        break;
    case MB_FC_WRITE_SINGLE_COIL:
        pdu_len = handle_write_single_coil(resp);
        break;
    case MB_FC_WRITE_SINGLE_REG:
        pdu_len = handle_write_single_reg(resp);
        break;
    case MB_FC_WRITE_MULTIPLE_COILS:
        pdu_len = handle_write_multiple_coils(resp);
        break;
    case MB_FC_WRITE_MULTIPLE_REGS:
        pdu_len = handle_write_multiple_regs(resp);
        break;
    default:
        pdu_len = build_exception(resp, fc, MB_EXC_ILLEGAL_FUNCTION);
        break;
    }

    if (pdu_len == 0u || broadcast) {
        return; /* malformed request, or a broadcast (processed above, never answered) */
    }

    uint16_t resp_crc = modbus_crc16(resp, pdu_len);
    resp[pdu_len]     = (uint8_t)(resp_crc & 0xFFu);
    resp[pdu_len + 1] = (uint8_t)(resp_crc >> 8);
    send_response(resp, (uint16_t)(pdu_len + 2u));
}

/* ─── public API ───────────────────────────────────────────────────────── */
void pic8_modbus_slave_init(uint32_t fosc_hz, uint32_t baud,
                             uint8_t slave_addr,
                             const pic8_modbus_slave_map_t *map)
{
    pic8_serial_init(fosc_hz, baud);

    s_slave_addr     = slave_addr;
    s_map            = map;
    s_t3_5_ms        = compute_t3_5_ms(baud);
    s_frame_len      = 0u;
    s_dir_configured = false;
}

void pic8_modbus_slave_set_rs485_dir_pin(uint8_t port, uint8_t pin)
{
    s_dir_port       = port;
    s_dir_pin        = pin;
    s_dir_configured = true;

    HAL_GPIO_Init((GPIO_TypeDef)port, (uint16_t)PIC8_BIT(pin), GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin((GPIO_TypeDef)port, (uint16_t)PIC8_BIT(pin), GPIO_PIN_RESET); /* idle = receive */
}

void pic8_modbus_slave_poll(void)
{
    int avail = pic8_serial_available();
    if (avail > 0) {
        uint16_t space = (uint16_t)(PIC8_MODBUS_MAX_ADU - s_frame_len);
        if (space > 0u) {
            int n = pic8_serial_read(&s_frame[s_frame_len], (int)space);
            if (n > 0) {
                s_frame_len = (uint16_t)(s_frame_len + (uint16_t)n);
            }
        }
        if ((uint16_t)avail > space) {
            /* frame already overflowed the ADU buffer: drain and discard the
             * rest so the RX ring never wedges. The oversized frame fails
             * length/CRC validation once silence is detected. */
            uint8_t  scratch[8];
            uint16_t remaining = (uint16_t)((uint16_t)avail - space);
            while (remaining > 0u) {
                int chunk = (remaining < 8u) ? (int)remaining : 8;
                int n     = pic8_serial_read(scratch, chunk);
                if (n <= 0) {
                    break;
                }
                remaining = (uint16_t)(remaining - (uint16_t)n);
            }
        }
        s_last_rx_tick = pic8_tick_get();
    }

    if (s_frame_len > 0u && pic8_tick_elapsed_since(s_last_rx_tick) >= s_t3_5_ms) {
        process_frame();
        s_frame_len = 0u;
    }
}
