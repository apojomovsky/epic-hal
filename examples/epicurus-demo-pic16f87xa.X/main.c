/*
 * Epicurus reference project, PIC16F87XA: a 1 ms tick toggles RB0,
 * the minimal build smoke test. See MPLABX.md to add Epicurus to an
 * existing project.
 */
#include <xc.h>

#include "epic_tick.h"
#include "peripherals/pic16f87xa_gpio.h"

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_tick_init(FOSC_HZ);

    uint32_t last = epic_tick_get();
    for (;;) {
        if ((epic_tick_get() - last) >= 500u) {
            last = epic_tick_get();
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
    }
}
