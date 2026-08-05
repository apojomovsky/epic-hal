# MPLAB X / XC8 project template (PIC18F2455 family)

This directory holds a minimal MPLAB X IDE / XC8 Makefile project that
builds the PIC18F2455 family HAL for a real PIC18F4550 target (also
2455 / 2550 / 4455).

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
  `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`. The
  Makefile's `DFP_DIR` defaults to the `xc8/` subdir there; override with
  `make DFP_DIR=/your/dfp/xc8` if you place it elsewhere. Pass
  `make DFP_DIR=` (empty) if your XC8 resolves PIC18 devices without a
  DFP.

## What works on real silicon (Phase 1)

The skeleton compiles on XC8 because:

- The SFR macros resolve to volatile direct-access on real targets via
  `include/target/pic18_platform.h` (the Makefile puts `include/target`
  ahead of `include` on the include path).
- `pic18_sim.c` and the host-side harness implementation are not in this
  build's source list, XC8 never links them.
- The family-blind harness target impl (`pic8_harness_target.c`, from
  `pic8-common/`) and the shared interrupt dispatch
  (`pic18_irq_dispatch.c`, empty body in Phase 1) link here.

Phase 1 produces a `.hex` with no link errors, proving the shared
`pic8_family.mk` fragment and the config-word generation plumbing work
end to end for PIC18. No peripherals are wired yet; Phase 2 adds the
ISR vector (`pic18_isr_vector.c`, using the
`__interrupt(high_priority)` / `__interrupt(low_priority)` syntax
recorded in the plan) and the first real drivers.

## Using this project

### Command line

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

# Build for PIC18F4550 (default).
make

# Build for any other part in the family.
make MCU=18F2455    # 28-pin, 24 KB
make MCU=18F2550    # 28-pin, 32 KB
make MCU=18F4455    # 40/44-pin, 24 KB, SPP
make MCU=18F4550    # 40/44-pin, 32 KB, SPP (default)
```

The result is `build/<MCU>-firmware.hex`. Program it with your favourite
tool (`ippe`, `PK2CMD`, MPLAB IPE, ...).

### MPLAB X IDE

Open this directory as a Standalone Project (or create a new Standalone
Project for the chosen device + XC8 toolchain, then add the HAL sources
under `../../src/`, `../../../pic8-common/src/`, and one application
`.c` from `../../tests/`, plus the include dirs `../../include`,
`../../include/target`, `../../../pic8-common/include`).

## Adjusting for your board

The configuration words in the `Makefile` set a conservative default
(DS39632E §23.1):

- `FOSC = HS`, `PLLDIV = 5`, `CPUDIV = OSC1_PLL2`, `USBDIV = 2`
  (20 MHz crystal → 48 MHz CPU/USB via the PLL)
- `WDT = ON`, `WDTPS = 32768` (refresh via `EPIC_WDT_Refresh()` in Phase 2)
- `PWRT = ON`, `MCLRE = ON`, `LVP = OFF`, `XINST = OFF`, `DEBUG = OFF`
- Code / write / read protection all `OFF`

Change these for your hardware. The block-3 protection settings
(`CP3` / `WRT3` / `EBTR3`) are emitted only for the 32 KB parts
(18F2550 / 18F4550); the 24 KB parts have no block 3.
