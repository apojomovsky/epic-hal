# epic-settings, EEPROM-backed settings blobs with CRC validation

Save an opaque caller-owned blob to data EEPROM and load it back later with a
CRC-16 integrity check.

- **Family-agnostic API**: no instance object, no device table, no hidden
  state. A settings block is just `(eeprom_addr, size, data)` passed to
  `epic_settings_save` / `load` / `load_or_default`.
- **Corruption detection**: appends a CRC-16-CCITT trailer, so load rejects a
  blank or damaged region instead of handing unchecked bytes to the caller.
- **Safe multi-byte EEPROM writes**: writes one byte at a time and waits for
  EEIF between bytes instead of calling the HAL's current `WriteBuffer`
  helper, which does not wait for the hardware write cycle.

## Quick start

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/test_settings
```

## Use it

```c
#include "epic_settings.h"

if (!epic_settings_load_or_default(0x10u, &cfg, sizeof(cfg), &defaults)) {
    /* First boot or corrupt blob: defaults were copied and persisted. */
}
```

## License

MIT, see the [repo LICENSE](../LICENSE).
