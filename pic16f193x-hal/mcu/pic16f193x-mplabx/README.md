# PIC16F193X HAL, XC8 real-target build

Builds the HAL for real PIC16F193X silicon via MPLAB XC8 (`xc8-cc`).

## Prerequisite: the PIC12-16F1xxx DFP

The PIC16F193X is an Enhanced Mid-range part. Its device support lives
in the **Microchip.PIC12-16F1xxx_DFP**, which is **not** bundled with
the XC8 installer, but **is** pinned in this repo's CI toolchain image
(`docker/ci-toolchain/Dockerfile`, version 1.9.258, alongside the
classic PIC16Fxxx_DFP + PIC18Fxxxx_DFP) and is baked into the pushed
`ghcr.io/apojomovsky/pic8-hal-ci` image `make image`/CI pull. No manual
DFP setup is needed if you're using the root `Makefile`'s Docker flow
(see repo-root `README.md`'s Docker quick start).

If building outside that image, it needs installing locally at
`.../pic/packs/Microchip.PIC12-16F1xxx_DFP/` (from the `.atpack`
unpacked into both the flat and versioned pack layout, matching the
existing PIC16Fxxx_DFP/PIC18Fxxxx_DFP convention).

## Build

Build this example with the manifest-driven driver, from the repo root:

```sh
python3 scripts/epic_build.py build --module epic-pic16f193x-firmware --mcu 16F1937 --run
```

Also 16F1933/1934/1936/1938/1939. Output is
`build/16F1937-firmware.hex`, program it with MPLAB X, MPLAB IPE, or
any external programmer (PICkit, ICD). Default app is
`tests/example_blink.c`; replace the manifest's example sources
(`epic-common/manifest/README.md`) with your project's `main.c`.

The `Makefile` this directory used to hold is gone; see
`epic-common/manifest/README.md`.

## Status

**Real-target build passes** for all six parts, each producing a valid
Intel-HEX firmware image, with the DFP above installed. One
datasheet/DFP disagreement was found and fixed getting here: the DFP's
`PIC16F1937.PIC` marks the `DEBUG` config-word field `islanghidden`,
meaning XC8 rejects it as a user `#pragma config` (error 1363, reserved
for debugger tooling); it is not emitted by the manifest's config-word
data. The other 12 directives
(FOSC/WDTE/PWRTE/MCLRE/CP/CPD/BOREN/CLKOUTEN/IESO/FCMEN/LVP/STVREN/
PLLEN/WRT) are confirmed accepted by the DFP.

Per `docs/adding-a-device.md`, "it compiled" is necessary but not
sufficient: no peripheral counts as done until the §4 `mdb`
register-readback gate passes for it specifically. The toolchain gap is
closed, `mdb` (MPLAB SIM, headless, part of MPLAB X) is installed and
confirmed working (verified against `epic-tick`'s pilot module, both
existing families, both reaching a real `EPIC_HARNESS_RESULT: PASS` via
CI's sim gate, see DEVELOPMENT.md's Docker section). This
family's own `HARNESS=sim` (`epic-pic16f193x-firmware`'s sim variant,
`epic-common/manifest/modules.toml`) reports over `MODE=gpio` (RA0
PASS/FAIL marker, this family has no EUSART driver yet) and is gated
in `sim-tests.yml` alongside `epic-tick`'s two families.
