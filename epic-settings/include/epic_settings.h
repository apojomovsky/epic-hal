/**
 * @file    epic_settings.h
 * @brief   EEPROM-backed settings blobs with CRC-16 validation.
 *
 * @details
 *   Saves an opaque caller-owned blob to EEPROM with a trailing
 *   CRC-16-CCITT for corruption/blank detection. No instance object or
 *   hidden state: a block is just (eeprom_addr, size, data) passed
 *   explicitly, so one firmware can keep several independent blobs.
 *   "Never written" and "corrupt" are not distinguished on load
 *   failure, both mean apply defaults.
 */

#ifndef EPIC_SETTINGS_H
#define EPIC_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Save @p size bytes from @p data to EEPROM at @p eeprom_addr,
 *         followed by a trailing CRC-16-CCITT.
 *
 * @param  eeprom_addr  First EEPROM byte to write.
 * @param  data         Caller-owned blob to persist.
 * @param  size         Number of payload bytes in @p data.
 *
 * @return true if every payload byte and both CRC bytes wrote successfully;
 *         false if the underlying EEPROM driver rejected any byte write.
 *
 * @note   The caller must ensure `eeprom_addr + size + 2` stays within the
 *         target device's EEPROM size. This module is intentionally family-
 *         agnostic and does not carry a device-capacity table.
 */
bool epic_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size);

/**
 * @brief  Load @p size bytes from EEPROM at @p eeprom_addr into @p data,
 *         validating the trailing CRC-16-CCITT first.
 *
 * @param  eeprom_addr  First EEPROM byte of the stored blob.
 * @param  data         Destination buffer to fill on success.
 * @param  size         Number of payload bytes expected.
 *
 * @return true if the stored blob was valid and copied into @p data; false
 *         if the region was blank/corrupt, in which case @p data is left
 *         untouched.
 */
bool epic_settings_load(uint8_t eeprom_addr, void *data, uint8_t size);

/**
 * @brief  Convenience wrapper: load if valid, otherwise copy
 *         @p default_data into @p data and persist it to EEPROM.
 *
 * @param  eeprom_addr   First EEPROM byte of the stored blob.
 * @param  data          Destination buffer for either the stored blob or the
 *                       applied defaults.
 * @param  size          Number of payload bytes.
 * @param  default_data  Caller-owned default blob of @p size bytes.
 *
 * @return true if a valid stored blob already existed; false if defaults had
 *         to be applied.
 *
 * @note   On the first invalid load this performs a real EEPROM write so the
 *         next boot sees the default blob as already valid.
 */
bool epic_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size,
                                   const void *default_data);

#endif /* EPIC_SETTINGS_H */
