/**
 * @file    combo_eeprom_isr.c
 * @brief   C5 of the combination matrix:
 *          EEPROM writes through the real HAL unlock sequence while a
 *          fast TIMER2 ISR runs the whole time, all real code, one
 *          firmware, one interrupt state.
 *
 * @details
 *   The bug class this gate hunts: an interrupt taken inside the
 *   EEPROM driver's unlock sequence (EECON2 0x55/0xAA then
 *   EECON1<WR>, DS39632E §7.2) while the sequence is mid-flight. On
 *   real silicon the three writes must land in order in EECON2 and
 *   EECON1; a preempting ISR that disturbs EECON1/EECON2 or leaves
 *   the access wrong aborts the unlock and the write never starts.
 *   The ISR must also keep the timer callback count advancing and
 *   leave PIE1/PIE2 and the master enables intact, so each of those
 *   is a CHECK'd cross-check below.
 *
 *   EEPROM completion is taken through the interrupt path: EEIE is
 *   armed (PIE2 bit 4) and the dispatcher's EEIF branch
 *   (pic18_irq_dispatch.c calls EEPROM_IRQHandler unconditionally on
 *   EEIF) clears the flag and fires the completion callback, so the
 *   EEIF dispatch branch itself runs under the live timer ISR. The
 *   main line waits on the callback's done flag, cleared BEFORE each
 *   write is launched so a stale completion (a spurious
 *   EEPROM_WRITES cycle landing while the firmware is not stalled)
 *   can never be mistaken for the current write's completion.
 *
 *   MPLAB SIM limitations this gate works around (same verified set
 *   as epic-settings' sim gate, 2026-08-09):
 *   - The simulator never completes a CPU-executed data-EEPROM write:
 *     after the firmware sets EECON1<WR>, WR stays set and EEIF never
 *     raises. The runner (scripts/sim-mdb-run.sh, EEPROM_WRITES=32)
 *     halts at each stalled write, clears WR, replays the EECON2
 *     unlock, re-asserts WR, and resumes; the simulator then
 *     completes the write from the firmware's own EEDATA/EEADR and
 *     raises EEIF, which the firmware's own ISR consumes. This
 *     scenario writes 8 bytes per main pass; the sim-target CRT
 *     re-runs main after it returns (`ljmp start`, verified by the
 *     C1 gate), so one gate run completes up to 4 passes within 32
 *     cycles. Keep EEPROM_WRITES >= 8 per expected pass if the
 *     scenario changes.
 *
 *   Hosted on the PIC18F4550, not the 16F877A the matrix row was
 *   drafted for, because MPLAB SIM's PIC16F87XA EEPROM model is inert
 *   (verified 2026-08-09, MPLAB X 6.35): neither a CPU-executed write
 *   nor the runner's debugger completion ever raises EEIF (EECON1
 *   held at WR|WREN and PIR2<EEIF> clear across repeated halt/
 *   complete/resume cycles), and the RD strobe never refreshes EEDATA
 *   (a readback after a write returns the stale last-written byte, so
 *   even read verification cannot observe the array). The runner's
 *   EEPROM_WRITES mechanism is proven on the 18F4550 by the
 *   epic-settings gate, and its default EECON1/EECON2 addresses
 *   (0xFA6/0xFA7) are exactly the PIC18 Access-Bank registers, so
 *   `make mdb-test MODULE=epic-combo-eeprom-isr MCU=18F4550
 *   DEVICE=PIC18F4550 WAIT_MS=5000 EEPROM_WRITES=32` runs the gate
 *   with no environment overrides.
 *
 *   Bounded and self-reporting (the harness contract). The EEIF waits
 *   are unbounded by design: the mdb runner's EEPROM_WRITES cycles
 *   complete each stalled write, and an exhausted cycle budget simply
 *   stalls the firmware (no PASS marker, gate FAILs, never a false
 *   PASS).
 */

#include "core/epic_harness.h"
#include "core/pic18_irq.h"
#include "peripherals/pic18fxx5x_eeprom.h"
#include "peripherals/pic18fxx5x_timer2.h"
#include "target/pic18_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define SIM_ITERATIONS 2000UL

/* Same ISR density as the C1 gate: ~1008 instruction cycles per
 * overflow. At 48 MHz the instruction clock is 12 MHz; TMR2 with
 * prescaler 1:16 and PR2=62 counts 16 x 63 = 1008 cycles, ~84 us. */
#define T2_PRESCALER TIMER2_PRESCALER_1_16
#define T2_PERIOD    62u

/* EEPROM region written each pass: 8 bytes at 0x20..0x27 (the 4550
 * has 256 bytes; POR is 0xFF). The pattern is a function of the
 * address so every byte is distinct and none equals 0xFF. */
#define EEPROM_BASE  0x20u
#define EEPROM_BYTES 8u

static volatile uint16_t g_t2_count = 0u;
static volatile uint8_t  g_eeprom_done = 0u;
static uint16_t g_fail = 0u;

/**
 * @brief TIMER2 overflow callback: count the overflow.
 */
static void t2_overflow_cb(void)
{
    g_t2_count++;
}

/**
 * @brief EEPROM completion callback fired by EEPROM_IRQHandler from the ISR.
 *
 * The dispatcher's EEIF branch clears the flag, then calls this.
 */
static void eeprom_done_cb(void)
{
    g_eeprom_done = 1u;
}

/**
 * @brief Record a check failure and log its index as two hex digits.
 */
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

/**
 * @brief Start one EEPROM write and block on the ISR-mediated EEIF completion.
 *
 * The done flag is cleared before the write is launched so a stale
 * completion (a spurious EEPROM_WRITES cycle landing while the
 * firmware is not stalled) can never be mistaken for the current
 * write's completion.
 */
static void eeprom_write_byte(uint8_t addr, uint8_t data)
{
    g_eeprom_done = 0u;
    if (EPIC_EEPROM_WriteByte(addr, data) != EPIC_OK) {
        fail(0x09);
    }
    while (g_eeprom_done == 0u) {
        epic_harness_tick();
    }
}

/**
 * @brief Log a 16-bit value as four hex digits over the harness UART.
 */
static void tx_hex4(uint16_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    c[0] = hx[(v >> 12) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[(v >> 8) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[(v >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[v & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
}

/**
 * @brief Run the EEPROM + TIMER2 interrupt-interleave gate (C5).
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* EEPROM completion via the interrupt path: arms EEIE (PIE2 bit
     * 4). The master enables are still off here, so the armed source
     * stays quiet until the per-pass Restore below. */
    (void)EPIC_EEPROM_Init(eeprom_done_cb);

    /* C1 discipline, per pass (main re-runs after returning): stop
     * the timer and clear its pending flag BEFORE the master-enable
     * edge, start the timer after. */
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~PIC_T2CON_TMR2ON;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR2);
    EPIC_IRQ_Restore(1);

    /* TIMER2 with the overflow callback: enables TMR2IE. */
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
    t2.Prescaler = T2_PRESCALER;
    t2.Period = T2_PERIOD;
    t2.OverflowCallback = t2_overflow_cb;
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER2_Start(&t2);

    /* (b) Write the pattern under the live ISR. Each write stalls on
     * EEIF until the runner's next EEPROM_WRITES cycle completes it,
     * so the unlock sequence and the ISR genuinely interleave. */
    uint16_t t2_before = (uint16_t)g_t2_count;
    for (uint8_t i = 0u; i < EEPROM_BYTES; i++) {
        uint8_t addr = (uint8_t)(EEPROM_BASE + i);
        epic_harness_log("W");
        eeprom_write_byte(addr, (uint8_t)(0xA5u ^ addr));
        epic_harness_log(".");
    }
    uint16_t t2_during = (uint16_t)((uint16_t)g_t2_count - t2_before);

    /* (c) Read the bytes back and verify they match the written
     * pattern. */
    epic_harness_log("R");
    uint8_t bad = 0u;
    uint8_t first_bad = 0u;
    for (uint8_t i = 0u; i < EEPROM_BYTES; i++) {
        uint8_t addr = (uint8_t)(EEPROM_BASE + i);
        uint8_t got = EPIC_EEPROM_ReadByte(addr);
        if (got != (uint8_t)(0xA5u ^ addr)) {
            if (bad == 0u) { first_bad = got; }
            bad++;
        }
    }

    /* (d) Master enables alive: checked BEFORE the final Disable,
     * while the ISR is still live (a wedged ISR path leaves the
     * enables cleared, the Finding 10.1 class). */
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEH) != 0u, 0x04);
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEL) != 0u, 0x08);

    /* Stop interrupts before the register checks. */
    EPIC_IRQ_Disable();

    /* Cross-checks. */
    uint8_t pie1 = epic_sfr_read8(PIC_REG_PIE1);
    CHECK((pie1 & PIC_PIE1_TMR2IE) != 0u, 0x00);              /* source still enabled */
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR2IE)) == 0u, 0x01);  /* nothing else */
    uint8_t pie2 = epic_sfr_read8(PIC_REG_PIE2);
    CHECK((pie2 & PIC_PIE2_EEIE) != 0u, 0x02);                /* source still enabled */
    CHECK((pie2 & (uint8_t)~(PIC_PIE2_EEIE)) == 0u, 0x03);    /* nothing else */
    CHECK(t2_during >= 1u, 0x05);      /* ISR fired during the writes */
    CHECK(g_t2_count >= 100u, 0x06);   /* timer kept firing all run */
    CHECK(bad == 0u, 0x07);            /* every byte landed */

    /* Headline numbers, logged always: 'T' + t2_during + total. */
    epic_harness_log("C");
    epic_harness_log("T");
    tx_hex4(t2_during);
    tx_hex4((uint16_t)g_t2_count);
    if (bad != 0u) {
        static const char hx[] = "0123456789ABCDEF";
        char c[3];
        epic_harness_log("E");
        c[0] = hx[(first_bad >> 4) & 0xF];
        c[1] = hx[first_bad & 0xF];
        c[2] = '\0';
        epic_harness_log(c);
        c[0] = hx[(bad >> 4) & 0xF];
        c[1] = hx[bad & 0xF];
        c[2] = '\0';
        epic_harness_log(c);
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
