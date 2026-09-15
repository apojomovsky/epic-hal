/*
 * Epic HAL reference project, PIC16F77: a Timer0 overflow toggles RB0,
 * the minimal build smoke test. 20 MHz HS crystal, watchdog enabled and
 * refreshed in the main loop. See MPLABX.md to add Epic HAL to an
 * existing project.
 */
#include <xc.h>

#include "pic16f7x.h"
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"
#include "peripherals/pic16f7x_timer0.h"
#include "peripherals/pic16f7x_gpio.h"

/* Config words validated against the 16F77 DFP by the config-key audit
 * (see scripts/config-key-audit.py): the 16F77 exposes FOSC, WDTE,
 * PWRTE, BOREN, CP only. */
#pragma config FOSC = HS
#pragma config WDTE = ON
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config CP = OFF

/**
 * @brief Toggle RB0 on every Timer0 overflow (the weak-ISR dispatch).
 */
static void on_t0_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

/**
 * @brief Blink RB0 via Timer0 overflow interrupts and refresh the WDT.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = on_t0_overflow;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);

    EPIC_IRQ_Restore(1);

    for (;;)
    {
        EPIC_WDT_Refresh();
    }
}
