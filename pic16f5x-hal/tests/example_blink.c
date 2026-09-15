/* Blink an LED on RB0 from Timer0: the canonical "the HAL drives a
 * real application" smoke test. Wiring: LED+resistor between RB0 and
 * GND, 4 MHz XT crystal. Timer0 Fosc/4, 1:256 prescaler, reload 0,
 * overflows every ~13 ms. This core has no Timer0 interrupt
 * (DS41213D §4.0), so the blink polls TMR0 and toggles the pin
 * directly: the section-4 exemplar proof that GPIO + the polled
 * timer drive real firmware.
 *
 * The 16F54 carries 25 B of GPR and 512 words of flash (DS41213D
 * §1.0), the tightest budget in this repo, so this example keeps its
 * loop state in one 16-bit counter plus two byte-width registers and
 * one reused handle; there is no room for a variadic log (XC8's
 * printf machinery alone overflows flash) or the WDT refresh loop, so
 * the manifest config disables the WDT for both the target and sim
 * builds. The pass/fail verdict goes out through
 * epic_harness_report's fixed marker. */

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"
#include "core/pic16_irq.h"
#include "core/pic16f5x_wdt_sleep.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). 256 × 256 = 65536 cycles per
 *  Timer0 overflow at 1:256, so 400k cycles give ~6 overflows. */
#define SIM_CYCLES  400000UL

/**
 * @brief Blink RB0 from a Timer0 overflow-driven polled loop.
 */
int main(void)
{
    uint8_t last_t0 = 0U;
    uint8_t overflowed = 0U;
    TIMER0_HandleTypeDef h;

    epic_harness_init(SIM_CYCLES);

    /* 1. RB0 as output, start low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: internal Fosc/4, 1:256 prescaler, reload 0. Polled:
     *    no interrupt exists on this core. */
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.ClockEdge         = TIMER0_EDGE_RISING;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
    last_t0 = EPIC_TIMER0_ReadCounter();

    /* 3. Let time pass, polling the counter for each 8-bit wraparound
     *    and toggling the pin on every overflow. The host run is
     *    bounded by the harness; a real target runs forever (the
     *    family-blind harness_running is always 1), and the infinite
     *    loop avoids the 32-bit iteration counter that overflows the
     *    16F54's 25-byte GPR budget. */
#ifdef PIC16F5X_HOST
    for (uint32_t i = 0U; epic_harness_running(i); i++)
    {
        uint8_t t0 = EPIC_TIMER0_ReadCounter();
        if (t0 < last_t0)
        {
            overflowed++;
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        last_t0 = t0;
        epic_harness_tick();
    }
    return epic_harness_report(overflowed >= 2U);
#else
    for (;;)
    {
        uint8_t t0 = EPIC_TIMER0_ReadCounter();
        if (t0 < last_t0)
        {
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        last_t0 = t0;
    }
#endif
}
