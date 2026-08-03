# pic16f193x-hal

Hardware-abstraction layer and host-simulation backend for the
**PIC16F193X** family (PIC16F1933/1934/1936/1937/1938/1939, F + LF), the
Enhanced Mid-range 8-bit PIC core with an LCD segment driver, per-pin
interrupt-on-change, and 5 CCP modules. Datasheet: DS41364B.

This is the repo's third HAL family. It implements the shared
`pic8-common` contract (same `HAL_GPIO_*` / `HAL_TIMER0_*` / `HAL_IRQ_*`
names and signatures as `pic16f87xa-hal` and `pic18fxx5x-hal`) with
family-shaped bodies for the Enhanced Mid-range core. Why a new family,
not a variant of `pic16f87xa-hal`: the 193X banks data memory with the
**BSR** (up to 32 banks x 128 bytes, not RP0/RP1's 4 banks), uses a
single interrupt vector with **automatic context save** (no manual
ISR push/pop), and an LATx/ANSELx I/O model. See
[`docs/pic16f193x-plan.md`](../docs/pic16f193x-plan.md) and
[`docs/adding-a-device.md`](../docs/adding-a-device.md) for the
rationale and the procedure this addition follows.

## Status

**Foundation, host-sim verified.** The platform headers, SFR map, IRQ
backend (23 sources across INTCON + PIR1/2/3 + PIE1/2/3), dispatch,
single-vector ISR, harness, WDT/Sleep, GPIO (LAT/ANSEL/WPUB/IOC), Timer0,
and the host simulation backend build and pass `ctest`. The remaining
peripherals (Timer1/2/4/6, 5x CCP, EUSART, MSSP, ADC, Comparator, DAC,
FVR, SR latch, capacitive sensing, LCD driver, EEPROM) are added one at
a time, each through the §4 verification gate.

The **real-target XC8 build and the `mdb` register-readback gate are
deferred** until the `Microchip.PIC12-16F1xxx_DFP` (not bundled with XC8,
not yet pinned in CI) is installed. Until then every piece is
`host-verified, mdb-pending`, and an XC8 codegen probe of the
known-risky SFR-access patterns (docs/adding-a-device.md appendix) runs
before any peripheral relies on them.

## Layout

```
include/      family + core/peripheral headers
  host/       host platform header (memory-backed SFR)
  target/     target platform header (volatile-deref SFR)
  core/       IRQ, WDT/Sleep, neutral shims
  peripherals/ GPIO, Timer0, neutral shims
src/
  core/       IRQ backend, dispatch, ISR vector, harness, WDT/Sleep
  peripherals/ GPIO, Timer0
  sim/        host simulation backend (flat-array register file)
tests/       example_blink.c, example_gpio.c
mcu/pic16f193x-mplabx/  XC8 Makefile (+ DFP pin, config words)
docs/        ARCHITECTURE.md (XC8 codegen findings, filled as found)
```

## Build (host simulation)

```sh
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
./build/example_blink
./build/example_gpio
```

The host build puts `include/host` first on the include path, so
`pic16f193x_platform.h` resolves to the memory-backed-SFR version and
the harness/WDT-sim implementations link. No DFP, no hardware needed.

## Build (real target)

See [`mcu/pic16f193x-mplabx/README.md`](mcu/pic16f193x-mplabx/README.md).
Needs the PIC12-16F1xxx DFP installed first.

## Documentation

- [`MANUAL.md`](MANUAL.md): per-family register-level reference
  (datasheet-cited), points to `pic8-common/MANUAL.md` for shared
  conventions.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): XC8 codegen findings
  for this family (filled as the §4 gate surfaces them).
- [`../pic8-common/README.md`](../pic8-common/README.md) +
  [`../pic8-common/MANUAL.md`](../pic8-common/MANUAL.md): the shared
  contract every family implements.
