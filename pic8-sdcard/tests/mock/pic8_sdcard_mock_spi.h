/**
 * @file    pic8_sdcard_mock_spi.h
 * @brief   Test-only mock SD-over-SPI card: plays the card's side of the
 *          protocol well enough that the REAL vendored mmc.c logic (not a
 *          hand-written stand-in for it) can be host-tested directly.
 *
 * @details
 *   Implements the command sequence real init/read/write/ready issue
 *   (CMD0/CMD8/CMD55+ACMD41/CMD58/CMD9 reporting an SDHC-style card,
 *   CMD17/CMD24 backed by an in-memory block store, CMD13 post-write
 *   status). CMD25 (multi-block write) is not implemented; the public
 *   API doesn't wrap it either. Bound into mmc.c via
 *   tests/mock/mmc_config.h, never linked into the real target build.
 */

#ifndef PIC8_SDCARD_MOCK_SPI_H
#define PIC8_SDCARD_MOCK_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Blocks actually backed by real storage; the card still *reports*
 *  PIC8_SDCARD_MOCK_REPORTED_BLOCKS via CMD9, but only reads/writes
 *  within this range are meaningful. */
#define PIC8_SDCARD_MOCK_BACKING_BLOCKS  4u
#define PIC8_SDCARD_MOCK_REPORTED_BLOCKS 1024u
#define PIC8_SDCARD_MOCK_BLOCK_SIZE      512u

/** Reset the mock to a freshly-inserted, uninitialized card: clears all
 *  protocol state (idle/ACMD41 retry count/pending write phase) AND
 *  zeroes the backing store. Call before every test. */
void pic8_sdcard_mock_reset(void);

/** Direct access to a backing block's 512 bytes, for pre-seeding data
 *  before a read test or inspecting it after a write test. Returns NULL
 *  if block_addr >= PIC8_SDCARD_MOCK_BACKING_BLOCKS. */
uint8_t *pic8_sdcard_mock_block(uint32_t block_addr);

#endif /* PIC8_SDCARD_MOCK_SPI_H */
