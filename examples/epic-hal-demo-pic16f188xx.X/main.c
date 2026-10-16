/**
 * Epic HAL reference project, PIC16F18855: busy-wait blink on RA0 using
 * only the GPIO peripheral. No timer, interrupt, or WDT is used, so WDTE is
 * OFF. See MPLABX.md to add Epic HAL to an existing project.
 */
#include "pic16f188xx.h"
#include "peripherals/pic16f188xx_gpio.h"

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINT32
#pragma config CLKOUTEN = OFF
#pragma config CSWEN = ON
#pragma config FCMEN = OFF
#pragma config MCLRE = ON
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config STVREN = ON
#pragma config WDTE = OFF
#pragma config WRT = OFF
#pragma config LVP = OFF
#pragma config CP = OFF
#pragma config CPD = OFF

#define BLINK_DELAY 20000U

/**
 * @brief Toggle RA0 forever with a busy-wait delay between toggles.
 * @return Never returns.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    for (;;)
    {
        EPIC_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
        for (volatile uint16_t i = 0U; i < BLINK_DELAY; i++)
        {
        }
    }
}
