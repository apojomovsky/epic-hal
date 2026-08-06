/*
 * Epicurus reference project, PIC16F193X.
 *
 * This family has no higher-level modules yet (HAL only), so this uses
 * GPIO and Timer0 directly: RB0 as output, Timer0 overflow toggles it
 * from interrupt context via the HAL's own weak-ISR dispatch (no
 * hand-written vector needed, see pic16f193x-hal's core sources). If
 * this builds and runs, your include paths, source folders, and device
 * pack are all right, and you can copy this project's settings into
 * your own.
 *
 * See MPLABX.md in this bundle for adding Epicurus to an existing
 * project instead.
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

static void on_t0_overflow(void)
{
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

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
