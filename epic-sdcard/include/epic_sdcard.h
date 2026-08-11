/**
 * SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550, wrapping the
 * vendored M-Stack driver: wires its MMC_SPI_ and timer macros to this
 * repo's HAL and epic-tick. PIC18-only because a 512-byte block exceeds
 * every PIC16F87XA variant's total RAM; single card only (M-Stack's
 * multi-card array is not wrapped).
 */

#ifndef EPIC_SDCARD_H
#define EPIC_SDCARD_H

#include <stdint.h>
#include <stdbool.h>
#include "epic_hal.h"   /* GPIO_TypeDef, needed by epic_sdcard_pins_t below */

/** Card CS pin, board-specific: SCK/SDI/SDO are fixed to the SSP
 *  peripheral's pins, but CS is ordinary GPIO wired however the board
 *  wires it. */
typedef struct {
    GPIO_TypeDef cs_port;
    uint16_t     cs_pin;
} epic_sdcard_pins_t;

/**
 * @brief Configure SPI mode 0,0, assert CS idle, and run the SD/MMC
 *        bring-up sequence (CMD0/CMD8/ACMD41/CMD58/CMD9).
 *
 * Blocks until the card responds or M-Stack's retry/timeout bounds are
 * hit. `fosc_hz` is the system oscillator frequency, needed for the SPI
 * clock divisor.
 *
 * @param pins     CS pin assignment
 * @param fosc_hz  system oscillator frequency in Hz
 * @return true when read/write/num_blocks are usable
 */
bool epic_sdcard_init(const epic_sdcard_pins_t *pins, uint32_t fosc_hz);

/**
 * @brief Re-query the card's status.
 *
 * SEND_STATUS-adjacent, see mmc_ready() in the vendored mmc.h. Has real
 * SPI traffic cost; don't call in a tight loop.
 *
 * @return true when the card reports ready
 */
bool epic_sdcard_ready(void);

/**
 * @brief Return the number of 512-byte blocks on the card.
 *
 * Cached from init.
 *
 * @return block count, or 0 if not initialized
 */
uint32_t epic_sdcard_num_blocks(void);

/**
 * @brief Read one 512-byte block into data.
 *
 * data must be at least 512 bytes.
 *
 * @param block_addr block address to read
 * @param data       destination buffer (>= 512 bytes)
 * @return true on success, including the CRC16 check passing
 */
bool epic_sdcard_read_block(uint32_t block_addr, uint8_t *data);

/**
 * @brief Write one 512-byte block from data.
 *
 * data must be exactly 512 bytes.
 *
 * @param block_addr block address to write
 * @param data       source buffer (exactly 512 bytes)
 * @return true if the card accepted the data and reported no write error
 *         via the follow-up SEND_STATUS check
 */
bool epic_sdcard_write_block(uint32_t block_addr, const uint8_t *data);

#endif /* EPIC_SDCARD_H */
