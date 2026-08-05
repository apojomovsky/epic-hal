/**
 * @file    example_timer2.c
 * @brief   End-to-end smoke test for the PIC18 Timer2 driver on the host sim.
 *
 * @details
 *   Timer2 with PR2=249, prescaler 1:1, postscaler 1:1 fires every 250
 *   instruction cycles (DS39632E §12.0: period = (PR2+1) x prescaler x
 *   postscaler = 250 x 1 x 1 = 250). The test checks the overflow count and
 *   the first-overflow cycle. One source builds for host sim and XC8 target
 *   with no `#ifdef`.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include <stdio.h>

#define EXPECTED_PERIOD_CYCLES  250UL
#define EXPECTED_OVERFLOWS      5U
#define SIM_CYCLES              ((EXPECTED_PERIOD_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t g_overflows   = 0;
static volatile uint32_t g_first_cycle = 0;
static uint32_t g_cycle = 0;

static void on_t2_overflow(void)
{
    g_overflows++;
    if (g_overflows == 1U) {
        g_first_cycle = g_cycle;
    }
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    h.Prescaler        = TIMER2_PRESCALER_1_1;
    h.Postscaler       = TIMER2_POSTSCALER_1_1;
    h.Period           = 249U;     /* PR2 = 249 -> 250 ticks per period. */
    h.OverflowCallback = on_t2_overflow;
    EPIC_TIMER2_Init(&h);
    EPIC_TIMER2_Start(&h);
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        g_cycle = i + 1;
        pic8_harness_tick();
        if (g_overflows >= EXPECTED_OVERFLOWS) break;
    }

    /* Allow +/-2 cycles slack for sim bookkeeping (we observe after the
     * step that just incremented). */
    int32_t delta = (int32_t)g_first_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    pic8_harness_log("Timer2: %u overflows, first at cycle %u (expected ~%u)\n",
                     (unsigned)g_overflows, (unsigned)g_first_cycle,
                     (unsigned)EXPECTED_PERIOD_CYCLES);
    return pic8_harness_report(g_overflows >= EXPECTED_OVERFLOWS && delta <= 2);
}
