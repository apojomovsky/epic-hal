/*
 * Epic HAL reference project, PIC16F54: the 12-bit baseline core's
 * Timer0 has no interrupt (DS41213D §4.0), so this demo polls the
 * counter and toggles RB0 on every 8-bit overflow. 4 MHz XT crystal.
 * WDT is off: the 512-word exemplar has no room for a refresh loop
 * (measured; see tests/example_blink.c), and the demo exists to show
 * the HAL's GPIO + polled-timer flow, not watchdog discipline. See
 * MPLABX.md to add Epic HAL to an existing project.
 */
#include <xc.h>

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"

/* Config fields validated by the config-key audit (DS41213D
 * §14.1): OSC, WDT, CP. */
#pragma config OSC = XT
#pragma config WDT = OFF
#pragma config CP = OFF

/**
 * @brief Blink RB0 from the polled Timer0 (no interrupt exists on the
 *        12-bit baseline core).
 */
int main(void)
{
    uint8_t last_t0 = 0U;
    TIMER0_HandleTypeDef h;

    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.ClockEdge         = TIMER0_EDGE_RISING;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
    last_t0 = EPIC_TIMER0_ReadCounter();

    for (;;)
    {
        uint8_t t0 = EPIC_TIMER0_ReadCounter();
        if (t0 < last_t0)
        {
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        last_t0 = t0;
    }
}
