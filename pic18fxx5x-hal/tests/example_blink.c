/*
 * Blink an LED on RB0 from a Timer0 overflow, the PIC18 analog of the
 * PIC16F87XA canonical HAL smoke test. One source builds for host sim
 * and real XC8 target via `core/epic_harness.h`; on real hardware:
 * LED + resistor on RB0 (active-high), 20 MHz crystal, Timer0
 * 8-bit/Fosc/4/1:256, overflow every ~13 ms (~76 Hz blink).
 */

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"
#include "peripherals/pic18fxx5x_gpio.h"
#include "peripherals/pic18fxx5x_timer0.h"
#include "core/pic18_irq.h"
#include "core/pic18fxx5x_wdt_sleep.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 256 x 256 = 65536 cycles per Timer0
 *  overflow at 1:256 in 8-bit mode, so 600k cycles give ~9 toggles. */
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

    /* 1. RB0 as output, start low (writes go through LATB, DS39632E §10.0). */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: 8-bit mode, internal Fosc/4, 1:256 prescaler, reload 0,
     *    toggle on each overflow. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.Mode              = TIMER0_BITMODE_8BIT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);

    /* 3. Arm the Timer0 interrupt (EPIC_TIMER0_Init set TMR0IE; now enable
     *    the masters). On the sim the IRQ fires regardless, so this is
     *    harmless there. */
    EPIC_IRQ_Restore(1);

    /* 4. Let time pass. On the target this busy-spins forever, refreshing
     *    the WDT while the Timer0 ISR toggles RB0; on the host the harness
     *    bounds the loop to SIM_CYCLES and pumps the sim each iteration.
     *    EPIC_WDT_Refresh is a no-op on the host, so it is called
     *    unconditionally. */
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
    }

    epic_harness_log("RB0 toggled %u times.\n", (unsigned)g_toggle_count);
    return epic_harness_report(g_toggle_count >= 2U);
}
