/**
 * Blink RA0 from a busy-wait loop: GPIO-only target smoke. No timer, no
 * interrupt, no WDT. The target config sets WDTE=OFF because wdt_sleep is
 * not part of this family yet.
 */
#include "pic16f1508.h"
#include "peripherals/pic16f1508_gpio.h"

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
