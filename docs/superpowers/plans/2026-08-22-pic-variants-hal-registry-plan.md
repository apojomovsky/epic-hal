# HAL -- Device registry + SFR header generation + per-core canonical CI

**Parent spec:** `docs/superpowers/specs/2026-08-22-pic-variants-design.md` (HAL)<br>
**Tickets:** `epic-hal#70` (primary), blocks `#71`<br>
**Depends on:** `epic-cc#83` (TOML `sfrs` contract)

## Goal

HAL side of "file per device": SFR headers become generated from ATDF/TOML, build selects via `-mcu <name>`, CI is canonical per family + lightweight per changed device.

## Steps

1. **Generator wiring** -- `scripts/gen-device.py` (shared or HAL wrapper) already in `cc#86` design; HAL reuses it to emit `pic16f87xa-hal/include/generated/<name>.h` (and `pic18fxx5x-hal/...`). Add `generated/` to `.gitignore` for ATDF itself, not for headers.
2. **`epic_build.py`/`epic-hal.mk`** -- `HAL-2 (#58)` already drives `epic-cc --target <name>`; add `-mcu <name>` → `generated/<name>.h` include path resolution.
3. **CI split** -- `ci.yml` → `host` + one `family-check.yml` per family (canonical `p16f877a`/`p18f4550`) stays; new `devices-changed` step detects `generated/<name>.h` or `mcu/<name>/` diff, runs single `blinky` smoke via `sim`/`mdb` on that `mcu`. Nightly lightweight for all boards.
4. **Docs** -- update `docs/adding-a-device.md` to codify generated path; add HAL spec (§3) as the decision record.

## Acceptance (from #70)

- Adding a HAL device is `generated/<name>.h` + `mcu/<name>/` + board JSON, no hand-edited `EQU`s.
- `epic_build.py --toolchain epic-cc --mcu p16f877a blinky` builds `pic16f87xa-hal` without Microchip pack.
- PR touching only `pic16f88x-hal` does not rebuild every `pic16f87xa-hal` peripheral matrix entry.
