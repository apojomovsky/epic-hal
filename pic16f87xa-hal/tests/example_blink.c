/* Blink an LED on RB0 from a Timer0 overflow: the canonical "the HAL
 * drives a real application" smoke test. Wiring: LED+resistor between
 * RB0 and GND, 20 MHz HS crystal. Timer0 Fosc/4, 1:256 prescaler,
 * reload 0, overflows every ~13 ms (~76 Hz toggle). */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16_irq.h"
#include "core/pic16f87xa_wdt_sleep.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 256 × 256 = 65536 cycles per Timer0
 *  overflow at 1:256, so 600k cycles give ~9 toggles. */
#define SIM_CYCLES  600000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/* Timer0 overflow callback, runs in interrupt context (target) or the
 * sim IRQ callback (host). */
static void on_t0_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    g_toggle_count++;
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as output, start low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: internal Fosc/4, 1:256 prescaler, reload 0, toggle on
     *    each overflow. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);

    /* 3. Arm the Timer0 interrupt (EPIC_TIMER0_Init set TMR0IE; now set GIE).
     *    On the sim the IRQ fires regardless, so this is harmless there. */
    EPIC_IRQ_Restore(1);

    /* 4. Let time pass: the harness bounds the loop on the host and
     *    pumps the sim each iteration (no-op on target, where real
     *    time advances on its own). WDT refresh is a host no-op, so it
     *    is called unconditionally. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
    }

    epic_harness_log("RB0 toggled %u times.\n", (unsigned)g_toggle_count);
    return epic_harness_report(g_toggle_count >= 2U);
}
