/*
 * Timer2 match smoke: count Timer2 PR2 matches via callback, the §4
 * gate's host-sim half for the Timer2 driver. One source builds for
 * host sim and real XC8 target via `core/epic_harness.h`. Timer2
 * 8-bit/1:1 prescaler/1:1 postscaler/PR2=9, match every 10 cycles.
 */

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "peripherals/pic18f1320_timer2.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 10 cycles per Timer2 match at
 * 1:1/1:1 with PR2=9, so 100 cycles give ~10 matches. */
#define SIM_CYCLES  100UL

/* Match count, the ISR is the only writer. */
static volatile uint32_t g_match_count = 0;

/** @brief  Timer2 match callback.
 *
 *          Runs in interrupt context (target) or the sim IRQ callback
 *          (host); bumps the match count.
 */
static void on_t2_match(void)
{
    g_match_count++;
}

/**
 * @brief  Count Timer2 PR2 matches.
 *
 *          Runs SIM_CYCLES harness cycles on the host, or busy-spins
 *          forever on target hardware while the Timer2 ISR counts.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* Timer2: 1:1 prescaler, 1:1 postscaler, PR2=9, count each match. */
    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    h.Prescaler        = TIMER2_PRESCALER_1_1;
    h.Postscaler       = TIMER2_POSTSCALER_1_1;
    h.Period           = 9U;
    h.OverflowCallback = on_t2_match;
    EPIC_TIMER2_Init(&h);
    EPIC_TIMER2_Start(&h);

    /* Arm the Timer2 interrupt (EPIC_TIMER2_Init set TMR2IE; now enable
     * the masters). */
    EPIC_IRQ_Restore(1);

    /* Let time pass. On the host the harness bounds the loop to
     * SIM_CYCLES and pumps the sim each iteration. */
    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("Timer2 matched %u times.\n", (unsigned)g_match_count);
    return epic_harness_report(g_match_count >= 2U);
}
