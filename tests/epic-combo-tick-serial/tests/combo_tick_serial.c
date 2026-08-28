/**
 * @file    combo_tick_serial.c
 * @brief   C6 of the combination matrix:
 *          epic-tick's live 1 ms Timer2 ISR churning against
 *          epic-serial's ring-buffered TX path, both real modules,
 *          one firmware, one bank state.
 *
 * @details
 *   The bug class this gate hunts: the tick ISR preempting the serial
 *   layer's class-G Disable/Restore sites (the TX callback's atomic
 *   pop, epic_serial.c:48-52, and the blocking push's
 *   Disable/Restore + dispatch loop, epic_serial.c:97-106) must not
 *   tear the ring, lose GIE, or wedge the tick; the dispatch's TXIE
 *   gate must keep the TX handler out of every tick vector.
 *
 *   MPLAB SIM's USART model shapes the discipline (the same findings
 *   that shaped epic-serial/tests/sim_serial.c):
 *   - TXIF is a permanent pending status bit in the sim (it never
 *     clears on a TXREG write), so TXIE on with GIE on is an
 *     interrupt storm, and the storm's in-ISR Disable/Restore churn
 *     trips the simulator's async-delivery quirk and wedges GIE.
 *     Every TX phase therefore runs with GIE OFF, the drain driven by
 *     manual epic_dispatch_all_irqs() calls through the dispatch's
 *     TXIE gate, exactly like sim_serial phases 2-3.
 *   - Enabling GIE while a timer interrupt is already pending wedges
 *     the sim (the C1 Finding 10.1 class). TMR2 keeps free-running
 *     through the GIE-off drain windows, so its flag is cleared
 *     before every GIE-on edge.
 *   The tick therefore increments in the GIE-on windows between TX
 *   phases (and via the manual dispatches' own TMR2 branch during
 *   drains); the cross-checks verify it stayed alive and monotone
 *   through the whole churn (GIE never lost), and every TX line
 *   drains to tx_pending() == 0 with the payload bytes captured
 *   byte-exact in the UART stream right after the "TX:" line header.
 *
 *   Bounded and self-reporting (the harness contract); no RX
 *   involvement, so the MPLAB SIM RX wall does not apply.
 */

#include "core/epic_harness.h"
#include "core/pic16_irq.h"
#include "epic_serial.h"
#include "epic_tick.h"
#include "target/pic16f87xa_platform.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_ITERATIONS 2000UL

/* One TX line every N iterations. The line is 34 bytes (32-byte ring
 * + 2), so the write blocks inside epic_serial_write and exercises
 * its internal Disable/Restore + dispatch drain loop, then the outer
 * manual drain finishes the ring. */
#define TX_EVERY_N_ITERS 40u
#define TX_LINE_LEN 34u
#define EXPECTED_LINES (SIM_ITERATIONS / TX_EVERY_N_ITERS)

/* Per-pass simulated GIE-on time is ~90 ms, so the tick must advance
 * at least this much or the ISR was wedged. */
#define TICK_MIN_MS 20u
/* The end-of-run pump must observe the tick advancing at least this
 * much after the last drain (GIE not lost by the churn). */
#define TICK_ALIVE_MS 3u

static uint16_t g_fail = 0u;

/**
 * @brief Record a check failure and log its index as two hex digits.
 */
static void fail(uint8_t idx)
{
    /* One static RAM buffer, not stack locals or const pointers: the
     * epic-cc build has no const-address form and no array allocas. */
    static const char hx[] = "0123456789ABCDEF";
    static char c[5];
    g_fail++;
    c[0] = 'F';
    c[1] = hx[(idx >> 4) & 0xF];
    c[2] = hx[idx & 0xF];
    c[3] = '.';
    c[4] = '\0';
    epic_harness_log(c);
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/**
 * @brief Build the n-th TX line: "C6-<hex n>:" plus 28 pattern chars.
 *
 * Every line's bytes in the captured UART stream are distinct and
 * deterministically verifiable.
 */
static void build_line(uint8_t *line, uint8_t n)
{
    static const char hx[] = "0123456789ABCDEF";
    static const char pat[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";   /* 36 chars */
    uint8_t j;
    line[0] = (uint8_t)'C';
    line[1] = (uint8_t)'6';
    line[2] = (uint8_t)'-';
    line[3] = (uint8_t)hx[(n >> 4) & 0xF];
    line[4] = (uint8_t)hx[n & 0xF];
    line[5] = (uint8_t)':';
    for (j = 0u; j < (uint8_t)(TX_LINE_LEN - 6u); j++) {
        line[6u + j] = (uint8_t)pat[(uint16_t)((uint16_t)n + j) % 36u];
    }
}

/**
 * @brief Log the "TX:<hex n>" header preceding each line's payload bytes.
 *
 * The sim harness prints fmt verbatim, no varargs.
 */
static void log_tx_header(uint8_t n)
{
    static const char hx[] = "0123456789ABCDEF";
    static char s[4];
    s[0] = 'T';
    s[1] = 'X';
    s[2] = ':';
    s[3] = '\0';
    epic_harness_log(s);
    s[0] = hx[(n >> 4) & 0xF];
    s[1] = hx[n & 0xF];
    s[2] = '\0';
    epic_harness_log(s);
}

/**
 * @brief Run the epic-tick + epic-serial ring-buffered TX gate (C6).
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* Re-entry safety (the sim runtime re-runs main after it
     * returns): stop the timer and clear its flag before
     * epic_tick_init's GIE-on edge (the C1 lesson: a GIE-on edge with
     * TMR2IF latched wedges the sim's ISR path). EPIC_TIMER2_Init
     * clears the flag itself, but keep the explicit pre-clean per
     * C1's discipline. */
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~PIC_T2CON_TMR2ON;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);

    /* GIE on (epic_tick_init enables it) with the 1 ms Timer2 tick
     * live; then the interrupt-driven ring-buffered USART (TXIE stays
     * off until a write arms it, and only the manual dispatch may run
     * it). */
    epic_tick_init(FOSC_HZ);
    epic_serial_init(FOSC_HZ, 9600u);

    uint32_t t_last = epic_tick_get();
    uint32_t t0 = t_last;
    uint8_t write_ok = 1u;
    uint8_t gie_ok = 1u;
    uint16_t tx_lines = 0u;

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        /* Bounded busy work so the loop advances simulated time
         * through several 1 ms tick periods between serial phases. */
        for (volatile uint16_t w = 0u; w < 32u; w++) {
        }

        if ((i % TX_EVERY_N_ITERS) == 0u) {
            /* ---- Serial phase (the serial gate's discipline) ---- */
            (void)EPIC_IRQ_Disable();            /* GIE off */

            /* static, not a stack local: the epic-cc build lowers
             * only global RAM arrays (no array allocas). */
            static uint8_t line[TX_LINE_LEN];
            uint8_t n = (uint8_t)(tx_lines & 0xFFu);
            log_tx_header(n);
            build_line(line, n);
            if (epic_serial_write(line, (int)TX_LINE_LEN) != (int)TX_LINE_LEN) {
                write_ok = 0u;
            }
            /* Drain: one dispatch pops one byte (TXIF stays set in
             * the sim); bounded, then one extra dispatch hits the
             * callback's empty-ring branch and disables TXIE so the
             * GIE-on edge cannot start a storm. */
            for (uint8_t k = 0u; k < 40u &&
                                epic_serial_tx_pending() > 0; k++) {
                epic_dispatch_all_irqs();
            }
            epic_dispatch_all_irqs();
            if (epic_serial_tx_pending() != 0) {
                fail(0x0C);   /* ring did not empty */
            }
            tx_lines++;

            /* GIE back on: clear the latched TMR2IF first (C1's wedge
             * discipline; TMR2 kept running through the drain). */
            EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);
            EPIC_IRQ_Restore(1);

            /* ---- Tick checkpoint ---- */
            {
                uint32_t t = epic_tick_get();
                if (t < t_last) {
                    fail(0x01);   /* tick regressed: torn or wedged */
                }
                t_last = t;
            }
            /* GIE probe: two back-to-back reads; only a double-zero
             * counts as lost (a single read could land inside the
             * tick ISR, where GIE is cleared by hardware). */
            {
                uint8_t g1 = EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE;
                uint8_t g2 = EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE;
                if ((g1 | g2) == 0u) {
                    gie_ok = 0u;   /* GIE lost by the churn */
                }
            }
        }
    }

    /* ---- Final alive pump: GIE on, TXIE off, the tick must keep
     * firing after the last drain. ---- */
    {
        uint32_t t_end = epic_tick_get();
        for (uint32_t i = 0; epic_harness_running(i) && i < 20000u &&
                            (epic_tick_get() - t_end) < TICK_ALIVE_MS; i++) {
            /* spin; the tick ISR advances the counter */
        }
        CHECK((epic_tick_get() - t_end) >= TICK_ALIVE_MS, 0x00);
    }

    /* Quiet before the final checks. */
    (void)EPIC_IRQ_Disable();

    /* Cross-checks. */
    CHECK((t_last - t0) >= TICK_MIN_MS, 0x04);    /* tick advanced all pass */
    CHECK(write_ok != 0u, 0x02);                  /* every write accepted all */
    CHECK(gie_ok != 0u, 0x03);                    /* GIE never observed lost */
    CHECK(tx_lines == EXPECTED_LINES, 0x05);      /* every line written */
    CHECK(epic_serial_tx_pending() == 0, 0x06);   /* ring fully drained */
    /* The tick source must still be armed and the TX source at rest
     * (the empty-ring branch disabled it). */
    {
        uint8_t pie1 = 0u;
        EPIC_BANK1_READ8(PIE1, pie1);
        CHECK((pie1 & PIC_PIE1_TMR2IE) != 0u, 0x07);
        CHECK((pie1 & PIC_PIE1_TXIE) == 0u, 0x08);
    }
    /* flush() returns only when the shift register has drained too. */
    epic_serial_flush();
    CHECK(epic_serial_tx_pending() == 0, 0x09);

    EPIC_HARNESS_LOG_STATIC("combo tick-serial done\n");
    return epic_harness_report(g_fail == 0u);
}
