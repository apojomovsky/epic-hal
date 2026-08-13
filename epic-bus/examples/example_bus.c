/*
 * epic-bus target example: register read/write round-trip to a MEM
 * device on the I2C bus, then the same round-trip over SPI. Both
 * phases run against the module's default (HAL) ops, so a real device
 * on the bus sees the transaction shapes documented in docs/API.md.
 * The LED on GPIOB1 shows the result: on when both round-trips
 * verified, off when a device NACKed or the data did not match.
 */

#include "epic_bus.h"
#include "epic_hal.h"                /* EPIC_GPIO_Init / EPIC_GPIO_WritePin */

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/* MEM device on the I2C bus (7-bit address), 100 kHz SCL. */
#define I2C_DEV_ADDR 0x50u
#define I2C_SPEED_HZ 100000UL

/* SPI chip-select on GPIOB0. */
#define SPI_CS_PORT  1u
#define SPI_CS_PIN   0u

/* Register both buses round-trip. */
#define SCRATCH_REG  0x10u

/* Result LED on GPIOB1. */
#define LED_PORT     GPIOB
#define LED_PIN      GPIO_PIN_1

/** @brief Pace the demo so the LED result stays visible. */
static void demo_delay(void)
{
    volatile uint32_t i;
    for (i = 0u; i < 200000u; i++) {
    }
}

/** @brief Round-trip SCRATCH_REG over I2C against the HAL ops. */
static uint8_t i2c_round_trip(void)
{
    static const uint8_t pattern[2] = { 0x5Au, 0xA5u };
    uint8_t rd[2] = { 0u, 0u };

    if (epic_bus_i2c_mem_write(I2C_DEV_ADDR, SCRATCH_REG, pattern, 2) != 2) {
        return 0u;                  /* address/register NACKed: no device */
    }
    if (epic_bus_i2c_mem_read(I2C_DEV_ADDR, SCRATCH_REG, rd, 2) != 2) {
        return 0u;
    }
    return (uint8_t)(rd[0] == pattern[0] && rd[1] == pattern[1]);
}

/** @brief Round-trip SCRATCH_REG over SPI against the HAL ops. */
static uint8_t spi_round_trip(void)
{
    static const uint8_t pattern[2] = { 0x3Cu, 0xC3u };
    uint8_t rd[2] = { 0u, 0u };

    if (epic_bus_spi_mem_write(SCRATCH_REG, pattern, 2) != 2) {
        return 0u;
    }
    if (epic_bus_spi_mem_read(SCRATCH_REG, rd, 2) != 2) {
        return 0u;
    }
    return (uint8_t)(rd[0] == pattern[0] && rd[1] == pattern[1]);
}

/** @brief Register read/write round-trip over I2C and SPI. */
int main(void)
{
    EPIC_GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    for (;;) {
        /* The MSSP is one peripheral: re-init per bus before use. */
        epic_bus_i2c_init(FOSC_HZ, I2C_SPEED_HZ);
        uint8_t ok = i2c_round_trip();

        epic_bus_spi_init(FOSC_HZ, 0UL, SPI_CS_PORT, SPI_CS_PIN);
        ok = (uint8_t)(ok && spi_round_trip());

        EPIC_GPIO_WritePin(LED_PORT, LED_PIN,
                           ok ? GPIO_PIN_SET : GPIO_PIN_RESET);
        demo_delay();
    }
}
