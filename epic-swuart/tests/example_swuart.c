/**
 * @file    example_swuart.c
 * @brief   Real-target loopback demo. Wire RB0 (TX) to RB2 (RX) with a
 *          jumper: the chip talks to itself, toggling RB1 on every
 *          successfully round-tripped byte so a scope or LED on RB1
 *          shows it working without needing a second UART to watch.
 *
 * See MANUAL.md / docs/API.md for the real function contracts; this
 * file has no host-sim dependency, the manifest builds it with XC8
 * directly.
 *
 * Family-agnostic on purpose: `epic_swuart.h` pulls in `epic_hal.h`,
 * each family's own neutral entry point, which already brings in that
 * family's GPIO header (`peripherals/pic16f87xa_gpio.h`,
 * `pic18fxx5x_gpio.h`, or `pic16f193x_gpio.h`). GPIO_TypeDef, GPIO_PIN_n
 * and EPIC_GPIO_* are the same names and signatures in all three per
 * the fixed contract (see AGENTS.md), so this file needs no
 * family-specific include of its own and the manifest builds it
 * unchanged for all three families' example sections.
 */
#include <xc.h>

#include "epic_swuart.h"

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2, FOSC_HZ, 9600u);

    uint8_t next = 0u;
    for (;;) {
        if (EPIC_SWUART_Write(&h, &next, 1) == 1u) {
            uint8_t back;
            while (EPIC_SWUART_Read(&h, &back, 1) != 1) {
                /* wait for our own byte to loop back */
            }
            if (back == next) {
                EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
            }
            next++;
        }
    }
}
