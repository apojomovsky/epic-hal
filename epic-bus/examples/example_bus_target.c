/* epic-bus on-target link smoke: configures an I2C master and an SPI
 * master, proving the module links against the real HAL. Issues no MEM
 * transaction: the default ops' SSPIF wait would block with no device
 * attached. */

#include "epic_bus.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/** @brief On-target link smoke: configure I2C and SPI masters. */
int main(void)
{
    epic_harness_init(0UL);
    epic_bus_i2c_init(FOSC_HZ, 100000UL);        /* I2C master, 100 kHz */
    epic_bus_spi_init(FOSC_HZ, 0UL, 1u, 0u);     /* SPI master, CS = GPIOB pin 0 */

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return 0;
}
