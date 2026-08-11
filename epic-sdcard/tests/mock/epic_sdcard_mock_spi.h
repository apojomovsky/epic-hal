/**
 * Test-only mock SD-over-SPI card: plays the card side of the protocol so
 * the REAL vendored mmc.c logic (not a hand-written stand-in) can be
 * host-tested. Implements CMD0/CMD8/CMD55+ACMD41/CMD58/CMD9 (SDHC-style),
 * CMD17/CMD24 over an in-memory block store, and CMD13 post-write status;
 * CMD25 is not implemented (the public API doesn't wrap it either). Bound
 * into mmc.c via tests/mock/mmc_config.h, never linked into the target
 * build.
 */

#ifndef EPIC_SDCARD_MOCK_SPI_H
#define EPIC_SDCARD_MOCK_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Blocks actually backed by real storage; the card still *reports*
 *  EPIC_SDCARD_MOCK_REPORTED_BLOCKS via CMD9, but only reads/writes
 *  within this range are meaningful. */
#define EPIC_SDCARD_MOCK_BACKING_BLOCKS  4u
#define EPIC_SDCARD_MOCK_REPORTED_BLOCKS 1024u
#define EPIC_SDCARD_MOCK_BLOCK_SIZE      512u

/**
 * @brief Reset the mock to a freshly-inserted, uninitialized card.
 *
 * Clears all protocol state (idle/ACMD41 retry count/pending write
 * phase) AND zeroes the backing store. Call before every test.
 */
void epic_sdcard_mock_reset(void);

/**
 * @brief Direct access to a backing block's 512 bytes.
 *
 * For pre-seeding data before a read test or inspecting it after a write
 * test.
 *
 * @param block_addr block to access
 * @return pointer to the block, or NULL if block_addr >=
 *         EPIC_SDCARD_MOCK_BACKING_BLOCKS
 */
uint8_t *epic_sdcard_mock_block(uint32_t block_addr);

#endif /* EPIC_SDCARD_MOCK_SPI_H */
