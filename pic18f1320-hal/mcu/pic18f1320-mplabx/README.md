# PIC18F1320 HAL, XC8 real-target build

Manifest-driven, same as `pic18fxx5x-hal`: no standalone Makefile here.
Build the HAL directly via its own manifest module:

```sh
python3 scripts/epic_build.py build --module pic18f1320-hal --mcu 18F1320 --run
```

See `epic-common/manifest/README.md` for the full module list.

## One-time setup: install the PIC18Fxxxx DFP

Same pack as `pic18fxx5x-hal` (`Microchip.PIC18Fxxxx_DFP`), not bundled
with XC8; see `pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md`'s "One-time
setup" section for the install steps.

## What works on real silicon

Same platform-layer split as `pic18fxx5x-hal`: `include/target`
resolves the SFR macros to volatile direct-access, `pic18_sim.c` and
the host-side harness are never linked into a real-target build.

## Adjusting for your board

`epic-common/manifest/modules.toml`'s `[modules.pic18f1320-hal.example.PIC18F1320]`
table carries the config words:

- `OSC = HS` (20 MHz crystal, no PLL on this part)
- `WDT = ON`, `WDTPS = 32768` (refresh via `EPIC_WDT_Refresh()`)
- `PWRT = ON`, `MCLRE = ON`, `LVP = OFF`, `DEBUG = OFF`
- Code / write / read protection all `OFF`

The full Configuration Word layout is DS39605F §26.1. This part's field
set is smaller than `pic18fxx5x-hal`'s: no `CPUDIV`/`PLLDIV`/`USBDIV`/
`VREGEN` (no USB), no `CCP2MX`/`PBADEN`/`LPT1OSC`, no `CP2`/`WRT2`/
`EBTR2` (only one flash-protection block, given the smaller 4096-word
flash), and `STVR` replaces `STVREN`.
