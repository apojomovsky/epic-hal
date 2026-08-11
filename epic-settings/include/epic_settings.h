/**
 * EEPROM-backed settings blobs with CRC-16-CCITT validation. A blob is
 * (eeprom_addr, size, data) passed explicitly, so no instance object and
 * one firmware can keep several independent blobs. "Never written" and
 * "corrupt" are not distinguished on load failure; both mean apply
 * defaults.
 */

#ifndef EPIC_SETTINGS_H
#define EPIC_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Save size bytes from data to EEPROM at eeprom_addr, followed by
 *        a trailing CRC-16-CCITT.
 *
 * Returns true if every payload byte and both CRC bytes wrote; false if
 * the EEPROM driver rejected any byte. The caller must keep
 * `eeprom_addr + size + 2` within the device's EEPROM: the module is
 * intentionally family-agnostic and carries no capacity table.
 *
 * @param eeprom_addr  EEPROM address of the blob
 * @param data         payload to save
 * @param size         payload size in bytes
 * @return true if the payload and CRC trailer were written
 */
bool epic_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size);

/**
 * @brief Load size bytes from EEPROM at eeprom_addr into data, validating
 *        the trailing CRC-16-CCITT first.
 *
 * Returns true if the stored blob was valid and copied; false if the
 * region was blank/corrupt, in which case data is left untouched.
 *
 * @param eeprom_addr  EEPROM address of the blob
 * @param data         buffer to receive the payload
 * @param size         payload size in bytes
 * @return true if the stored blob was valid and copied
 */
bool epic_settings_load(uint8_t eeprom_addr, void *data, uint8_t size);

/**
 * @brief Load if valid, otherwise copy default_data into data and persist
 *        it to EEPROM so the next boot sees it as already valid.
 *
 * @param eeprom_addr   EEPROM address of the blob
 * @param data          buffer to receive the payload
 * @param size          payload size in bytes
 * @param default_data  fallback payload, applied and saved on load failure
 * @return true if a valid stored blob existed, false if defaults were
 *         applied
 */
bool epic_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size,
                                   const void *default_data);

#endif /* EPIC_SETTINGS_H */
