# MPLAB X project seed for the PIC16F87XA family

This directory's `Makefile` (a standalone HAL-only smoke build,
`tests/example_blink.c`) is gone; the manifest-driven build replaces it
and has no direct equivalent, since PIC16F87XA's own modules (e.g.
`epic-tick`) already compile the family's full HAL and are what CI
actually exercises:

```sh
python3 scripts/epic_build.py build --module epic-tick --mcu 16F877A --run
```

See `epic-common/manifest/README.md` for the full module list.

## What works on real silicon

The HAL compiles on XC8 because:

- The SFR macros resolve to volatile direct-access on real targets via
  `include/target/pic16f87xa_platform.h` (`include/target` precedes
  `include` on the include path, see the manifest's own
  `families.PIC16F87XA.includes`).
- `pic16f87xa_sim.c` and the host-side harness / WDT-sleep
  implementations are simply not in a real-target build's source list,
  XC8 never links them.
- The weak ISRs (`TIMER0_IRQHandler`, `ADC_IRQHandler`, …) and the
  `clrwdt` / `sleep` asm helpers compile to native instructions on
  XC8.

## nbproject/

`nbproject/` stays, unlike the `Makefile`: it seeds this family's
reference MPLAB X project, which the bundle generator
(`scripts/make_bundle.py`) builds headlessly into the release bundles.
Do not delete it.

## Adjusting for your board

`epic-common/manifest/modules.toml`'s per-module `example.PIC16F87XA`
tables carry the config word (FOSC/WDTE/PWRTE/BOREN/LVP/WRT and so on)
that used to live in this directory's `Makefile`. The full
Configuration Word layout is DS39582B §14.1, Register 14-1.
