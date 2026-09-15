/*
 * Timer3 overflow smoke: count Timer3 overflows via callback, the §4
 * gate's host-sim half for the Timer3 driver. One source builds for
 * host sim and real XC8 target via `core/epic_harness.h`. Timer3
 * 16-bit/Fosc/4/1:1, overflow every 65536 cycles.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_timer3.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 65536 cycles per Timer3 overflow
 * at 1:1 in 16-bit mode, so 150k cycles give ~2 overflows. */
#define SIM_CYCLES  150000UL

/* Overflow count, the ISR is the only writer. */
static volatile uint32_t g_overflow_count = 0;

/** @brief  Timer3 overflow callback.
 *
 *          Runs in interrupt context (target) or the sim IRQ callback
 *          (host); bumps the overflow count.
 */
static void on_t3_overflow(void)
{
    g_overflow_count++;
}

/**
 * @brief  Count Timer3 overflows.
 *
 *          Runs SIM_CYCLES harness cycles on the host, or busy-spins
 *          forever on target hardware while the Timer3 ISR counts.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* Timer3: 16-bit, internal Fosc/4, 1:1 prescaler, reload 0,
     * count each overflow. */
    TIMER3_HandleTypeDef h = TIMER3_HANDLE_DEFAULT;
    h.ClockSource      = TIMER3_CLOCK_INTERNAL;
    h.Prescaler        = TIMER3_PRESCALER_1_1;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t3_overflow;
    EPIC_TIMER3_Init(&h);
    EPIC_TIMER3_Start(&h);

    /* Arm the Timer3 interrupt (EPIC_TIMER3_Init set TMR3IE; now enable
     * the masters). */
    EPIC_IRQ_Restore(1);

    /* Let time pass. On the host the harness bounds the loop to
     * SIM_CYCLES and pumps the sim each iteration. */
    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("Timer3 overflowed %u times.\n", (unsigned)g_overflow_count);
    return epic_harness_report(g_overflow_count >= 2U);
}
