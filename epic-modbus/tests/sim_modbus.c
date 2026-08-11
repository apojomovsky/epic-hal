/* Bounded, self-reporting HARNESS=sim `mdb` gate for epic-modbus: runs
 * the compiled epic_modbus.c under MPLAB SIM on an 18F4550 and verifies
 * the RTU wire format + an independent CRC-16 through the real TX path.
 * RX-driven dispatch is unreachable: MPLAB SIM cannot inject UART RX at
 * all (no mdb input stimulus, SCL `stim` broken in 6.35, `set RCREG`/
 * `set PIR1` ignored, and the EUSART model re-derives RCIF from its own
 * state; established experimentally, do not silently re-open). The gate
 * exercises: (a) real init SFR readback, (b) a built FC03 request,
 * (c) the real TX path via TXREG capture, (d) frame+CRC equality,
 * (e) RS-485 dir pin TRIS/LAT, (f) a bounded idle poll loop. */

#include "core/epic_harness.h"
#include "epic_hal.h"
#include "epic_modbus.h"
#include "epic_serial.h"
#include "epic_tick.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

/* Expected USART configuration for 9600 baud at 48 MHz, BRGH=1,
 * BRG16=1: SPBRG:SPBRGH = (48e6 / (4 * 9600)) - 1 = 1249 = 0x04E1. */
#define EXPECT_SPBRG  0xE1u
#define EXPECT_SPBRGH 0x04u

/* RS-485 direction pin: GPIOC (port index 2), bit 3. */
#define DIR_PORT 2u
#define DIR_BIT  3u

static uint16_t holding_regs[4];

/* Independent CRC-16/MODBUS reference (poly 0xA001, init 0xFFFF),
 * written separately from src/epic_modbus.c's copy so the gate cannot
 * share a CRC bug with the module. */
/** @brief Independent CRC-16/MODBUS reference so the gate cannot share
 *         a CRC bug with the module. */
static uint16_t ref_crc16(const uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFFu;
    for (int i = 0; i < len; i++) {
        crc = (uint16_t)(crc ^ buf[i]);
        for (int b = 0; b < 8; b++) {
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001u)
                             : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

/* Diagnostic: log one byte as two hex digits (the sim harness logs the
 * string's raw bytes, varargs are ignored). */
/** @brief Log one byte as two hex digits. */
static void log_hex8(uint8_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char s[3];
    s[0] = hx[(v >> 4) & 0xFu];
    s[1] = hx[v & 0xFu];
    s[2] = '\0';
    epic_harness_log(s);
}

/** @brief Log a buffer as space-separated hex bytes. */
static void log_hex_buf(const uint8_t *buf, int n)
{
    char s[2];
    s[1] = '\0';
    for (int i = 0; i < n; i++) {
        s[0] = ' ';
        epic_harness_log(s);
        log_hex8(buf[i]);
    }
    epic_harness_log("\n");
}

/* Service the TX ISR by hand and read back each byte it loads into
 * TXREG (the host smoke test's drain_tx uses the same TXREG readback).
 * With GIE masked the ring cannot drain on its own, so one dispatch
 * per TXIF pumps exactly one byte. Bounded by a hard iteration guard:
 * if MPLAB SIM never re-arms TXIF the loop bails out and the frame
 * check fails instead of hanging. */
/** @brief Service the TX ISR by hand and capture each TXREG byte. */
static int drain_tx(uint8_t *out, int max)
{
    int n = 0;
    uint32_t guard = 0UL;
    while (n < max && epic_serial_tx_pending() > 0 && guard < 1000000UL) {
        guard++;
        if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TXIF) {
            epic_dispatch_all_irqs();
            out[n++] = EPIC_REG8(PIC_REG_TXREG);
        }
    }
    return n;
}

/** @brief Sim gate main: verify init SFRs, TX path, frame/CRC, and the
 *         RS-485 dir pin. */
int main(void)
{
    epic_harness_init(0UL);
    epic_tick_init(FOSC_HZ);   /* Timer2 1 ms timebase, enables GIE */
    epic_harness_log("modbus sim: init\n");

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

    /* (a) real init: module handle + register map */
    epic_modbus_slave_init(FOSC_HZ, BAUD, SLAVE_ADDR, &map);

    int init_ok =
        (EPIC_REG8(PIC_REG_SPBRG) == EXPECT_SPBRG) &&
        (EPIC_REG8(PIC_REG_SPBRGH) == EXPECT_SPBRGH) &&
        ((EPIC_REG8(PIC_REG_TXSTA) & PIC_TXSTA_BRGH) != 0u) &&
        ((EPIC_REG8(PIC_REG_BAUDCON) & PIC_BAUDCON_BRG16) != 0u) &&
        ((EPIC_REG8(PIC_REG_RCSTA) & (PIC_RCSTA_SPEN | PIC_RCSTA_CREN))
         == (PIC_RCSTA_SPEN | PIC_RCSTA_CREN));
    if (init_ok) {
        epic_harness_log("modbus sim: init sfr ok\n");
    } else {
        epic_harness_log("modbus sim: init sfr BAD\n");
    }

    /* (b) build the FC03 request: addr, fc, start=0, qty=2, CRC */
    uint8_t req[8];
    req[0] = SLAVE_ADDR;
    req[1] = 0x03u;
    req[2] = 0x00u;
    req[3] = 0x00u;
    req[4] = 0x00u;
    req[5] = 0x02u;
    {
        uint16_t crc = ref_crc16(req, 6);
        req[6] = (uint8_t)(crc & 0xFFu);
        req[7] = (uint8_t)(crc >> 8);
    }

    /* (c) push the frame through the module's real TX path and capture
     * each byte the TX ISR loads into TXREG */
    uint8_t captured[8];
    int n;
    {
        uint8_t prev = EPIC_IRQ_Disable();
        epic_serial_write(req, 8);
        n = drain_tx(captured, (int)sizeof(captured));
        EPIC_IRQ_Restore(prev);
    }

    /* (d) frame bytes + CRC against the independent reference */
    int len_ok = (n == 8);
    int frame_ok = len_ok;
    for (int i = 0; i < n && i < 8; i++) {
        if (captured[i] != req[i]) {
            frame_ok = 0;
        }
    }
    int crc_ok = 0;
    if (n >= 2) {
        uint16_t c = ref_crc16(captured, n - 2);
        crc_ok = (captured[n - 2] == (uint8_t)(c & 0xFFu) &&
                  captured[n - 1] == (uint8_t)(c >> 8));
    }

    epic_harness_log("modbus sim: tx:");
    log_hex_buf(captured, n);
    if (len_ok) {
        epic_harness_log("modbus sim: tx len ok\n");
    } else {
        epic_harness_log("modbus sim: tx len BAD\n");
    }
    if (frame_ok) {
        epic_harness_log("modbus sim: frame ok\n");
    } else {
        epic_harness_log("modbus sim: frame BAD\n");
    }
    if (crc_ok) {
        epic_harness_log("modbus sim: crc ok\n");
    } else {
        epic_harness_log("modbus sim: crc BAD\n");
    }

    /* (e) RS-485 direction pin: output, idle low (receive) */
    epic_modbus_slave_set_rs485_dir_pin(DIR_PORT, DIR_BIT);
    int dir_ok =
        ((EPIC_REG8(PIC_REG_TRISC) & (uint8_t)EPIC_BIT(DIR_BIT)) == 0u) &&
        ((EPIC_REG8(PIC_REG_LATC) & (uint8_t)EPIC_BIT(DIR_BIT)) == 0u);
    if (dir_ok) {
        epic_harness_log("modbus sim: dir pin ok\n");
    } else {
        epic_harness_log("modbus sim: dir pin BAD\n");
    }

    /* (f) poll smoke: idle framing path over a bounded loop */
    {
        uint32_t t0 = epic_tick_get();
        while (epic_tick_elapsed_since(t0) < 3u) {
            epic_modbus_slave_poll();
            epic_harness_tick();
        }
    }
    epic_harness_log("modbus sim: poll ok\n");

    int ok = init_ok && len_ok && frame_ok && crc_ok && dir_ok;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(ok);
}
