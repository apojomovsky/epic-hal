/**
 * @file    combo_swuart_tick.c
 * @brief   C11 of the combination matrix
 *          (docs/superpowers/plans/2026-08-09-combination-matrix.md):
 *          epic-swuart + epic-tick interleaved on PIC16F877A.
 *
 * @details
 *   The swuart TX state machine is CCP2-compare-driven on a
 *   free-running Timer1 (its overflow interrupt stays disabled: the
 *   swuart gate notes it needs TMR1 as a timebase, never its ISR).
 *   epic-tick's 1 ms timebase is a Timer2 period-match ISR. The
 *   interleave under test: while the CCP2 ISR steps a real 4-byte
 *   payload through the shift register (10 compare events per byte,
 *   ~1 ms of wire time per byte at 9600 baud), the Timer2 ISR fires
 *   ~4 times per byte and preempts the swuart's own ISR/ring path.
 *   The gate writes the payload four rounds in a row, waits for each
 *   to drain (the ring empties and the state machine lands back in
 *   TX_IDLE, i.e. the last stop bit's compare event actually fired),
 *   and cross-checks that the tick kept counting through every
 *   transmission, GIE is still set when the last one completed, and
 *   PIE1/PIE2 hold exactly the sources the two modules armed
 *   (TMR2IE, CCP1IE, CCP2IE; TXIE stays off because the harness
 *   reports by polling).
 *
 *   The swuart/CCP handle-pointer fragility this gate was built to
 *   catch is fixed at the source: XC8 v4.00 compiles the CCP driver's
 *   stored handle pointers as 8-bit values dereferenced with IRP=1
 *   (RAM banks 2/3 only), so the ISR path only worked when the CCP
 *   handle structs happened to land in 0x100-0x1FF. The linker's
 *   best-fit placement of the auto pool is layout-dependent: in an
 *   early build, adding epic-tick's code moved the handles to bank
 *   0/1, the CCP2 handler's callback read landed in the wrong bank,
 *   and the TX chain stalled at the first compare event (the deadline
 *   never advanced; the EventCallback slot resolved to the tick's own
 *   counter increment, so every CCP2 event silently bumped g_tick_ms).
 *   The CCP driver now stores its own per-instance callback copy and
 *   the IRQ handlers call that copy directly, never dereferencing a
 *   caller handle, so this gate runs with the handle unpinned. It is
 *   the regression gate for that fragility; see the C11 section of
 *   docs/superpowers/plans/2026-08-09-combination-matrix.md.
 *
 *   MPLAB SIM lessons applied from the C1 gate: pending timer flags
 *   are cleared before any GIE-on edge (the sim wedges the ISR path
 *   when GIE is enabled while a timer interrupt is latched, and main
 *   re-runs after `ljmp start` with TMR2 still running, so the clear
 *   is re-applied per pass), and the 87XA vector is already
 *   bank-normalized, so no extra asm belongs here.
 */

#include "epic_swuart.h"
#include "epic_tick.h"
#include "core/epic_harness.h"
#include "core/pic16_irq.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/* Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 * The happy path finishes in a few thousand iterations (each round's
 * 4-byte TX is ~1 ms of wire time per byte); the bound only matters
 * when a failure makes the payload never drain, and 200000 is the
 * swuart gate's own proven budget (finishes well inside a 60000 ms
 * wait under mdb on PIC16F877A). */
#define SIM_ITERATIONS 200000UL

#define TX_BYTES  4u
#define TX_ROUNDS 4u

/* Deliberately unpinned (see the file header): the CCP driver no
 * longer dereferences this handle (or its own CCP handle copies)
 * through the 8-bit pointer path, so its bank is irrelevant. */
static EPIC_SWUART_HandleTypeDef g_h;

static uint16_t g_fail = 0u;

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

/* Known payload: 'A','B','C','D', shifted LSB-first on the wire. */
static const uint8_t g_payload[TX_BYTES] = { 0x41u, 0x42u, 0x43u, 0x44u };

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* Re-run safety (C1 lesson): when main re-runs after returning
     * (`ljmp start`), TMR2 is still running with TMR2IF latched, and
     * the next GIE-on edge would wedge the sim's ISR path. Stop TMR2
     * and clear the pending flags before any GIE-on, per pass. */
    EPIC_REG8(PIC_REG_T2CON) &= (uint8_t)~0x04u;   /* TMR2ON = 0 */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CCP1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_CCP2);

    /* swuart channel A (RC1 TX, RC2 RX): starts Timer1 free-running
     * (overflow interrupt disabled, the swuart gate's TMR1 note),
     * arms CCP2 compare and CCP1 capture, and turns GIE on itself.
     * All three flags are clear, so that edge is safe. */
    CHECK(EPIC_SWUART_Init(&g_h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                           FOSC_HZ, 9600u) == EPIC_OK, 0x00);

    /* 1 ms tick on Timer2, layered on top of the swuart's live TMR1
     * and GIE. TMR2IF is clear and TMR2 stopped, so the tick's own
     * internal GIE-on is safe too. */
    epic_tick_init(FOSC_HZ);

    uint32_t tick_start = epic_tick_get();
    CHECK(tick_start <= 10u, 0x01);

    uint8_t all_queued = 1u;
    uint8_t drained = 0u;
    uint8_t completed = 0u;
    uint8_t paced = 1u;        /* tick advanced >= 1 ms inside every round */

    for (uint8_t round = 0u; round < TX_ROUNDS; round++) {
        size_t queued = EPIC_SWUART_Write(&g_h, g_payload, TX_BYTES);
        if (queued != TX_BYTES) {
            all_queued = 0u;
            break;
        }

        uint32_t t0 = epic_tick_get();
        uint8_t round_done = 0u;
        for (uint32_t i = 0; epic_harness_running(i); i++) {
            epic_harness_tick();
            EPIC_WDT_Refresh();
            if (g_h.tx_count == 0u && g_h.tx_state == 0u) {
                /* tx_count 0 and TX_IDLE (state 0): the ring drained
                 * and the last stop bit's compare event fired, so the
                 * whole payload shifted out under the live tick. */
                uint32_t t1 = epic_tick_get();
                if (t1 - t0 < 1u) {
                    paced = 0u;
                }
                drained = 1u;
                completed = 1u;
                round_done = 1u;
                break;
            }
        }
        if (!round_done) {
            break;
        }
    }

    /* GIE must still be set when the last transmission completed. */
    uint8_t gie_alive = (uint8_t)(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE);
    uint32_t tick_end = epic_tick_get();

    /* Stop everything before the register checks, like the C1 gate. */
    EPIC_IRQ_Disable();

    uint8_t pie1 = 0u;
    EPIC_BANK1_READ8(PIE1, pie1);
    uint8_t pie2 = 0u;
    EPIC_BANK1_READ8(PIE2, pie2);

    CHECK(all_queued != 0u, 0x02);       /* every payload fully queued */
    CHECK(drained != 0u, 0x03);          /* payload drained via the ISR */
    CHECK(completed != 0u, 0x04);        /* TX state machine back to TX_IDLE */
    CHECK(paced != 0u, 0x05);            /* tick advanced during every TX */
    CHECK(gie_alive != 0u, 0x06);        /* GIE survived the interleave */
    CHECK((pie1 & (PIC_PIE1_TMR2IE | PIC_PIE1_CCP1IE)) ==
              (PIC_PIE1_TMR2IE | PIC_PIE1_CCP1IE), 0x07);
    CHECK((pie1 & (uint8_t)~(PIC_PIE1_TMR2IE | PIC_PIE1_CCP1IE)) == 0u, 0x08);
    CHECK((pie2 & PIC_PIE2_CCP2IE) != 0u, 0x09);
    CHECK((pie2 & (uint8_t)~PIC_PIE2_CCP2IE) == 0u, 0x0A);
    CHECK(g_h.error_count == 0u, 0x0B);  /* RX side saw no noise */
    CHECK(tick_end > tick_start, 0x0C);  /* tick kept counting overall */

    epic_harness_log("combo swuart+tick: payload drained under live tick\n");
    return epic_harness_report(g_fail == 0u);
}
