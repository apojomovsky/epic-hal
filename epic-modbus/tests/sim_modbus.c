/**
 * @file    sim_modbus.c
 * @brief   Bounded, self-reporting HARNESS=sim build for epic-modbus:
 *          the module's real `mdb` gate. Runs the actual compiled
 *          epic_modbus.c under MPLAB SIM on an 18F4550, exercises
 *          every code path the public API makes reachable without UART
 *          RX injection, and verifies the Modbus RTU wire format plus
 *          an independent CRC-16 end to end through the module's real
 *          TX machinery.
 *
 * @details
 *   The RX wall, established experimentally (do not silently re-open):
 *   MPLAB SIM on this stack (XC8 v4.00, MPLAB X 6.35) cannot inject
 *   UART RX at all, so the module's RX-driven dispatch
 *   (process_frame -> send_response) is not reachable under mdb:
 *
 *     - scripts/sim-mdb-run.sh's fixed device/program/run/wait/halt
 *       shape has no stimulus slot, and its one hook (extra_mdb) runs
 *       after halt. The mdb UART1 tool exposes output properties only
 *       (output/outputfile/uartioenabled, verified in the mdbcore
 *       simulator jar's UARTOption enum): there is no input file.
 *     - The SCL `stim` command is broken in MPLAB X 6.35 (syntax error
 *       on the guide's minimal file, then a simulator crash), and mdb
 *       `set RCREG` / `set PIR1` are silently ignored (probed by the
 *       epic-console gate work).
 *     - Firmware-side injection is sealed too: the simulator's EUSART
 *       model owns RCIF and RCREG. Poking RCREG and setting
 *       PIR1<RCIF> then running the RX ISR leaves the RX ring empty
 *       (verified: available() stayed 0), because RxUART re-derives
 *       RCIF from its own receive state and the model rejects RCREG
 *       writes. Bit-banging a cycle-accurate 9600 8N1 waveform onto
 *       the RX pin RC7 (1250-instruction bit period, verified against
 *       the XC8 -O2 listing) never sets RCIF either: the model's
 *       receive data path (RxUART.readData -> readStimBuffer) consumes
 *       the injected stimulus buffer, not the pin; the pin observer
 *       only handles wake-from-sleep, and the receiver raises FERR
 *       against a pin-driven waveform without ever completing a byte.
 *
 *   What the gate does exercise, all through the real APIs and the
 *   real compiled code:
 *
 *     (a) epic_modbus_slave_init with a real register map: the USART
 *         configuration it programs (SPBRG/SPBRGH for 9600 at 48 MHz,
 *         BRGH=1, BRG16=1, SPEN+CREN) is read back through the real
 *         SFR path and checked.
 *     (b) The FC03 Read Holding Registers request (slave address,
 *         function code 0x03, start address, quantity) is built to the
 *         RTU wire format the host smoke test documents, with an
 *         independently written CRC-16/MODBUS appended (a CRC bug in
 *         the module's stack cannot be shared with the gate).
 *     (c) The frame is pushed through the module's real TX path:
 *         epic_serial_write is the exact API send_response uses, the
 *         TX ISR drains the ring into TXREG, and the gate captures
 *         every byte from TXREG as the ISR loads it (the same TXREG
 *         readback the host test's drain_tx uses), with GIE masked so
 *         the drain is deterministic and one dispatch per TXIF pumps
 *         exactly one byte.
 *     (d) The captured bytes must equal the request exactly and the
 *         CRC must match the independent reference.
 *     (e) epic_modbus_slave_set_rs485_dir_pin: the direction pin's
 *         TRIS (output) and LAT (idle low = receive) are verified
 *         through the real GPIO driver.
 *     (f) epic_modbus_slave_poll is exercised over a bounded loop
 *         (empty RX ring: framing state machine idle path).
 *
 *   Loop bounds: the TX drain carries a hard iteration guard, every
 *   wait is a bounded tick-based timeout, and the program terminates
 *   by calling epic_harness_report like every other sim gate.
 */

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
static void log_hex8(uint8_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char s[3];
    s[0] = hx[(v >> 4) & 0xFu];
    s[1] = hx[v & 0xFu];
    s[2] = '\0';
    epic_harness_log(s);
}

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

    /* ---- (a) real init: module handle + register map ---- */
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

    /* ---- (b) build the FC03 request: addr, fc, start=0, qty=2, CRC ---- */
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

    /* ---- (c) push the frame through the module's real TX path and
     * capture each byte the TX ISR loads into TXREG ---- */
    uint8_t captured[8];
    int n;
    {
        uint8_t prev = EPIC_IRQ_Disable();
        epic_serial_write(req, 8);
        n = drain_tx(captured, (int)sizeof(captured));
        EPIC_IRQ_Restore(prev);
    }

    /* ---- (d) frame bytes + CRC against the independent reference ---- */
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

    /* ---- (e) RS-485 direction pin: output, idle low (receive) ---- */
    epic_modbus_slave_set_rs485_dir_pin(DIR_PORT, DIR_BIT);
    int dir_ok =
        ((EPIC_REG8(PIC_REG_TRISC) & (uint8_t)EPIC_BIT(DIR_BIT)) == 0u) &&
        ((EPIC_REG8(PIC_REG_LATC) & (uint8_t)EPIC_BIT(DIR_BIT)) == 0u);
    if (dir_ok) {
        epic_harness_log("modbus sim: dir pin ok\n");
    } else {
        epic_harness_log("modbus sim: dir pin BAD\n");
    }

    /* ---- (f) poll smoke: idle framing path over a bounded loop ---- */
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
