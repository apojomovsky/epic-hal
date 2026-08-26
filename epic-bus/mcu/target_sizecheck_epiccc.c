/* Pure footprint probe for epic-cc: init both buses, toggle a pin.
 * The mem transactions are stubbed under __EPIC_CC__ (iselcore gep gap
 * on the ops chain), so this links the module and reports its footprint
 * without the dispatch. XC8 keeps the full example. */
#include "epic_bus.h"
#include "epic_hal.h"
#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/** @brief Main. @return 0. */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_bus_i2c_init(FOSC_HZ, 100000UL);
    epic_bus_spi_init(FOSC_HZ, 0u, 1u, 0u);
    for (;;) {
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    }
}
