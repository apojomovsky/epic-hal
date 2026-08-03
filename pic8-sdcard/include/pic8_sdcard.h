/**
 * @file    pic8_sdcard.h
 * @brief   SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550,
 *          wrapping the vendored M-Stack storage driver.
 *
 * @details
 *   Thin binding layer: M-Stack's mmc_read_block/write_block already do
 *   the buffering, this module wires the MMC_SPI_ and timer macros to
 *   this repo's HAL and pic8-tick. PIC18Fxx5x-only for a RAM reason: a
 *   512-byte block exceeds every PIC16F87XA variant's total RAM. Single
 *   card only, M-Stack's multi-card array isn't wrapped here.
 */

#ifndef PIC8_SDCARD_H
#define PIC8_SDCARD_H

#include <stdint.h>
#include <stdbool.h>
#include "pic8_hal.h"   /* GPIO_TypeDef, needed by pic8_sdcard_pins_t below */

/** Card CS pin, board-specific -- SCK/SDI/SDO are fixed to the SSP
 *  peripheral's pins, but CS is ordinary GPIO wired however the board
 *  wires it. */
typedef struct {
    GPIO_TypeDef cs_port;
    uint16_t     cs_pin;
} pic8_sdcard_pins_t;

/**
 * @brief  Configure SPI mode 0,0, assert CS idle, and run the SD/MMC
 *         bring-up sequence (CMD0/CMD8/ACMD41/CMD58/CMD9). Blocks until
 *         the card responds or M-Stack's retry/timeout bounds are hit.
 * @param  pins     which GPIO pin drives the card's CS line.
 * @param  fosc_hz  system oscillator frequency in Hz, needed for the SPI
 *                  clock divisor.
 * @return true if the card initialized and read/write/num_blocks are
 *         now usable.
 */
bool pic8_sdcard_init(const pic8_sdcard_pins_t *pins, uint32_t fosc_hz);

/**
 * @brief  Re-query the card's status (SEND_STATUS-adjacent, see mmc_ready()
 *         in the vendored mmc.h). Has real SPI traffic cost -- don't call
 *         in a tight loop.
 */
bool pic8_sdcard_ready(void);

/**
 * @brief  Number of 512-byte blocks on the card, cached from init. 0 if
 *         not initialized.
 */
uint32_t pic8_sdcard_num_blocks(void);

/**
 * @brief  Read one 512-byte block.
 * @param  data  destination buffer, must be at least 512 bytes.
 * @return true on success (including CRC16 check passing).
 */
bool pic8_sdcard_read_block(uint32_t block_addr, uint8_t *data);

/**
 * @brief  Write one 512-byte block.
 * @param  data  source buffer, must be exactly 512 bytes.
 * @return true on success (card accepted the data and reported no write
 *         error via the follow-up SEND_STATUS check).
 */
bool pic8_sdcard_write_block(uint32_t block_addr, const uint8_t *data);

#endif /* PIC8_SDCARD_H */
