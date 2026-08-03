/**
 * @file    example_blink.c
 * @brief   Blink an LED on RB0 from a Timer0 overflow, the canonical
 *          "the HAL drives a real application" smoke test.
 *
 * @details
 *   Timer0 overflows drive an interrupt; the ISR toggles RB0. The main
 *   loop just lets time pass (pumping the sim on host, busy-spinning on
 *   target, via core/pic8_harness.h) and refreshes the WDT.
 *
 *   Wiring: LED + resistor between RB0 and GND. The 193X has a 32 MHz
 *   internal oscillator; with Fosc/4 + 1:256 prescaler + reload 0,
 *   Timer0 overflows every 256 * 256 = 65536 cycles, so the pin toggles
 *   at ~FCY/(65536*2).
 *
 *   Expected register image (host sim, after init):
 *     TRISB  = 0xFF & ~0x01 = 0xFE   (RB0 output, rest input)
 *     ANSELB = 0xFF & ~0x01 = 0xFE   (RB0 digital, rest analog)
 *     LATB   = 0x00                   (RB0 driving low at start)
 *     OPTION_REG = WPUEN=1,INTEDG=1,T0CS=0,T0SE=0,PSA=0,PS=111 = 0xD7
 *     INTCON = TMR0IE=1, PEIE=1, GIE=1 = 0xE8 (after HAL_IRQ_Restore(1))
 *   The ISR flips LATB<0> on every overflow; g_toggle_count counts them.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_timer0.h"
#include "core/pic16f193x_irq.h"
#include "core/pic16f193x_wdt_sleep.h"
#include "core/pic8_harness.h"

/** Simulated run length (host only). 256 * 256 = 65536 cycles per
 *  Timer0 overflow at 1:256, so 600k cycles give ~9 toggles. */
#define SIM_CYCLES  600000UL

/* Toggle count, the ISR is the only writer. */
static volatile uint32_t g_toggle_count = 0;

/* Timer0 overflow callback, runs in interrupt context (target) or the
 * sim IRQ callback (host). */
static void on_t0_overflow(void)
{
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
    g_toggle_count++;
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    /* 1. RB0 as digital output, start low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: internal Fosc/4, 1:256 prescaler, reload 0, toggle on
     *    each overflow. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    HAL_TIMER0_Init(&h);
    HAL_TIMER0_Start(&h);

    /* 3. Arm the Timer0 interrupt (HAL_TIMER0_Init set TMR0IE; now GIE). */
    HAL_IRQ_Restore(1);

    /* 4. Let time pass. On the target this busy-spins forever, refreshing
     *    the WDT while the Timer0 ISR toggles RB0; on the host the harness
     *    bounds the loop to SIM_CYCLES and pumps the sim each iteration.
     *    HAL_WDT_Refresh is a no-op on the host, so it is called
     *    unconditionally. */
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
        HAL_WDT_Refresh();
    }

    pic8_harness_log("RB0 toggled %u times.\n", (unsigned)g_toggle_count);
    return pic8_harness_report(g_toggle_count >= 2U);
}
