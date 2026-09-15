/*
 * Timer1 overflow smoke: count Timer1 overflows via callback, the §4
 * gate's host-sim half for the Timer1 driver. One source builds for
 * host sim and real XC8 target via `core/epic_harness.h`. Timer1
 * 16-bit/Fosc/4/1:1, overflow every 65536 cycles.
 */

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "peripherals/pic18f1320_timer1.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 65536 cycles per Timer1 overflow
 * at 1:1 in 16-bit mode, so 150k cycles give ~2 overflows. */
#define SIM_CYCLES  150000UL

/* Overflow count, the ISR is the only writer. */
static volatile uint32_t g_overflow_count = 0;

/** @brief  Timer1 overflow callback.
 *
 *          Runs in interrupt context (target) or the sim IRQ callback
 *          (host); bumps the overflow count.
 */
static void on_t1_overflow(void)
{
    g_overflow_count++;
}

/**
 * @brief  Count Timer1 overflows.
 *
 *          Runs SIM_CYCLES harness cycles on the host, or busy-spins
 *          forever on target hardware while the Timer1 ISR counts.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* Timer1: 16-bit, internal Fosc/4, 1:1 prescaler, reload 0,
     * count each overflow. */
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t1_overflow;
    EPIC_TIMER1_Init(&h);
    EPIC_TIMER1_Start(&h);

    /* Arm the Timer1 interrupt (EPIC_TIMER1_Init set TMR1IE; now enable
     * the masters). */
    EPIC_IRQ_Restore(1);

    /* Let time pass. On the host the harness bounds the loop to
     * SIM_CYCLES and pumps the sim each iteration. */
    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("Timer1 overflowed %u times.\n", (unsigned)g_overflow_count);
    return epic_harness_report(g_overflow_count >= 2U);
}
