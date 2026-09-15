# PIC18F6520 HAL, XC8 real-target build

This family's real-target build is manifest-driven (see
`epic-common/manifest/README.md`); there is no standalone Makefile here.
CI builds the family's blink smoke and its MPLAB SIM gates via the
`pic18f6520-hal` manifest pseudo-module:

```sh
python3 scripts/epic_build.py build --module pic18f6520-hal --mcu 18F6520 --run
```

The PIC18F6520 uses the `Microchip.PIC18Fxxxx_DFP` device pack (the same
DFP as the 18F2455/4550 family), so no extra pack install is needed beyond
what the toolchain image already ships. The DFP version is pinned in
`docker/ci-toolchain/Dockerfile` (`PIC18FXXXX_DFP_VERSION`).

## Configuration words (DS39609B §23.0)

The 6520 has no USB peripheral, so its config layout drops the 2455
family's `USBDIV` / `CPUDIV` / `PLLDIV` / `VREGEN` fields, and its
oscillator-select key is `OSC` (not the 4550 family's `FOSC`). The
brown-out and stack-reset fields are spelled `BOR` / `STVR` on this
part, not `BOREN` / `STVREN` (Register 23-2/23-5); there is no
`MCLRE`, `IESO`, `FCMEN`, `PBADEN` or `XINST` key. The manifest
`example.PIC18F6520` table carries the default set used by all 6520
builds: `OSC = HS`, `WDT = OFF` (kept off in the foundation so a
bounded diagnostic build cannot be reset mid-run), `PWRT = ON`,
`LVP = OFF`, `DEBUG = OFF`, `BOR = ON`, `STVR = ON`, with code /
write / table-read protection all `OFF`.

## HARNESS=sim gates

The foundation has no EUSART driver, so the MPLAB SIM gates report the
PASS/FAIL marker on a GPIO pin, not UART: the mdb harness
(`src/mdb/pic18_harness_mdb.c`) drives RA0 (PORTA bit 0) on the marker.
Under MPSIM, a PIC18 driven output latch does not read back on the PORTx
input register (verified on PIC18F6520, 2026-09-15), so the CI wrapper
reads the latch `LATA` (via `GPIO_REG=LATA` in `scripts/ci-target-sim.sh`),
not `PORTA`. This is the PIC16F193X pattern adapted for PIC18 (see
`pic16f193x-hal/mcu/pic16f193x-mplabx/README.md`); once the EUSART driver
lands in phase 3 the gates can switch to `MODE=uart`.
