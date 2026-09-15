# PIC18F1320 HAL

Hardware abstraction layer for **PIC18F1320**, family B phase 1 of 4
(umbrella epic-hal#150), forked from `pic18fxx5x-hal` (same Access Bank
addressing and two-vector interrupt architecture). Every constant,
register address and behavior is taken 1-to-1 from the datasheet
[DS39605F](https://ww1.microchip.com/downloads/en/DeviceDoc/30009605g.pdf)
(PIC18F1220/1320 Data Sheet).

➜ **[MANUAL.md](MANUAL.md)** is the datasheet-cited human-readable manual
for this family.

## Status: complete (epic-hal#178, #179, #180, #181)

GPIO (PORTA/PORTB only, both full 8-bit), Timer0-3, the interrupt core,
WDT/Sleep/BOR/POR, ECCP1 (Enhanced Capture/Compare/PWM), the EUSART, the
10-bit ADC (7 channels) and the 256-byte Data EEPROM are all implemented
and verified (host sim + real `mdb`), each through the full section-4
gate. This part has no MSSP, CCP2, comparator or USB, confirmed absent
from the DFP header.

## XC8 codegen gotchas

Same rule as `pic18fxx5x-hal`: never pass an SFR address as a runtime
value on PIC18 (a runtime-computed SFR pointer compiles to the
program-memory table mechanism, silently writing nowhere). Every SFR
access here names a compile-time-constant `PIC_REG_*` token.

## Build (host simulation)

```sh
cmake -B build -S .
cmake --build build

./build/example_blink          # Timer0 + GPIO + interrupt smoke
./build/example_timer0_irq     # IRQ backend's dedicated smoke
./build/example_timer1         # Timer1 overflow smoke
./build/example_timer2         # Timer2 PR2-match smoke
./build/example_timer3         # Timer3 overflow smoke
./build/example_ccp            # ECCP1 compare-toggle smoke
./build/example_usart          # EUSART BRG + init + TX smoke
./build/example_adc            # ADC AN0 conversion smoke
./build/example_eeprom         # Data EEPROM write/read smoke
./build/example_smoke          # bare harness seam
```

## Build (real target) and the mdb gate

Manifest-driven (`epic-common/manifest/modules.toml`'s `PIC18F1320`
family and `pic18f1320-hal` module), same as every other family:

```sh
python3 scripts/epic_build.py build --module pic18f1320-hal --mcu 18F1320 \
  --dfp-dir /opt/microchip/xc8/v4.00/pic/packs/Microchip.PIC18Fxxxx_DFP/xc8
```

The dedicated IRQ smoke (`tests/example_timer0_irq.c`) is verified under
real `mdb` via `MODE=toggle` (watching `LATB`, not the default `PORTB`;
see below), not `MODE=gpio`'s marker protocol: a `run`+`wait` mdb session
did not reliably let its bounded main loop finish inside the wait window
(measured empirically), the same failure shape
`docs/adding-a-device.md` section 4 step 6's sub-bullet documents for
continuously-firing ISRs under MPLAB SIM.

**This part's simulated `PORTB` does not mirror the `LATB` output latch**
for an output-configured pin under MPLAB SIM (confirmed empirically:
`LATB` visibly toggled across `stepi` samples while `PORTB` read a
constant 0 across the same run), unlike every other family's
toggle/gpio gate. `scripts/ci-target-sim.sh` exports `TOGGLE_REG=LATB`
for this family's gate for that reason.

## Not yet wired into `family-check.yml`

This family is not yet added to `.github/workflows/ci.yml`'s
`family-check.yml` calls: that reusable job's "epic-hal init
scaffold-and-build" step needs at least one consumer module for the
family (`bundlegen.modules_for_family` excludes the family's own `-hal`
dir by design), which `pic18f1320-hal` does not have yet.
`PIC16F193X`'s own job was wired in only once
`epic-pic16f193x-firmware` existed as that consumer; this family gets
the same treatment once a real module targets it. Until then, this
family's device-data audits (`scripts/sfr-map-audit.py`,
`config-key-audit.py`, `hex-identity-audit.py`) and its `mdb` gate
(`scripts/ci-target-sim.sh`) are wired and pass, verified manually.
