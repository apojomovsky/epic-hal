/* Real-target loopback demo: wire RC1 (TX/CCP2) to RC2 (RX/CCP1) and
 * the chip talks to itself, toggling RB1 per round-tripped byte.
 * No host-sim dependency; the manifest builds it with XC8 directly.
 * Family-agnostic via epic_hal.h's neutral GPIO contract (AGENTS.md). */
#include <xc.h>

#include "epic_swuart.h"

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, FOSC_HZ, 9600u);

    uint8_t next = 0u;
    for (;;) {
        if (EPIC_SWUART_Write(&h, &next, 1) == 1u) {
            uint8_t back;
            while (EPIC_SWUART_Read(&h, &back, 1) != 1) {
                /* wait for our own byte to loop back. WDTE/WDT is ON in
                 * this example's config table (same as every other
                 * WDTE=ON example in this repo), so refresh it every
                 * iteration: without this, a missing or miswired
                 * loopback jumper leaves the wait unbounded and the
                 * watchdog resets the chip mid-wait, over and over,
                 * instead of just waiting here as intended. */
                EPIC_WDT_Refresh();
            }
            if (back == next) {
                EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
            }
            next++;
        }
    }
}
