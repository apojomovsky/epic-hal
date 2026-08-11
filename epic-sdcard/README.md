# epic-sdcard, SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550

A thin block-storage driver wrapping the vendored M-Stack `mmc.c`/`crc.c`
SD-over-SPI driver (`third_party/m-stack-storage`), bound to this repo's
SSP/GPIO HAL and `epic-tick`.

- **Thin by design**: unlike `epic-usb`, M-Stack's `mmc_read_block`/
  `mmc_write_block` are already the right shape — this module's real job is
  binding `MMC_SPI_TRANSFER`/`MMC_SPI_SET_CS`/`MMC_SPI_SET_SPEED` to
  `EPIC_SSP_*` and `EPIC_GPIO_WritePin`, and the timer macros to `epic-tick`,
  not inventing new buffering.
- **PIC18Fxx5x-only for a RAM reason**: `MMC_BLOCK_SIZE` is a fixed 512
  bytes; every PIC16F87XA family member's total RAM (192–368 B) is smaller
  than one block. The SSP driver itself is portable across both families —
  the blocker is memory, not the peripheral.
- **CS pin is caller-supplied**: SCK/SDI/SDO are fixed to the SSP
  peripheral's pins, but CS is ordinary GPIO wired however the board wires
  it. Pass it at init time, same as `epic-debounce`'s read callback and
  `epic-adcfilter`'s read callback.
- **Host tests exercise the real protocol logic**: the mock SPI slave
  (`tests/mock/epic_sdcard_mock_spi.c`) plays the SD card's side of the
  command/response protocol well enough that the **actual vendored
  `mmc.c`/`crc.c`** are compiled and tested directly — not a hand-written
  stand-in for them, unlike `epic-usb`'s host stub.
- **Bring-up clock gap**: at 48 MHz the SSP's slowest fixed divisor is
  750 kHz, above the SD spec's 400 kHz bring-up ceiling; many cards
  tolerate it, unverified on real hardware.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.
- [Implementation plan](../docs/epic-sdcard-plan.md), M-Stack storage
  vendoring, chip scope rationale, Phase 2 XC8 findings, open risks.

## Quick start

### Host simulator (the test)

```sh
cmake -B build -S epic-sdcard && cmake --build build
ctest --test-dir build --output-on-failure
```

### Real target (XC8)

Real-silicon bring-up is Phase 3 — the `mcu/pic18fxx5x-sdcard-mplabx/Makefile`
does not exist yet. Phase 2 confirmed that `mmc.c` + `crc.c` compile and
link clean under `xc8-cc` against real PIC18F4550 headers: 3924 bytes flash
(12.0%), 919 bytes RAM (44.9% of 2048). No PIC16 `mcu/` directory will be
added — RAM is the blocker, not the peripheral.

## Use it

```c
#include "epic_sdcard.h"
epic_sdcard_pins_t pins = { .cs_port = GPIOC, .cs_pin = 6 };
if (!epic_sdcard_init(&pins, 48000000UL)) {
    /* init failed — no card, or card didn't respond */
}

uint8_t block[512];
if (epic_sdcard_read_block(0, block)) {
    /* block 0 read successfully */
}
```

## Third-party license

`third_party/m-stack-storage/` vendors the storage subset of
[M-Stack](https://github.com/signal11/m-stack) (same pinned upstream commit
as `epic-usb`) under the Apache-2.0 arm of its dual LGPLv3 / Apache-2.0
license. Remove it from your fork if you do not want to redistribute
third-party code.

## License

MIT, see the [repo LICENSE](../LICENSE).
