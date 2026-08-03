# PIC16F193X HAL, XC8 / MPLAB X real-target build

Builds the HAL for real PIC16F193X silicon via MPLAB XC8 (`xc8-cc`).

## Prerequisite: the PIC12-16F1xxx DFP

The PIC16F193X is an Enhanced Mid-range part. Its device support lives
in the **Microchip.PIC12-16F1xxx_DFP**, which is **not** bundled with
the XC8 installer and is **not** pinned in this repo's CI toolchain image
yet (`docker/ci-toolchain/Dockerfile` pins only the classic
PIC16Fxxx_DFP + PIC18Fxxxx_DFP).

Installed locally at
`/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC12-16F1xxx_DFP/`
(version 1.9.258, from the `.atpack` unpacked into both the flat and
versioned pack layout, matching the existing PIC16Fxxx_DFP/
PIC18Fxxxx_DFP convention). `DFP_DIR` in the `Makefile` already points
here by default; override it if your install lives elsewhere (via MPLAB
X's *Tools > Packs* panel, or by unpacking a `.atpack` from
`packs.download.microchip.com` into your own `.mchp_packs` /
`xc8/pic/packs` tree).

## Build

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make MCU=16F1937          # default; also 1933/1934/1936/1938/1939
make clean
```

Output is `build/16F1937-firmware.hex`, program it with MPLAB X, MPLAB
IPE, or any external programmer (PICkit, ICD). Default app is
`tests/example_blink.c`; replace `APP_SOURCES` in the `Makefile` with
your project's `main.c`.

## Status

**Real-target build passes** for all six parts (`make MCU=16F193{3,4,6,7,8,9}`),
each producing a valid Intel-HEX firmware image, with the DFP above
installed. One datasheet/DFP disagreement was found and fixed getting
here: the DFP's `PIC16F1937.PIC` marks the `DEBUG` config-word field
`islanghidden`, meaning XC8 rejects it as a user `#pragma config`
(error 1363, reserved for debugger tooling); it is not emitted by this
Makefile's config-word recipe. The other 12 directives
(FOSC/WDTE/PWRTE/MCLRE/CP/CPD/BOREN/CLKOUTEN/IESO/FCMEN/LVP/STVREN/
PLLEN/WRT) are confirmed accepted by the DFP.

Per `docs/adding-a-device.md`, "it compiled" is necessary but not
sufficient: no peripheral counts as done until the §4 `mdb`
register-readback gate passes, and `mdb` (MPLAB SIM, headless, part of
MPLAB X) is not yet installed. That is the only remaining toolchain gap.
