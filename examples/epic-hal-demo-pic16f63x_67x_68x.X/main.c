/*
 * Epic HAL reference project, PIC16F677: Timer0 overflow toggles RB4
 * via the HAL's weak-ISR dispatch. Internal oscillator (INTRCIO).
 * See MPLABX.md to add Epic HAL to an existing project.
 */
#include <xc.h>

#include "peripherals/pic16f63x_67x_68x_gpio.h"
#include "peripherals/pic14_timer0.h"
#include "core/pic16_irq.h"
#include "core/pic14_wdt_sleep.h"

#pragma config FOSC = INTRCIO
#pragma config WDTE = ON
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = ON
#pragma config IESO = OFF
#pragma config FCMEN = OFF

/* Toggle count, the ISR is the only writer. */
static volatile uint16_t g_toggle_count = 0;

/**
 * @brief Toggle RB4 on every Timer0 overflow (the weak-ISR dispatch).
 *
 * The pin toggles through a direct latch RMW, not EPIC_GPIO_TogglePin:
 * this family's small parts cannot afford the driver call path in the
 * ISR partition.
 */
static void on_t0_overflow(void)
{
    EPIC_REG8(PIC_REG_PORTB) ^= GPIO_PIN_4;
    g_toggle_count++;
}

/**
 * @brief Blink RB4 via Timer0 overflow interrupts and refresh the WDT.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);

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
