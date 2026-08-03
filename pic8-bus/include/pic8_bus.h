/**
 * @file    pic8_bus.h
 * @brief   Family-agnostic I2C/SPI "MEM" register-access idiom, the
 *          `HAL_I2C_Mem_Read`/`Mem_Write` layer Cube sensor code uses, for
 *          8-bit PICs. Sits on the HAL's MSSP/SSP driver through a small
 *          injectable "bus ops" interface, which is also how the host
 *          test wires in a mock device (the host sim has no SSP slave
 *          model). See docs/ARCHITECTURE.md for the transaction shapes.
 */

#ifndef PIC8_BUS_H
#define PIC8_BUS_H

#include <stdint.h>

/* ---- I2C ---- */

/** I2C bus operations (the MEM logic calls these). The default implementation
 *  wraps the HAL SSP driver; inject your own (e.g. a mock) for testing. */
typedef struct {
    void    (*start)(void);
    void    (*repeated_start)(void);
    void    (*stop)(void);
    int     (*write_byte)(uint8_t b);   /**< returns 1 if slave ACKed, 0 if NACK */
    uint8_t (*read_byte)(int ack);      /**< reads one byte; ack=1 sends ACK, 0 sends NACK */
} pic8_bus_i2c_ops_t;

/** Configure the MSSP as an I2C master at @p fscl_hz (from @p fosc_hz) and
 *  use the default (HAL) I2C ops. Call once before the mem_read/write. */
void pic8_bus_i2c_init(uint32_t fosc_hz, uint32_t fscl_hz);

/** Use a custom I2C ops table (NULL restores the default HAL ops). */
void pic8_bus_set_i2c_ops(const pic8_bus_i2c_ops_t *ops);

/** Write @p n bytes to register @p reg on I2C device @p dev (7-bit address).
 *  Transaction: START, (dev<<1)|W, reg, data..., STOP. Returns n on success
 *  (all ACKed), or -1 if the device NACKed the address or register. */
int pic8_bus_i2c_mem_write(uint8_t dev, uint8_t reg, const uint8_t *data, int n);

/** Read @p n bytes from register @p reg on I2C device @p dev (7-bit address).
 *  Transaction: START, (dev<<1)|W, reg, REPEATED-START, (dev<<1)|R,
 *  read n-1 with ACK, read last with NACK, STOP. Returns n on success, -1 on
 *  address/register NACK. */
int pic8_bus_i2c_mem_read(uint8_t dev, uint8_t reg, uint8_t *buf, int n);

/* ---- SPI ---- */

/** SPI bus operations. The default wraps the HAL SSP driver + a GPIO CS. */
typedef struct {
    void    (*select)(void);
    void    (*deselect)(void);
    uint8_t (*exchange)(uint8_t b);     /**< write MOSI byte, return MISO byte shifted in */
} pic8_bus_spi_ops_t;

/** Configure the MSSP as an SPI master and use @p cs_port/@p cs_pin (GPIO) as
 *  the chip-select (asserted low). Uses the default (HAL) SPI ops. */
void pic8_bus_spi_init(uint32_t fosc_hz, uint32_t f_sclk_hz, uint8_t cs_port, uint8_t cs_pin);

/** Use a custom SPI ops table (NULL restores the default HAL ops). */
void pic8_bus_set_spi_ops(const pic8_bus_spi_ops_t *ops);

/** Write @p n bytes to register @p reg over SPI: CS low, exchange(reg),
 *  exchange(data[0..n-1]), CS high. Returns n. */
int pic8_bus_spi_mem_write(uint8_t reg, const uint8_t *data, int n);

/** Read @p n bytes from register @p reg over SPI: CS low, exchange(reg),
 *  exchange(0) x n (capturing MISO), CS high. Returns n. */
int pic8_bus_spi_mem_read(uint8_t reg, uint8_t *buf, int n);

#endif /* PIC8_BUS_H */
