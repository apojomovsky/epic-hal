/**
 * @file    combo_multitimer.c
 * @brief   C2 of the combination matrix:
 *          TMR0 + TMR1 + TMR2 + USART, all interrupt-driven, one
 *          firmware, one bank state.
 *
 * @details
 *   Three timers with distinct periods, configured through the three
 *   distinct period mechanisms, running simultaneously:
 *     - TMR0, prescaler 1:256, reload 0:      65536 cycles = 13.107 ms
 *       (the 8-bit counter with the shared prescaler; the counter
 *       rollover is the TMR0 "period").
 *     - TMR1, prescaler 1:1, 16-bit reload 0x8000 re-armed by the
 *       overflow callback: 32768 cycles = 6.554 ms steady state
 *       (the driver writes ReloadValue once at Start; a free-running
 *       16-bit timer wraps to 0x0000 on overflow, so without the
 *       callback re-arm the steady-state period would be a full
 *       65536 cycles. The callback re-arm is the classic periodic
 *       TMR1 pattern, same as epic-swuart).
 *     - TMR2, prescaler 1:16, PR2 = 62:       1008 cycles = 0.202 ms
 *       (the 200 us "tick"; TMR2 resets to 0 on the PR2 match in
 *       hardware, so the period is exact).
 *   TMR1's steady period is exactly half of TMR0's, so in a healthy
 *   run n1 == 2*n0 within a couple of counts; TMR2 runs at
 *   65536/1008 = 65.016x TMR0, so n2 == 65*n0 plus a phase term
 *   bounded by 67 + (n0 >> 6) (16-cycle remainder per TMR0 period)
 *   and at most one missed final wrap (+65). Those ratios are the
 *   cross-timer corruption check: a dropped TMR1 dispatch (the
 *   dispatch's flag-gate reads TMR1IE banked; if that read
 *   misdirects, TMR1IF is dropped without the handler running)
 *   freezes n1 while n0 and n2 keep advancing; a misdirected
 *   callback increments the wrong counter; a corrupted OPTION_REG
 *   prescaler changes TMR0's rate. Each breaks a ratio.
 *
 *   The USART runs polled (no-op TX callback, TXIE off): every timer
 *   ISR therefore exercises the dispatch's TXIE gate (TXIF is pending
 *   whenever TXREG is empty, so without the gate each ISR would
 *   dispatch the TX handler through XC8's PC-relative
 *   function-pointer table, the epic-tick wedge class). The loop also
 *   transmits a fixed byte every 256 iterations under the live ISR
 *   load; the marker line's byte-exact arrival in the mdb UART
 *   capture is the TX integrity check.
 *
 *   MPLAB SIM behaviors pinned (probed 2026-08-09 by this gate's own
 *   diagnostic builds; the host simulator pic16f87xa_sim.c models all
 *   of these correctly, so they are simulator gaps, not firmware
 *   bugs):
 *   - TMR0IF is not a reliable event source in MPLAB SIM when the
 *     prescaler is assigned to TMR0 (PSA=0): a TMR0-only probe
 *     observed zero latches across thousands of rollover samples,
 *     and under this gate's own three-timer load the flag latches
 *     only as a rare transient (logged as T0I) that never
 *     deterministically drives the ISR (T0C stays 0). The counter
 *     itself runs at the correct prescaled rate (verified by wrap
 *     counting). With PSA=1 (1:1) the flag does latch, but the
 *     resulting ~50 us interrupt rate starves the CPU and the run
 *     wedges. The gate therefore checks TMR0 through its hardware
 *     counter (wrap count, floors, and the two rate ratios) and
 *     through the TMR0IE/PEIE/TMR0IF register image; the TMR0
 *     overflow callback path is exercised by the real-target smoke
 *     build of this same source.
 *   - Enabling GIE while a timer interrupt is already pending wedges
 *     the sim's ISR path (count freezes, GIE left cleared, the
 *     Finding 10.1 class; C1's lesson, re-applied per pass). Per
 *     pass: stop TMR1/TMR2 and clear all three timer flags BEFORE
 *     EPIC_IRQ_Restore(1); the timers start after GIE is on. (TMR0's
 *     stop is folded into EPIC_TIMER0_Init's bank-safe
 *     option_clr_set; EPIC_TIMER0_Stop itself is a plain OPTION_REG
 *     RMW that misdirects under XC8 v4.00, see
 *     pic16f87xa-hal/tests/sim_bank_probe.c.)
 *   - TXIF is not cleared by the sim on a TXREG write (the driver
 *     clears it in software); polled TX uses the TRMT wait, which
 *     the sim does model (the harness's own log path proves it).
 *
 *   GIE aliveness is checked from the loop, not by a single INTCON
 *   sample after the loop: the hardware clears GIE for the duration
 *   of every ISR (~30% duty at the TMR2 rate), so one sample races
 *   the ISR. Instead every loop iteration samples INTCON and the
 *   gate fails if GIE was never observed set (the wedge leaves GIE
 *   permanently clear, so every sample reads 0).
 *
 *   Bounded and self-reporting (the harness contract); no RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_usart.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_ITERATIONS 4000UL

/* TMR0: 1:256 prescaler, full 8-bit rollover (reload 0) -> 65536
 * cycles = 13.107 ms at the 5 MHz instruction clock. */
#define T0_PRESCALER TIMER0_PRESCALER_1_256
#define T0_RELOAD    0x00u

/* TMR1: 16-bit reload 0x8000 re-armed every overflow by the callback
 * -> 32768 cycles = 6.554 ms steady state, exactly half of TMR0's
 * period (the 2:1 cross-check). */
#define T1_RELOAD    0x8000u

/* TMR2: 1:16 prescaler, PR2 = 62 -> 1008 cycles = 201.6 us (the
 * tick; same period as C1's proven TMR2 setup). */
#define T2_PRESCALER TIMER2_PRESCALER_1_16
#define T2_PERIOD    62u

/* Advancement floors, well below the counts a single bounded pass
 * produces (the gate logs the final counts before the marker; main
 * re-runs from reset per pass and the counters restart at 0, so the
 * floors are per-pass). */
#define T0_WRAP_FLOOR 8u
#define T1_FLOOR      20u
#define T2_FLOOR      700u

static volatile uint16_t g_t0_count = 0u;
static volatile uint16_t g_t1_count = 0u;
static volatile uint16_t g_t2_count = 0u;
static uint16_t g_fail = 0u;

/**
 * @brief TIMER0 overflow callback: count the overflow.
 */
static void t0_overflow_cb(void)
{
    g_t0_count++;
}

/**
 * @brief TIMER1 overflow callback: re-arm the reload and count the overflow.
 *
 * Re-arm the 16-bit reload: the timer free-runs to 0x0000 after
 * an overflow, so without this write the steady-state period
 * would be 65536 cycles, not 32768 (see the header). Writing
 * TMR1H then TMR1L also clears the TMR1 prescaler (none here).
 */
static void t1_overflow_cb(void)
{
    EPIC_TIMER1_WriteCounter(T1_RELOAD);
    g_t1_count++;
}

/**
 * @brief TIMER2 overflow callback: count the overflow.
 */
static void t2_overflow_cb(void)
{
    g_t2_count++;
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
 * @brief No-op USART TX-complete callback (transmission is polled).
 */
static void s_tx_noop(void)
{
}

/**
 * @brief 16-bit counter read with torn-read protection.
 *
 * g_t1_count and g_t2_count are incremented by the ISR (a 2-byte
 * sequence), so a main-line read can straddle an increment. A retry
 * converges: the ISR's next increment is at least a full period away.
 */
static uint16_t stable_read16(const volatile uint16_t *p)
{
    uint16_t a = *p;
    uint16_t b = *p;
    if (a == b) return a;
    return *p;
}

/**
 * @brief Log a 16-bit value as four hex digits over the harness UART.
 */
static void log_hex16(uint16_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    c[0] = hx[(v >> 12) & 0xFu];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[(v >> 8) & 0xFu];
    epic_harness_log(c);
    c[0] = hx[(v >> 4) & 0xFu];
    epic_harness_log(c);
    c[0] = hx[v & 0xFu];
    epic_harness_log(c);
}

/**
 * @brief Run the TMR0 + TMR1 + TMR2 + USART interrupt gate (C2).
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* USART: re-init with the no-op callback handle (arms TXEN) and
     * turn the TX interrupt source off (transmission is polled). */
    static USART_HandleTypeDef s_usart;
    s_usart = (USART_HandleTypeDef)USART_HANDLE_DEFAULT;
    s_usart.SPBRG = (uint8_t)USART_ComputeSPBRG(
        FOSC_HZ, 9600UL, USART_MODE_ASYNCHRONOUS, USART_BRGH_HIGH);
    s_usart.TxCpltCallback = s_tx_noop;
    (void)EPIC_USART_Init(&s_usart);
    EPIC_IRQ_DisableSrc(PIC16_IRQ_USART_TX);

    /* Per-pass discipline (C1 lesson): stop TMR1/TMR2 and clear all
     * three timer flags BEFORE the GIE-on edge, then start the timers
     * after GIE is on, so no flag can be pending when GIE rises. */
    EPIC_REG8(PIC_REG_T1CON) &= (uint8_t)~PIC_T1CON_TMR1ON;
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~PIC_T2CON_TMR2ON;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);
    EPIC_IRQ_Restore(1);

    /* TIMER0 with the overflow callback: enables TMR0IE (INTCON). */
    TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
    t0.Prescaler = T0_PRESCALER;
    t0.ReloadValue = T0_RELOAD;
    t0.OverflowCallback = t0_overflow_cb;
    (void)EPIC_TIMER0_Init(&t0);

    /* TIMER1 with the 16-bit reload and the overflow callback:
     * enables TMR1IE (PIE1). */
    TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
    t1.ReloadValue = T1_RELOAD;
    t1.OverflowCallback = t1_overflow_cb;
    (void)EPIC_TIMER1_Init(&t1);

    /* TIMER2 with the overflow callback: enables TMR2IE (PIE1). */
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
    t2.Prescaler = T2_PRESCALER;
    t2.Period = T2_PERIOD;
    t2.OverflowCallback = t2_overflow_cb;
    (void)EPIC_TIMER2_Init(&t2);

    (void)EPIC_TIMER0_Start(&t0);
    (void)EPIC_TIMER1_Start(&t1);
    (void)EPIC_TIMER2_Start(&t2);

    uint16_t prev1 = 0u;
    uint16_t prev2 = 0u;
    uint16_t mono_bad = 0u;
    uint16_t gie_off_seen = 0u;
    uint16_t t0_if_seen = 0u;
    uint16_t t0_wraps = 0u;
    uint8_t t0_prev = EPIC_TIMER0_ReadCounter();
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        /* Polled TX exercise under the live ISR load. */
        if ((i & 0xFFu) == 0u) {
            while (!EPIC_USART_IsTxShiftRegisterEmpty()) {
                /* wait for the shift register to drain */
            }
            EPIC_USART_Transmit(0x55u);
        }

        /* TMR0 hardware rollover detection: the counter increments
         * every 256 cycles (1:256 prescaler), the loop samples it
         * every pass, and every rollover 0xFF -> 0x00 is caught
         * exactly once (the counter advances at most 2 per sample).
         * This is the TMR0 liveness check under MPLAB SIM, which
         * never sets TMR0IF when the prescaler is assigned (see the
         * header). */
        {
            uint8_t t0_now = EPIC_TIMER0_ReadCounter();
            if (t0_now < t0_prev) t0_wraps++;
            t0_prev = t0_now;
        }

        /* Snapshot the two ISR counters. Monotonicity: neither may
         * ever decrease (each changes only in its own callback, so
         * any decrease is corruption or a torn read; the retry below
         * disambiguates). */
        uint16_t t1c = stable_read16(&g_t1_count);
        uint16_t t2c = stable_read16(&g_t2_count);
        if (t1c < prev1 || t2c < prev2) {
            /* Re-read once: the snapshot may have straddled an ISR
             * increment. A genuine decrease survives the retry. */
            t1c = stable_read16(&g_t1_count);
            t2c = stable_read16(&g_t2_count);
            if (t1c < prev1 || t2c < prev2) mono_bad++;
        }
        prev1 = t1c;
        prev2 = t2c;

        /* GIE and TMR0IF samples: with a healthy GIE at least one
         * sample over the whole pass observes it set. TMR0IF is
         * logged as T0I for evidence but not checked: under MPLAB
         * SIM it latches only as a rare transient (see the header),
         * and on hardware the ISR clears it within a few cycles of
         * the rollover. */
        {
            uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
            if ((intcon & PIC_INTCON_GIE) == 0u) gie_off_seen++;
            if (intcon & PIC_INTCON_TMR0IF) t0_if_seen++;
        }
    }

    /* Freeze the callback counters before the checks (the timers keep
     * running, but with GIE off no callback can advance a counter;
     * TMR0's hardware counter keeps rolling, so the wrap count below
     * is closed out with one final read). */
    EPIC_IRQ_Disable();
    {
        uint8_t t0_now = EPIC_TIMER0_ReadCounter();
        if (t0_now < t0_prev) t0_wraps++;
        t0_prev = t0_now;
    }

    /* GIE alive: the wedge leaves GIE permanently clear, so every
     * loop sample read 0. (An ISR in flight legitimately clears GIE
     * for its duration, hence the whole-pass evidence, not a single
     * sample.) */
    CHECK(gie_off_seen < SIM_ITERATIONS, 0x00);

    /* Source-enable bits. TMR0IE and PEIE live in INTCON and are
     * race-free to read (the ISR entry only clears GIE); TMR1IE and
     * TMR2IE live in the banked PIE1. */
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    CHECK((intcon & PIC_INTCON_TMR0IE) != 0u, 0x01);
    CHECK((intcon & PIC_INTCON_PEIE) != 0u, 0x02);
    CHECK((intcon & PIC_INTCON_TMR0IF) == 0u, 0x0B);

    uint8_t pie1 = 0u;
    EPIC_BANK1_READ8(PIE1, pie1);
    CHECK((pie1 & (PIC_PIE1_TMR1IE | PIC_PIE1_TMR2IE)) ==
          (PIC_PIE1_TMR1IE | PIC_PIE1_TMR2IE), 0x03);
    /* Nothing else enabled, in particular TXIE must be off (polled
     * mode) and RCIE/SSPIE/etc. untouched. */
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR1IE | PIC_PIE1_TMR2IE)) == 0u,
          0x04);

    uint16_t t0w = t0_wraps;
    uint16_t t1c = g_t1_count;
    uint16_t t2c = g_t2_count;

    /* Each source advanced: TMR0 through its hardware counter (the
     * prescaled rollover rate; its callback is not checkable under
     * MPLAB SIM, see below), TMR1 and TMR2 through their callback
     * counters. */
    CHECK(t0w >= T0_WRAP_FLOOR, 0x05);
    CHECK(t1c >= T1_FLOOR, 0x06);
    CHECK(t2c >= T2_FLOOR, 0x07);

    /* Cross-timer consistency. TMR1's steady period is exactly half
     * of TMR0's: n1 == 2*n0 within the start skew, the callback
     * re-arm latency (a few hundred cycles per 32768-cycle period)
     * and at most one missed final wrap. TMR2's period is
     * 65536/1008 = 65.016x TMR0's: n2 == 65*n0 + phase, where the
     * phase term (start skew, the 16-cycle remainder per TMR0
     * period, the first-pass prescaler phase shift from the POR vs
     * re-run start state, one boundary wrap) is bounded by
     * -16 .. 132 + (n0 >> 6). A dropped TMR1 dispatch (the TMR1IE
     * flag-gate), a misdirected callback, or a corrupted TMR0
     * prescaler breaks these (a stalled source diverges by at least
     * 65 per TMR0 period, far outside the bounds). */
    {
        int32_t d1 = (int32_t)t1c - 2 * (int32_t)t0w;
        if (d1 < -6 || d1 > 6) fail(0x08);
    }
    {
        int32_t d2 = (int32_t)t2c - 65 * (int32_t)t0w;
        if (d2 < -16 || d2 > 132 + ((int32_t)t0w >> 6)) fail(0x09);
    }

    /* No monotonicity violation survived the torn-read retry. */
    CHECK(mono_bad == 0u, 0x0A);

    /* TMR0's overflow callback is not checkable under MPLAB SIM: with
     * the prescaler assigned (PSA=0) the simulator raises TMR0IF only
     * as a rare transient (a TMR0-only probe observed zero latches
     * across thousands of rollover samples; this gate's own T0I
     * counter logs the rare observations), and the transient does not
     * deterministically drive the ISR. The TMR0 interrupt path
     * (dispatch branch, handler, callback) is exercised by the
     * real-target smoke build of this same source; the sim gate
     * covers TMR0 through its hardware counter (0x05), the rate
     * ratios (0x08/0x09), and the enable/flag image (0x01/0x02/0x0B). */

    /* Diagnostics: final counts, visible in the mdb UART capture
     * (no-op on the real-target smoke build). */
    epic_harness_log("T0W=");
    log_hex16(t0w);
    epic_harness_log(" T0C=");
    log_hex16(g_t0_count);
    epic_harness_log(" T0I=");
    log_hex16(t0_if_seen);
    epic_harness_log(" T1=");
    log_hex16(t1c);
    epic_harness_log(" T2=");
    log_hex16(t2c);
    epic_harness_log("\n");

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(g_fail == 0u);
}
