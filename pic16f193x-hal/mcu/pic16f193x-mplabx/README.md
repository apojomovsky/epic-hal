# PIC16F193X HAL, XC8 / MPLAB X real-target build

Builds the HAL for real PIC16F193X silicon via MPLAB XC8 (`xc8-cc`).

## Prerequisite: the PIC12-16F1xxx DFP

The PIC16F193X is an Enhanced Mid-range part. Its device support lives
in the **Microchip.PIC12-16F1xxx_DFP**, which is **not** bundled with
the XC8 installer and is **not** pinned in this repo's CI toolchain image
yet (`docker/ci-toolchain/Dockerfile` pins only the classic
PIC16Fxxx_DFP + PIC18Fxxxx_DFP).

Install it first (either via MPLAB X's *Tools > Packs* panel, or by
unpacking the `.atpack` from `packs.download.microchip.com` into your
`.mchp_packs` / `xc8/pic/packs` tree), then point `DFP_DIR` at the DFP's
`xc8/` subdirectory if it is not in the default location below.

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

This Makefile is a **draft pending the DFP**: the `#pragma config`
directive names are the standard Enhanced Mid-range set and are
verified against the DFP's device `.PIC` the first time the build runs
with the pack installed (the `#pragma config` spellings come from the
device file, not from XC8 itself). The host-sim build
(`../CMakeLists.txt`) has no such dependency and runs now. Per
`docs/adding-a-device.md`, no peripheral counts as done until the §4
`mdb` register-readback gate passes, which also needs this DFP plus
`mdb` (MPLAB SIM, headless).
