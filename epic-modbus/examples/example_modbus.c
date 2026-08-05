/**
 * @file    example_modbus.c
 * @brief   epic-modbus host smoke test: inject full RTU request frames via
 *          the family sim's UART RX hook, advance simulated time past T3.5,
 *          and assert the exact response bytes on the wire.
 *
 * @details
 *   Host-sim test: RX bytes go in via *_sim_drive_usart_rx, TX bytes are
 *   captured via epic_dispatch_all_irqs + TXREG reads, and
 *   epic_tick_delay_ms advances simulated time past the T3.5 timeout.
 *   CRC-16 expected values use an independently written copy of the
 *   algorithm, so this test can't share a CRC bug with epic_modbus.c.
 */

#include "epic_modbus.h"
#include "core/epic_harness.h"
#include "epic_hal.h"
#include "epic_serial.h"
#include "epic_tick.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_RX(b) pic18_sim_drive_usart_rx((uint8_t)(b))
  #define FOSC_HZ_  48000000UL
#else
  #include "pic16f87xa_sim.h"
  #define SIM_RX(b) pic16f87xa_sim_drive_usart_rx((uint8_t)(b))
  #define FOSC_HZ_  20000000UL
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { epic_harness_log("FAIL: %s\n", m); g_fails++; } } while (0)

/* Independent CRC-16/MODBUS reference (poly 0xA001, init 0xFFFF), written
 * separately from src/epic_modbus.c's copy for genuine cross-checking. */
static uint16_t ref_crc16(const uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFFu;
    for (int i = 0; i < len; i++) {
        crc = (uint16_t)(crc ^ buf[i]);
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static void append_crc(uint8_t *frame, int len)
{
    uint16_t crc = ref_crc16(frame, len);
    frame[len]     = (uint8_t)(crc & 0xFFu);
    frame[len + 1] = (uint8_t)(crc >> 8);
}

static void inject_frame(const uint8_t *frame, int len)
{
    for (int i = 0; i < len; i++) {
        SIM_RX(frame[i]);
    }
}

static int drain_tx(uint8_t *out, int max)
{
    int n = 0;
    while (epic_serial_tx_pending() > 0 && n < max) {
        epic_dispatch_all_irqs();
        out[n++] = EPIC_REG8(PIC_REG_TXREG);
    }
    return n;
}

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

static uint16_t holding_regs[4];

int main(void)
{
    epic_harness_init(1000000UL);

    CHECK(ref_crc16((const uint8_t *)"123456789", 9) == 0x4B37u,
          "ref_crc16 matches the CRC-16/MODBUS catalogue check value");

    epic_tick_init(FOSC_HZ_);

    holding_regs[0] = 0x1234u;
    holding_regs[1] = 0x5678u;
    holding_regs[2] = 0x0000u;
    holding_regs[3] = 0xFFFFu;

    static const epic_modbus_slave_map_t map = {
        .coils               = NULL,
        .num_coils           = 0,
        .discrete_inputs     = NULL,
        .num_discrete_inputs = 0,
        .holding_regs        = holding_regs,
        .num_holding_regs    = 4,
        .input_regs          = NULL,
        .num_input_regs      = 0,
    };
    epic_modbus_slave_init(FOSC_HZ_, BAUD, SLAVE_ADDR, &map);

    uint8_t req[16];
    uint8_t resp[16];
    int     n;

    /* ---- FC03 Read Holding Registers: regs[0..1] = 0x1234, 0x5678 ---- */
    req[0] = SLAVE_ADDR; req[1] = 0x03u;
    req[2] = 0x00u; req[3] = 0x00u;  /* start = 0 */
    req[4] = 0x00u; req[5] = 0x02u;  /* qty   = 2 */
    append_crc(req, 6);
    inject_frame(req, 8);
    epic_modbus_slave_poll();               /* drain RX into the frame buffer */
    epic_tick_delay_ms(10);                  /* advance past T3.5 (5 ms @9600) */
    epic_modbus_slave_poll();               /* silence elapsed -> dispatch    */

    n = drain_tx(resp, sizeof(resp));
    CHECK(n == 9, "FC03 response length == 9");
    CHECK(resp[0] == SLAVE_ADDR && resp[1] == 0x03u && resp[2] == 0x04u &&
          resp[3] == 0x12u && resp[4] == 0x34u && resp[5] == 0x56u && resp[6] == 0x78u,
          "FC03 response payload == addr,fc,bytecount,0x1234,0x5678");
    CHECK(resp[7] == (uint8_t)(ref_crc16(resp, 7) & 0xFFu) &&
          resp[8] == (uint8_t)(ref_crc16(resp, 7) >> 8),
          "FC03 response CRC-16 matches the independent reference");

    /* ---- FC06 Write Single Register: regs[2] = 0xBEEF, echoed back ---- */
    req[0] = SLAVE_ADDR; req[1] = 0x06u;
    req[2] = 0x00u; req[3] = 0x02u;  /* addr  = 2 */
    req[4] = 0xBEu; req[5] = 0xEFu;  /* value = 0xBEEF */
    append_crc(req, 6);
    inject_frame(req, 8);
    epic_modbus_slave_poll();
    epic_tick_delay_ms(10);
    epic_modbus_slave_poll();

    n = drain_tx(resp, sizeof(resp));
    CHECK(holding_regs[2] == 0xBEEFu, "FC06 wrote holding_regs[2]");
    CHECK(n == 8, "FC06 response length == 8");
    CHECK(resp[0] == SLAVE_ADDR && resp[1] == 0x06u && resp[2] == 0x00u &&
          resp[3] == 0x02u && resp[4] == 0xBEu && resp[5] == 0xEFu,
          "FC06 response echoes the request");
    CHECK(resp[6] == (uint8_t)(ref_crc16(resp, 6) & 0xFFu) &&
          resp[7] == (uint8_t)(ref_crc16(resp, 6) >> 8),
          "FC06 response CRC-16 matches the independent reference");

    /* ---- FC03 out of range -> ILLEGAL DATA ADDRESS exception ---- */
    req[0] = SLAVE_ADDR; req[1] = 0x03u;
    req[2] = 0x00u; req[3] = 0x64u;  /* start = 100, past num_holding_regs=4 */
    req[4] = 0x00u; req[5] = 0x01u;  /* qty   = 1 */
    append_crc(req, 6);
    inject_frame(req, 8);
    epic_modbus_slave_poll();
    epic_tick_delay_ms(10);
    epic_modbus_slave_poll();

    n = drain_tx(resp, sizeof(resp));
    CHECK(n == 5, "exception response length == 5");
    CHECK(resp[0] == SLAVE_ADDR && resp[1] == 0x83u && resp[2] == 0x02u,
          "exception response == addr, fc|0x80, ILLEGAL_DATA_ADDRESS");
    CHECK(resp[3] == (uint8_t)(ref_crc16(resp, 3) & 0xFFu) &&
          resp[4] == (uint8_t)(ref_crc16(resp, 3) >> 8),
          "exception response CRC-16 matches the independent reference");

    /* ---- Broadcast (addr 0) write: applied, but never answered ---- */
    req[0] = 0x00u; req[1] = 0x06u;
    req[2] = 0x00u; req[3] = 0x00u;  /* addr  = 0 */
    req[4] = 0xABu; req[5] = 0xCDu;  /* value = 0xABCD */
    append_crc(req, 6);
    inject_frame(req, 8);
    epic_modbus_slave_poll();
    epic_tick_delay_ms(10);
    epic_modbus_slave_poll();

    n = drain_tx(resp, sizeof(resp));
    CHECK(holding_regs[0] == 0xABCDu, "broadcast write still applied");
    CHECK(n == 0, "broadcast request gets no response");

    epic_harness_log("modbus: fails=%d\n", g_fails);
    return epic_harness_report(g_fails == 0);
}
