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

/** Save @p size bytes from @p data to EEPROM at @p eeprom_addr, followed
 *  by a trailing CRC-16-CCITT. Returns true if every payload byte and both
 *  CRC bytes wrote; false if the EEPROM driver rejected any byte.
 *
 *  The caller must keep `eeprom_addr + size + 2` within the device's
 *  EEPROM: the module is intentionally family-agnostic and carries no
 *  capacity table. */
bool epic_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size);

/** Load @p size bytes from EEPROM at @p eeprom_addr into @p data,
 *  validating the trailing CRC-16-CCITT first. Returns true if the stored
 *  blob was valid and copied; false if the region was blank/corrupt, in
 *  which case @p data is left untouched. */
bool epic_settings_load(uint8_t eeprom_addr, void *data, uint8_t size);

/** Load if valid, otherwise copy @p default_data into @p data and persist
 *  it to EEPROM so the next boot sees it as already valid. Returns true if
 *  a valid stored blob existed, false if defaults were applied. */
bool epic_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size,
                                   const void *default_data);

#endif /* EPIC_SETTINGS_H */
