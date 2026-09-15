# PIC18F2520 HAL, XC8 real-target build

This family's real-target build is manifest-driven (see
`epic-common/manifest/README.md`); there is no standalone Makefile here.
CI builds the family's blink smoke and its MPLAB SIM gates via the
`pic18f2520-hal` manifest pseudo-module:

```sh
python3 scripts/epic_build.py build --module pic18f2520-hal --mcu 18F2520 --run
```

The PIC18F2520 uses the `Microchip.PIC18Fxxxx_DFP` device pack (the same
DFP as the 18F2455/4550 family), so no extra pack install is needed beyond
what the toolchain image already ships. The DFP version is pinned in
`docker/ci-toolchain/Dockerfile` (`PIC18FXXXX_DFP_VERSION`).

## Configuration words (DS39631E §22.1)

The 2520 has no USB peripheral, so its config layout drops the 2455
family's `USBDIV` / `CPUDIV` / `PLLDIV` / `VREGEN` fields, and its
oscillator-select key is `OSC` (not the 4550 family's `FOSC`). The
manifest `example.PIC18F2520` table carries the default set used by all
2520 builds: `OSC = HS`, `WDT = OFF` (kept off in the foundation so a
bounded diagnostic build cannot be reset mid-run), `PWRT = ON`, `MCLRE =
ON`, `LVP = OFF`, `XINST = OFF`, `DEBUG = OFF`, `BOREN = ON`, `STVREN =
ON`, `PBADEN = OFF`, with code / write / read protection all `OFF`. The
`CCP2MX` bit selects whether CCP2 multiplexes onto RB3 or RC1; it is
irrelevant for the foundation (no CCP driver yet, phase 3).

## HARNESS=sim gates

The foundation has no EUSART driver, so the MPLAB SIM gates report the
PASS/FAIL marker on a GPIO pin, not UART: the mdb harness
(`src/mdb/pic18_harness_mdb.c`) drives RA0 (PORTA bit 0) on the marker.
Under MPSIM, a PIC18 driven output latch does not read back on the PORTx
input register, so the CI wrapper reads the latch `LATA` (via
`GPIO_REG=LATA` in `scripts/ci-target-sim.sh`), not `PORTA`. This is the
PIC16F193X pattern adapted for PIC18 (see
`pic16f193x-hal/mcu/pic16f193x-mplabx/README.md`); once the EUSART driver
lands in phase 3 the gates can switch to `MODE=uart`.
