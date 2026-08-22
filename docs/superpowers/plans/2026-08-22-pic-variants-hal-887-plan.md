# HAL -- Add PIC16F887 board (file-per-device exemplar)

**Parent spec:** §5–§6<br>
**Tickets:** `epic-hal#71`<br>
**Depends on:** HAL registry (#70) + `epic-cc#85` (compiler can target p16f887)

## Goal

Exemplar that proves HAL file-per-device: one header + one `mcu/` entry + one board JSON → `blinky` on `p16f887` via `sim`.

## Steps

1. **Data** -- `generated/p16f887.h` via `gen-device.py` ATDF → header; `mcu/p16f887/` Makefile fragment.
2. **Board** -- `boards/p16f887.json` or `pic16f87xa-hal/boards/p16f887.json` with `"mcu": "p16f887"`.
3. **Smoke** -- `blinky` on `p16f887` via `epic-cc --target p16f887` → `sim` (HAL-4, #60) or `mdb` single-register read per `adding-a-device.md` §4.

## Acceptance (from #71)

- `blinky` builds and runs on `p16f887` in `sim`.
- Takes one header + one `mcu/` entry + one board JSON.
