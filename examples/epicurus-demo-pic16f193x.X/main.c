/*
 * Epicurus reference project, PIC16F193X: Timer0 overflow toggles RB0
 * via the HAL's weak-ISR dispatch (HAL-only family, no hand-written
 * vector). See MPLABX.md to add Epicurus to an existing project.
 */
#include <xc.h>

#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_timer0.h"
#include "core/pic16f193x_irq.h"
#include "core/pic16f193x_wdt_sleep.h"

#pragma config FOSC = INTOSC
#pragma config WDTE = ON
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = ON
#pragma config CLKOUTEN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config LVP = OFF
#pragma config STVREN = ON
#pragma config PLLEN = OFF
#pragma config WRT = OFF

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

    for (;;) {
        EPIC_WDT_Refresh();
    }
}
