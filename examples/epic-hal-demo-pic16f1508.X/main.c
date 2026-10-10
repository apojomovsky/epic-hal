/**
 * Epic HAL reference project, PIC16F1508/1509: busy-wait blink on RA0 using
 * only the GPIO peripheral. No timer, interrupt, or WDT is used, so WDTE is
 * OFF. See MPLABX.md to add Epic HAL to an existing project.
 */
#include "pic16f1508.h"
#include "peripherals/pic16f1508_gpio.h"

#pragma config FOSC = INTOSC
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config BOREN = ON
#pragma config CLKOUTEN = OFF
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config LVP = OFF
#pragma config STVREN = ON
#pragma config WRT = OFF

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
