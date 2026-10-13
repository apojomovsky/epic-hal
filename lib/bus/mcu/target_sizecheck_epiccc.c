/* Mem-dispatch probe for epic-cc: I2C and SPI register round-trips
 * through the ops seam against a RAM loopback device, toggling RB0
 * while both verify. No slave exists under the simulators, so the mock
 * stands in for the MEM device; a silent pin means a NACK or a data
 * mismatch, which is what the toggle gate fails on. The inits run
 * first: each installs the default ops table, so the loopback tables
 * are installed after them. */
#include "epic_bus.h"
#include "epic_hal.h"
#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/* MEM device on the I2C bus (7-bit address). */
#define I2C_DEV_ADDR 0x50u

/* Register both buses round-trip. */
#define SCRATCH_REG  0x10u

/* RAM loopback file standing in for the MEM device. */
static uint8_t g_reg[16];

enum { I_IDLE, I_ADDR, I_REG, I_DATA };
static uint8_t g_i2c_state;
static uint8_t g_i2c_reg;

/** @brief Loopback I2C START: enter the address phase. */
static void mock_i2c_start(void)
{
    g_i2c_state = I_ADDR;
}

/** @brief Loopback I2C repeated START: re-enter the address phase. */
static void mock_i2c_repeated_start(void)
{
    g_i2c_state = I_ADDR;
}

/** @brief Loopback I2C STOP: return to idle. */
static void mock_i2c_stop(void)
{
    g_i2c_state = I_IDLE;
}

/**
 * @brief  Loopback I2C write: check the address, latch the register,
 *         then file the data bytes with auto-increment.
 * @param b the byte shifted out by the master.
 * @return 1 when the byte is accepted, 0 on a device mismatch.
 */
static int mock_i2c_write_byte(uint8_t b)
{
    if (g_i2c_state == I_ADDR)
    {
        if ((b >> 1) != I2C_DEV_ADDR)
        {
            return 0;
        }
        g_i2c_state = I_REG;
        return 1;
    }
    if (g_i2c_state == I_REG)
    {
        g_i2c_reg = b;
        g_i2c_state = I_DATA;
        return 1;
    }
    if (g_i2c_state == I_DATA)
    {
        g_reg[g_i2c_reg & 0x0Fu] = b;
        g_i2c_reg++;
        return 1;
    }
    return 0;
}

/**
 * @brief  Loopback I2C read: serve the latched register, auto-increment.
 * @param ack nonzero when the master ACKs for another byte.
 * @return the filed byte at the latched register.
 */
static uint8_t mock_i2c_read_byte(int ack)
{
    (void)ack;
    uint8_t b = g_reg[g_i2c_reg & 0x0Fu];
    g_i2c_reg++;
    return b;
}

static const epic_bus_i2c_ops_t mock_i2c = {
    mock_i2c_start, mock_i2c_repeated_start, mock_i2c_stop,
    mock_i2c_write_byte, mock_i2c_read_byte
};

enum { S_IDLE, S_REG, S_XFER };
static uint8_t g_spi_state;
static uint8_t g_spi_reg;

/** @brief Loopback SPI select: enter the register phase. */
static void mock_spi_select(void)
{
    g_spi_state = S_REG;
}

/** @brief Loopback SPI deselect: return to idle. */
static void mock_spi_deselect(void)
{
    g_spi_state = S_IDLE;
}

/**
 * @brief  Loopback SPI exchange: latch the register first, then swap
 *         each byte with the filed one at the auto-incremented address.
 * @param b the MOSI byte shifted out by the master.
 * @return the MISO byte shifted in (0xFF on the register phase).
 */
static uint8_t mock_spi_exchange(uint8_t b)
{
    if (g_spi_state == S_REG)
    {
        g_spi_reg = b;
        g_spi_state = S_XFER;
        return 0xFFu;
    }
    uint8_t out = g_reg[g_spi_reg & 0x0Fu];
    g_reg[g_spi_reg & 0x0Fu] = b;
    g_spi_reg++;
    return out;
}

static const epic_bus_spi_ops_t mock_spi = {
    mock_spi_select, mock_spi_deselect, mock_spi_exchange
};

/**
 * @brief  Round-trip SCRATCH_REG over I2C through the mem dispatch.
 * @return nonzero when the write and read both return 2 and match.
 */
static uint8_t i2c_round_trip(void)
{
    static const uint8_t pattern[2] = { 0x5Au, 0xA5u };
    uint8_t rd[2] = { 0u, 0u };

    if (epic_bus_i2c_mem_write(I2C_DEV_ADDR, SCRATCH_REG, pattern, 2) != 2)
    {
        return 0u;
    }
    if (epic_bus_i2c_mem_read(I2C_DEV_ADDR, SCRATCH_REG, rd, 2) != 2)
    {
        return 0u;
    }
    return (uint8_t)(rd[0] == pattern[0] && rd[1] == pattern[1]);
}

/**
 * @brief  Round-trip SCRATCH_REG over SPI through the mem dispatch.
 * @return nonzero when the write and read both return 2 and match.
 */
static uint8_t spi_round_trip(void)
{
    static const uint8_t pattern[2] = { 0x3Cu, 0xC3u };
    uint8_t rd[2] = { 0u, 0u };

    if (epic_bus_spi_mem_write(SCRATCH_REG, pattern, 2) != 2)
    {
        return 0u;
    }
    if (epic_bus_spi_mem_read(SCRATCH_REG, rd, 2) != 2)
    {
        return 0u;
    }
    return (uint8_t)(rd[0] == pattern[0] && rd[1] == pattern[1]);
}

/** @brief Main: verify both round-trips in a loop, toggling RB0. @return 0. */
int main(void)
{
    /* ADCON1 at 0xFC1 is PIC18-only. RB0 is analog at reset (PCFG=0000, DS39632E
     * Register 21-2), so PORTB reads 0 even while LATB drives it. */
#if defined(PIC18F4550)
    (*(volatile uint8_t *)0xFC1u) = 0x0Fu;
#endif
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_bus_i2c_init(FOSC_HZ, 100000UL);
    epic_bus_spi_init(FOSC_HZ, 0u, 1u, 0u);
    epic_bus_set_i2c_ops(&mock_i2c);
    epic_bus_set_spi_ops(&mock_spi);
    for (;;)
    {
        uint8_t ok = i2c_round_trip();
        ok = (uint8_t)(ok && spi_round_trip());
        if (ok)
        {
            EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
            EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        }
    }
}
