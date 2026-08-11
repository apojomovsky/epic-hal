/*
 * End-to-end smoke test for the PIC18 Timer3 driver on the host sim:
 * 16-bit counter at Fosc/4 with a 1:8 prescaler, TMR3IF (PIR2<1>) fires
 * every 524288 instruction cycles; overflow count checked. One source
 * builds for host sim and XC8 target.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include <stdio.h>

/** 16-bit, 1:8 prescaler -> overflow every 0x10000 x 8 = 524288 cycles. */
#define OVERFLOW_CYCLES     524288UL
#define EXPECTED_OVERFLOWS  3U
#define SIM_CYCLES          ((OVERFLOW_CYCLES * EXPECTED_OVERFLOWS) + 2048UL)

static volatile uint32_t g_overflows = 0;

/** @brief  Timer3 overflow callback, counts overflows. */
static void on_t3_overflow(void)
{
    g_overflows++;
}

/** @brief  Timer3 driver smoke test.
 *
 *          Counts TMR3IF overflows over the sim run and checks the total.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    TIMER3_HandleTypeDef h = TIMER3_HANDLE_DEFAULT;
    h.Prescaler        = TIMER3_PRESCALER_1_8;
    h.ClockSource      = TIMER3_CLOCK_INTERNAL;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t3_overflow;
    EPIC_TIMER3_Init(&h);
    EPIC_TIMER3_Start(&h);
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        if (g_overflows >= EXPECTED_OVERFLOWS) break;
    }

    epic_harness_log("Timer3: %u overflows (expected >= %u)\n",
                     (unsigned)g_overflows, (unsigned)EXPECTED_OVERFLOWS);
    return epic_harness_report(g_overflows >= EXPECTED_OVERFLOWS);
}
