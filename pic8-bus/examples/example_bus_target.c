/**
 * @file    example_bus_target.c
 * @brief   pic8-bus on-target link smoke: configures an I2C master and an
 *          SPI master, proving the module links against the real HAL.
 *          Deliberately issues no MEM transaction, since the default
 *          ops' SSPIF wait would block with no device attached.
 */

#include "pic8_bus.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

int main(void)
{
    pic8_harness_init(0UL);
    pic8_bus_i2c_init(FOSC_HZ, 100000UL);        /* I2C master, 100 kHz */
    pic8_bus_spi_init(FOSC_HZ, 0UL, 1u, 0u);     /* SPI master, CS = GPIOB pin 0 */

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
    }
    return 0;
}
