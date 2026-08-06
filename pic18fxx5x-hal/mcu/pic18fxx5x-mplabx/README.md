# PIC18Fxx5x HAL, XC8 real-target build

This directory's `Makefile` (a standalone HAL-only smoke build) is
gone; the manifest-driven build replaces it and has no direct
equivalent, since PIC18Fxx5x's own modules (e.g. `epic-tick`) already
compile the family's full HAL and are what CI actually exercises:

```sh
python3 scripts/epic_build.py build --module epic-tick --mcu 18F4550 --run
```

Also 18F2455/2550/4455. See `epic-common/manifest/README.md` for the
full module list.

## One-time setup: install the PIC18Fxxxx DFP

XC8 v3.x moved device support into Device Family Packs. The PIC18Fxxxx
DFP is **not bundled with XC8** (unlike the PIC16Fxxx DFP) and must be
installed once, or the build fails with
`error: (2104) no device-support files found`:

- **MPLAB X**: *Tools → Packs → Pack Manager*, search `PIC18Fxxxx_DFP`
  and install the latest version.
- **Manual**: download `Microchip.PIC18Fxxxx_DFP.<ver>.atpack` from
  [packs.download.microchip.com](https://packs.download.microchip.com/)
  and unzip it into
  `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`.
  `epic_build.py build --dfp-dir <dir>` points at the `xc8/` subdir
  there; the root `Makefile`'s `xc8-build`/`mdb-test` targets resolve
  this from the manifest automatically.

## What works on real silicon

The HAL compiles on XC8 because:

- The SFR macros resolve to volatile direct-access on real targets via
  `include/target/pic18_platform.h` (`include/target` precedes
  `include` on the include path, see the manifest's own
  `families.PIC18Fxx5x.includes`).
- `pic18_sim.c` and the host-side harness implementation are not in a
  real-target build's source list, XC8 never links them.
- The family-blind harness target impl (`epic_harness_target.c`, from
  `epic-common/`) and the shared interrupt dispatch
  (`pic18_irq_dispatch.c`) link here.

## Adjusting for your board

`epic-common/manifest/modules.toml`'s per-module `example.PIC18Fxx5x`
tables carry the config words that used to live in this directory's
`Makefile`:

- `FOSC = HS`, `PLLDIV = 5`, `CPUDIV = OSC1_PLL2`, `USBDIV = 2`
  (20 MHz crystal, 48 MHz CPU/USB via the PLL)
- `WDT = ON`, `WDTPS = 32768` (refresh via `EPIC_WDT_Refresh()`)
- `PWRT = ON`, `MCLRE = ON`, `LVP = OFF`, `XINST = OFF`, `DEBUG = OFF`
- Code / write / read protection all `OFF`

The full Configuration Word layout is DS39632E §23.1. The block-3
protection settings (`CP3` / `WRT3` / `EBTR3`) are emitted only for the
32 KB parts (18F2550 / 18F4550); the 24 KB parts have no block 3.
