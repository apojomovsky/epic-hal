/**
 * @file    epic_lcd_transport.h
 * @brief   HAL-dependent transport type definitions and init functions for
 *          epic_lcd. Separated from epic_lcd.h so the host test build (no HAL)
 *          only needs the ops/config types.
 *
 *          Include this header when using GPIO or SPI transports on target.
 */

#ifndef PIC8_LCD_TRANSPORT_H
#define PIC8_LCD_TRANSPORT_H

#include "epic_lcd.h"
#include "epic_hal.h"

/* ---- GPIO 4-bit transport ---- */

typedef struct {
    GPIO_TypeDef rs_port;  uint16_t rs_pin;
    GPIO_TypeDef e_port;   uint16_t e_pin;
    GPIO_TypeDef db4_port; uint16_t db4_pin;
    GPIO_TypeDef db5_port; uint16_t db5_pin;
    GPIO_TypeDef db6_port; uint16_t db6_pin;
    GPIO_TypeDef db7_port; uint16_t db7_pin;
} epic_lcd_gpio4_pins_t;

void epic_lcd_gpio4_init(epic_lcd_ops_t *ops, void **ctx,
                         const epic_lcd_gpio4_pins_t *pins);

/* ---- GPIO 8-bit transport ---- */

typedef struct {
    GPIO_TypeDef rs_port;  uint16_t rs_pin;
    GPIO_TypeDef e_port;   uint16_t e_pin;
    GPIO_TypeDef db0_port; uint16_t db0_pin;
    GPIO_TypeDef db1_port; uint16_t db1_pin;
    GPIO_TypeDef db2_port; uint16_t db2_pin;
    GPIO_TypeDef db3_port; uint16_t db3_pin;
    GPIO_TypeDef db4_port; uint16_t db4_pin;
    GPIO_TypeDef db5_port; uint16_t db5_pin;
    GPIO_TypeDef db6_port; uint16_t db6_pin;
    GPIO_TypeDef db7_port; uint16_t db7_pin;
} epic_lcd_gpio8_pins_t;

void epic_lcd_gpio8_init(epic_lcd_ops_t *ops, void **ctx,
                         const epic_lcd_gpio8_pins_t *pins);

/* ---- SPI transport (74HC595) ---- */

typedef struct {
    uint8_t rs_bit;
    uint8_t e_bit;
    uint8_t db4_bit;
    uint8_t db5_bit;
    uint8_t db6_bit;
    uint8_t db7_bit;
    uint8_t rw_bit;    /* 0xFF = not connected (tied low) */
} epic_lcd_spi_layout_t;

extern const epic_lcd_spi_layout_t PIC8_LCD_SPI_LAYOUT_COMMON;

typedef struct {
    GPIO_TypeDef cs_port;  uint16_t cs_pin;
    uint32_t fosc_hz;
    uint32_t spi_hz;
} epic_lcd_spi_config_t;

void epic_lcd_spi_init(epic_lcd_ops_t *ops, void **ctx,
                       const epic_lcd_spi_config_t *config,
                       const epic_lcd_spi_layout_t *layout);

#endif /* PIC8_LCD_TRANSPORT_H */
