# epic-settings architecture

## Model

`epic-settings` stores a caller-owned blob at a caller-chosen EEPROM address,
then appends a two-byte CRC-16-CCITT trailer. There is no module instance and
no registry of blocks. A firmware that needs several settings regions simply
passes different `(eeprom_addr, size)` pairs on different calls.

`epic_settings_load` intentionally treats "never written" and "corrupt" as the
same outcome. The module cannot recover useful meaning from either, and the
caller's normal response is the same in both cases: apply defaults.

## CRC format

The CRC is the standard CCITT polynomial `0x1021`, computed byte-at-a-time with
no lookup table. The initial CRC value is `0xFFFF`; the stored trailer is
big-endian: high byte first, low byte second.

The implementation does not allocate a temporary buffer on load. It first reads
the EEPROM bytes only to compute the CRC, compares that against the stored
trailer, and only then copies the data bytes into the caller's buffer. This is
how `epic_settings_load` keeps `data` untouched on failure without heap use or
stack-sized VLAs.

## Why it does not call `EPIC_EEPROM_WriteBuffer`

The underlying HAL's `EPIC_EEPROM_WriteByte` starts a hardware write cycle and
returns immediately. Completion is reported separately through EEIF and
`EPIC_EEPROM_IsWriteComplete()`. Today `EPIC_EEPROM_WriteBuffer` just loops over
`EPIC_EEPROM_WriteByte` without waiting between bytes, which is unsafe on real
silicon because the next byte can be launched while the prior write cycle is
still active.

`epic-settings` therefore performs its own byte-at-a-time sequence:

1. Call `EPIC_EEPROM_WriteByte(addr, data)`.
2. Wait until `EPIC_EEPROM_IsWriteComplete()` reports done.
3. Call `EPIC_EEPROM_ClearITFlag()`.
4. Move to the next byte.

That wait/clear sequence is internal to this module because it is part of the
safe multi-byte write policy, not part of the public settings API.

## Host simulator behavior

On target hardware the EEPROM write completes asynchronously in real time. On
the host simulator there is no timed EEPROM write model; completion is exposed
only through the simulator helper that marks a byte done and raises EEIF.

To keep the same blocking `epic_settings_save` implementation working on host
and target, `epic-settings` drives each simulated byte write to completion
immediately after starting it, then follows the same EEIF poll/clear contract
as on silicon. The tests still exercise the same saved bytes, CRC checks, and
corruption cases the target code depends on.
