# epic-settings API

## Public header

```c
#include "epic_settings.h"
```

The public header depends only on `<stdint.h>` and `<stdbool.h>`.

## Functions

### `bool epic_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size);`

Writes `size` data bytes starting at `eeprom_addr`, then writes a trailing
CRC-16-CCITT. Returns `true` only if every byte write succeeded.

The caller must ensure `eeprom_addr + size + 2` fits in the target's data
EEPROM. This module deliberately does not ship a per-device EEPROM-capacity
table because its public API is family-agnostic and the HAL already exposes
only an 8-bit EEPROM address.

### `bool epic_settings_load(uint8_t eeprom_addr, void *data, uint8_t size);`

Reads `size` bytes and the trailing CRC from EEPROM, validates them, and copies
the bytes into `data` only on success.

- Returns `true` when the CRC matches.
- Returns `false` for both blank and corrupt regions.
- Leaves `data` untouched on failure.

### `bool epic_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size, const void *default_data);`

Calls `epic_settings_load`. If the stored region is invalid, copies
`default_data` into `data` and then attempts to persist that default blob to
EEPROM so the next boot sees it as valid.

- Returns `true` when an existing stored blob was valid.
- Returns `false` when defaults had to be applied.
- The first invalid load therefore performs a real EEPROM write; account for
  that in any wear-sensitive startup path.
