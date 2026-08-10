/**
 * @file    combo_modbus_full.c
 * @brief   C12 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          epic-modbus + epic-serial + epic-tick, the full stack
 *          under a live 1 ms tick ISR, all real code, one firmware,
 *          one interrupt state.
 *
 * @details
 *   The point of this gate is the interleave the single-module gates
 *   cannot see: an FC03 read-holding-registers request is pushed
 *   through the module's real TX path (epic_serial_write into the TX
 *   ring, drained by the real TX ISR loading TXREG) while the 1 ms
 *   epic-tick Timer2 ISR is live the whole time, and the transmitted
 *   frame must come out byte-exact against the modbus gate's oracle
 *   (11 03 00 00 00 02 C6 9B for slave address 0x11) with the tick
 *   still advancing and GIE still set afterwards.
 *
 *   The PIC18 EUSART model under MPLAB SIM differs from the PIC16 one
 *   (probed 2026-08-09 while building this gate):
 *   - TXIF is re-asserted every sim step while TXEN is set
 *     (pic18_sim.c's sim_step_usart), so the TX ISR drains the ring
 *     automatically with GIE live and no manual dispatch is needed.
 *   - The TXREG readback is NOT a stable mirror of the last written
 *     byte: it tracks the uart1io model's last output byte, so a read
 *     must immediately follow the pump with no other UART activity
 *     (harness logging included) in between. The gate therefore
 *     captures each byte with a single read right after its write,
 *     and paces one byte per >= 2 ms (the byte's shift time plus the
 *     tick wait) so the model is idle when the next write lands; this
 *     is the pattern the probing run found stable (8/8 bytes).
 *   - A burst write (the whole frame at once) transmits every byte
 *     but can overflow the model's output FIFO (W9201: data lost), so
 *     the burst phase checks the firmware side only: the ring drains,
 *     the last byte reaches TXREG, and the tick/GIE survive.
 *   - The epic-serial gate documented that the PIC16 TX storm wedges
 *     GIE under the sim; on PIC18F4550 this gate verifies GIE stays
 *     set and the tick keeps advancing through both TX phases (it
 *     does; no wedge observed).
 *
 *   Bounded and self-reporting (the harness contract); no RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
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

/* Only bounds the decorative final drain; every phase loop has its own
 * bounded guard (a guard trip reports FAIL instead of hanging). */
#define SIM_ITERATIONS 2000UL

#define SLAVE_ADDR 0x11u
#define BAUD       9600u

/* Expected USART configuration for 9600 baud at 48 MHz, BRGH=1,
 * BRG16=1: SPBRG:SPBRGH = (48e6 / (4 * 9600)) - 1 = 1249 = 0x04E1. */
#define EXPECT_SPBRG  0xE1u
#define EXPECT_SPBRGH 0x04u

/* The modbus gate's oracle for the FC03 request to slave 0x11:
 * addr, fc=03, start=0x0000, qty=0x0002, CRC-16/MODBUS = 0x9BC6. */
static const uint8_t ORACLE[8] = {
    0x11u, 0x03u, 0x00u, 0x00u, 0x00u, 0x02u, 0xC6u, 0x9Bu
};

/* Bounded-wait guards: with a live 1 ms tick the per-byte wait ends
 * in a few thousand iterations; the guards trip only when the tick
 * has wedged or the ring has stopped draining, so a broken run
 * reports FAIL instead of hanging under mdb. */
#define TICK_WAIT_GUARD   500000UL
#define TX_DRAIN_GUARD    2000000UL

static uint16_t holding_regs[4];
static uint8_t  g_fail = 0u;

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

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

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

/* Bounded wait for the tick to advance at least @p ms milliseconds.
 * Returns 1 on success; 0 if the guard trips (tick wedged, GIE lost). */
static int wait_ticks(uint32_t ms)
{
    uint32_t t0 = epic_tick_get();
    uint32_t guard = 0UL;
    while (epic_tick_elapsed_since(t0) < ms) {
        if (++guard >= TICK_WAIT_GUARD) {
            return 0;
        }
        epic_harness_tick();
    }
    return 1;
}

/* Bounded wait for the TX ring to drain (the ISR popped the byte).
 * With GIE live the vector is delivered automatically; this is only
 * the bounded completion wait. Returns 1 on success. */
static int wait_drained(void)
{
    uint32_t guard = 0UL;
    while (epic_serial_tx_pending() > 0) {
        if (++guard >= TX_DRAIN_GUARD) {
            return 0;
        }
        epic_harness_tick();
    }
    return 1;
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* ---- (a) the tick first: 1 ms Timer2 timebase, GIE on ---- */
    epic_tick_init(FOSC_HZ);

    /* ---- (b) the modbus slave with a real register map. Init goes
     * through epic_serial_init: verify the USART config it programmed
     * through the real SFR path, like the modbus gate. ---- */
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

    epic_modbus_slave_init(FOSC_HZ, BAUD, SLAVE_ADDR, &map);

    int init_ok =
        (EPIC_REG8(PIC_REG_SPBRG) == EXPECT_SPBRG) &&
        (EPIC_REG8(PIC_REG_SPBRGH) == EXPECT_SPBRGH) &&
        ((EPIC_REG8(PIC_REG_TXSTA) & PIC_TXSTA_BRGH) != 0u) &&
        ((EPIC_REG8(PIC_REG_BAUDCON) & PIC_BAUDCON_BRG16) != 0u) &&
        ((EPIC_REG8(PIC_REG_RCSTA) & (PIC_RCSTA_SPEN | PIC_RCSTA_CREN))
         == (PIC_RCSTA_SPEN | PIC_RCSTA_CREN));
    CHECK(init_ok, 0x00);
    if (init_ok) {
        epic_harness_log("combo modbus: init ok\n");
    } else {
        epic_harness_log("combo modbus: init BAD\n");
    }

    /* The oracle's own CRC against the independent reference: the
     * request bytes must really carry CRC 0x9BC6 (else the oracle is
     * wrong, not the firmware). */
    uint16_t oracle_crc = ref_crc16(ORACLE, 6);
    CHECK((uint8_t)(oracle_crc & 0xFFu) == ORACLE[6] &&
          (uint8_t)(oracle_crc >> 8) == ORACLE[7], 0x01);

    uint8_t captured[8];

    /* ---- (c)/(d) phase A: the frame, one byte per tick, through the
     * real TX path with the tick ISR live. Each epic_serial_write
     * enqueues one byte and arms TXIE; the ISR vectors (the PIC18
     * model re-asserts TXIF every step), pops the byte, and loads
     * TXREG. The capture is a single read right after the write, the
     * pattern this sim's TXREG readback supports (see the header);
     * the bounded drain wait proves the ISR emptied the ring. Between
     * bytes the gate waits >= 2 ms so a real tick vector fires with
     * the byte already shifted out, interleaving every transmission
     * with the live timebase and keeping the model idle for the next
     * write. ---- */
    uint32_t ta0 = epic_tick_get();
    int drain_ok = 1;
    int tick_ok = 1;
    for (int k = 0; k < 8; k++) {
        uint8_t b = ORACLE[k];
        (void)epic_serial_write(&b, 1);
        if (!wait_drained()) {
            drain_ok = 0;
        }
        /* The ISR pops the ring and then loads TXREG a few instructions
         * later; the drain wait can exit in that window, so re-read up
         * to a small bound until the byte shows (reads do not touch the
         * UART model). If the readback never shows the byte the frame
         * check below fails honestly. */
        uint8_t rb;
        uint32_t rr;
        for (rr = 0; rr < 64u; rr++) {
            rb = EPIC_REG8(PIC_REG_TXREG);
            if (rb == b) {
                break;
            }
        }
        captured[k] = rb;
        if (!wait_ticks(2u)) {
            tick_ok = 0;
        }
    }
    int frame_ok = 1;
    for (int i = 0; i < 8; i++) {
        if (captured[i] != ORACLE[i]) {
            frame_ok = 0;
        }
    }
    uint16_t cap_crc = ref_crc16(captured, 6);   /* CRC covers the 6 PDU bytes */
    int crc_ok = (captured[6] == (uint8_t)(cap_crc & 0xFFu) &&
                  captured[7] == (uint8_t)(cap_crc >> 8));
    uint32_t phaseA_ms = epic_tick_get() - ta0;

    epic_harness_log("combo modbus: phaseA tx:");
    log_hex_buf(captured, 8);
    CHECK(frame_ok, 0x02);                     /* byte-exact vs the oracle */
    CHECK(crc_ok, 0x0D);                       /* CRC still valid end to end */
    CHECK(drain_ok, 0x06);                     /* ISR drained every byte */
    CHECK(tick_ok, 0x03);                      /* tick never stalled */
    CHECK(phaseA_ms >= 16u, 0x04);             /* tick fired per byte */
    if (frame_ok && crc_ok) {
        epic_harness_log("combo modbus: phaseA frame ok\n");
    } else {
        epic_harness_log("combo modbus: phaseA frame BAD\n");
    }

    /* ---- phase B: the modbus gate's exact push, one write for the
     * whole frame, still with GIE live: the ISR drains the ring on
     * its own. Check the ring emptied and the last byte reached
     * TXREG, and that the tick/GIE survived the burst (the PIC16
     * storm-wedge class this gate hunts on PIC18). ---- */
    {
        uint32_t tb0 = epic_tick_get();
        int w = epic_serial_write(ORACLE, 8);
        int drained = wait_drained();
        uint8_t last = EPIC_REG8(PIC_REG_TXREG);
        CHECK(w == 8, 0x05);                   /* write accepted the frame */
        CHECK(drained, 0x06);                  /* ISR drained the ring */
        CHECK(last == ORACLE[7], 0x07);        /* last byte reached TXREG */
        if (w == 8 && drained && last == ORACLE[7]) {
            epic_harness_log("combo modbus: phaseB tx ok\n");
        } else {
            epic_harness_log("combo modbus: phaseB tx BAD\n");
        }
        (void)tb0;
    }

    /* ---- (e) cross-checks: GIE alive, the tick source still armed,
     * and the tick still advancing after both TX phases. ---- */
    uint8_t intcon_live = EPIC_REG8(PIC_REG_INTCON);
    uint8_t pie1_live = EPIC_REG8(PIC_REG_PIE1);
    CHECK((intcon_live & PIC_INTCON_GIEH) != 0u, 0x08);  /* GIE stayed on */
    CHECK((pie1_live & PIC_PIE1_TMR2IE) != 0u, 0x09);    /* tick source on */
    {
        uint32_t tc0 = epic_tick_get();
        int alive = wait_ticks(2u);
        uint32_t adv = epic_tick_get() - tc0;
        CHECK(alive && adv >= 2u, 0x0A);       /* tick still advancing */
    }
    if ((intcon_live & PIC_INTCON_GIEH) != 0u) {
        epic_harness_log("combo modbus: gie alive\n");
    } else {
        epic_harness_log("combo modbus: gie LOST\n");
    }

    /* ---- poll smoke: the slave's idle framing path (empty RX ring)
     * under the live tick, bounded like the modbus gate. ---- */
    {
        uint32_t t0 = epic_tick_get();
        uint32_t guard = 0UL;
        while (epic_tick_elapsed_since(t0) < 3u && guard < TICK_WAIT_GUARD) {
            epic_modbus_slave_poll();
            epic_harness_tick();
            guard++;
        }
        CHECK(epic_tick_elapsed_since(t0) >= 3u, 0x0B);
    }
    epic_harness_log("combo modbus: poll ok\n");

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
