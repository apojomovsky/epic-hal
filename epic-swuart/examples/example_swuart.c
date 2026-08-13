/*
 * epic-swuart on-target demo: a software UART echo on channel A's
 * CCP-wired pins (TX = RC1 / CCP2 compare, RX = RC2 / CCP1 capture,
 * identical on every supported family). Connect a serial terminal at
 * 9600 baud 8N1: a banner goes out on startup, then every received
 * byte is echoed back. RB1 toggles per echoed byte as a liveness
 * indicator.
 */

#include "epic_hal.h"
#include "epic_swuart.h"

#define BAUD_RATE 9600u

/**
 * @brief Echo bytes received on the software UART back out its TX pin.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

    EPIC_SWUART_HandleTypeDef h;
    if (EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                         FOSC_HZ, BAUD_RATE) != EPIC_OK) {
        /* The pin pair does not match this slot's fixed CCP pins. */
        for (;;) {
            EPIC_WDT_Refresh();
        }
    }

    static const uint8_t banner[] = "epic-swuart echo ready\r\n";
    for (size_t i = 0u; i < sizeof(banner) - 1u; i++) {
        while (EPIC_SWUART_Write(&h, &banner[i], 1u) != 1u) {
            /* TX ring full; the CCP-compare ISR drains it. */
        }
    }

    uint8_t buf[4];
    for (;;) {
        int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
        if (n > 0) {
            EPIC_SWUART_Write(&h, buf, (size_t)n);
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
        }
        EPIC_WDT_Refresh();
    }
}
