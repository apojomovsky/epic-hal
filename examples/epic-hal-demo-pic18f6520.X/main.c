/*
 * Epic HAL reference project, PIC18F6520: a Timer0 overflow toggles RB0
 * via the HAL's weak-ISR dispatch (HAL-only family, no hand-written
 * vector). 20 MHz HS crystal. See MPLABX.md to add Epic HAL to an
 * existing project.
 */
#include <xc.h>

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_gpio.h"
#include "peripherals/pic18f6520_timer0.h"
#include "core/pic18_irq.h"
#include "core/pic18f6520_wdt_sleep.h"

#pragma config BOR = ON
#pragma config BORV = 27
#pragma config CCP2MUX = ON
#pragma config CP0 = OFF
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config CPB = OFF
#pragma config CPD = OFF
#pragma config DEBUG = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF
#pragma config LVP = OFF
#pragma config OSC = HS
#pragma config OSCS = OFF
#pragma config PWRT = ON
#pragma config STVR = ON
#pragma config WDT = OFF
#pragma config WDTPS = 128
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF

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
