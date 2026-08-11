/*
 * End-to-end smoke test for the PIC18 Timer1 driver on the host sim:
 * 16-bit counter at Fosc/4 with a 1:1 prescaler, TMR1IF fires every
 * 65536 instruction cycles, overflow count checked. One source builds
 * for host sim and XC8 target (the harness abstracts the two models).
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include <stdio.h>

/** 16-bit, 1:1 prescaler -> overflow every 0x10000 = 65536 cycles. */
#define OVERFLOW_CYCLES     65536UL
#define EXPECTED_OVERFLOWS  3U
#define SIM_CYCLES          ((OVERFLOW_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t g_overflows = 0;

/** @brief  Timer1 overflow callback, counts overflows. */
static void on_t1_overflow(void)
{
    g_overflows++;
}

/** @brief  Timer1 driver smoke test.
 *
 *          Counts TMR1IF overflows over the sim run and checks the total.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        if (g_overflows >= EXPECTED_OVERFLOWS) break;
    }

    epic_harness_log("Timer1: %u overflows (expected >= %u)\n",
                     (unsigned)g_overflows, (unsigned)EXPECTED_OVERFLOWS);
    return epic_harness_report(g_overflows >= EXPECTED_OVERFLOWS);
}
