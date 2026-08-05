/**
 * @file    example_timer1.c
 * @brief   End-to-end smoke test for the PIC18 Timer1 driver on the host sim.
 *
 * @details
 *   Timer1 is a 16-bit counter clocked from Fosc/4 with a 1:1 prescaler.
 *   Expected: TMR1IF fires every 65 536 instruction cycles; the test counts
 *   overflows. One source builds for the host sim and a real XC8 target with
 *   no `#ifdef` (the harness abstracts the two execution models).
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include <stdio.h>

/** 16-bit, 1:1 prescaler -> overflow every 0x10000 = 65536 cycles. */
#define OVERFLOW_CYCLES     65536UL
#define EXPECTED_OVERFLOWS  3U
#define SIM_CYCLES          ((OVERFLOW_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t g_overflows = 0;

static void on_t1_overflow(void)
{
    g_overflows++;
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);
    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
        if (g_overflows >= EXPECTED_OVERFLOWS) break;
    }

    pic8_harness_log("Timer1: %u overflows (expected >= %u)\n",
                     (unsigned)g_overflows, (unsigned)EXPECTED_OVERFLOWS);
    return pic8_harness_report(g_overflows >= EXPECTED_OVERFLOWS);
}
