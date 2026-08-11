# PIC16F193X HAL

Register-level hardware abstraction layer for the PIC16F1933, 1934,
1936, 1937, 1938, and 1939 (Enhanced Mid-range core, F and LF
variants). Covers the full peripheral set: GPIO, Timer0/1/2/4/6, five
CCP modules, EUSART, MSSP, ADC, dual comparators, DAC, FVR, SR latch,
capacitive sensing, EEPROM, and the LCD segment driver. Register facts
are taken from
[DS41364B](https://ww1.microchip.com/downloads/en/DeviceDoc/41364B.pdf).

Third family in this repo, alongside the PIC16F87XA and PIC18F2455
HALs. All three implement a shared API contract from epic-common: same
function names and signatures, family-specific register bodies
underneath. Applications written against the shared API are portable
across families at build time (include-path swap), not source time.
The contract design is documented in
[epic-common/README.md](../epic-common/README.md) +
[epic-common/MANUAL.md](../epic-common/MANUAL.md), and the procedure
for adding a new family in
[docs/adding-a-device.md](../docs/adding-a-device.md).

## Status

Every peripheral on the part is implemented and verified through three
stages: host simulation, six-part real-target XC8 build, and the mdb
(MPLAB SIM) register-readback gate.

- [x] Family header, SFR map, platform layer (host + target)
- [x] Interrupt core (23 sources, INTCON + PIR1/2/3 + PIE1/2/3, single
      vector with automatic context save, no manual push/pop)
- [x] GPIO (PORTA-E, LAT/ANSEL/WPUB/WPUE/IOC, init/read/write/toggle)
- [x] Timer0, Timer1 (16-bit, prescaler, atomic read/write)
- [x] Timer2 / Timer4 / Timer6 (PR-match + postscaler, one driver, three instances)
- [x] CCP1-5 (capture/compare, CCP1-3 Enhanced, CCP4/5 plain, one driver, five instances)
- [x] EUSART (async 8-bit, baud computation, TX/RX, weak ISRs)
- [x] MSSP (SPI master, CKE=1 errata-safe default)
- [x] ADC (10-bit, FRC errata-safe clock, channel select, right/left justify)
- [x] Comparator C1/C2 (enable, hysteresis, polarity, output, edge interrupt)
- [x] EEPROM (256 bytes, data space, 0x55/0xAA unlock, read/write)
- [x] DAC (5-bit, output value, enable)
- [x] FVR (fixed voltage reference, ADC/comp/DAC gain, ready flag)
- [x] SR Latch (enable, set/reset pulse, output steering)
- [x] Capacitive Sensing (channel select, range, Timer0 oscillator routing)
- [x] LCD segment driver (24/16 segments per device, pixel-level set/clear,
      DFP-derived segment-to-register mapping)
- [x] WDT / Sleep / BOR / POR helpers
- [x] Host simulation backend (flat-array register file, Timer0/1/2-6,
      GPIO, IOC, EUSART TRMT, EEPROM 256-byte model)
- [x] Test harness (bounded host run, real-target firmware, MODE=gpio
      RA0 PASS/FAIL marker for the mdb gate)
- [x] Real-target build (manifest-driven, `epic-pic16f193x-firmware`
      module, all 6 parts)

## Quick start

Host simulation (no hardware, no toolchain):

```sh
cmake -B build && cmake --build build
./build/example_blink        # Timer0 + GPIO + IRQ blink
./build/example_timer246     # 3 timers, 3 RC pins, all at once
./build/example_ccp         # CCP1 + CCP2 compare-set register check
```

Real target (needs XC8 + the PIC12-16F1xxx DFP):

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f193x-mplabx MCU=16F1937
```

## Layout

```
include/
  pic16f193x.h          device selection + per-device capability macros
  pic16f193x_sfr.h      SFR addresses, bit masks, POR values (DS41364B-cited)
  host/                 host platform header (memory-backed SFR array)
  target/               target platform header (volatile-deref SFR)
  core/                 IRQ, WDT/Sleep, harness, neutral shims
  peripherals/          GPIO, Timer0-6, CCP1-5, EUSART, MSSP, ADC, etc.
src/
  core/                 IRQ backend, dispatch, ISR vector, harness, WDT/Sleep
  peripherals/          all peripheral driver bodies
  sim/                  host simulation backend (flat-array register file)
tests/                  example_blink, example_timer1, example_timer246, ...
mcu/pic16f193x-mplabx/  XC8 Makefile + config words (all 6 parts)
docs/                   ARCHITECTURE.md (XC8 codegen findings)
```

## Build model

Two builds, one source tree, no `#ifdef` in driver or example code. The
host build puts `include/host` first on the include path so
`pic16f193x_platform.h` resolves to a memory-backed SFR array; the target
build puts `include/target` first so it resolves to volatile-deref. The
linked harness file (`pic16f193x_harness_sim.c` vs
`pic16f193x_harness_sim_target.c`) and the sim backend (`src/sim/`,
host-only) are selected by the build, not by preprocessor. Every example
source compiles unchanged in both builds.

## Documentation

- [**MANUAL.md**](MANUAL.md): per-family register-level reference
  (datasheet-cited), covering every peripheral's register layout, driver
  API, and errata notes.
- [**docs/ARCHITECTURE.md**](docs/ARCHITECTURE.md): XC8 codegen findings
  for this family (BSR banking, PIE1/2/3 RMW fix, FSR/INDF verification).
- [**../epic-common/README.md**](../epic-common/README.md) +
  [**../epic-common/MANUAL.md**](../epic-common/MANUAL.md): the shared
  contract every family implements (status codes, harness, interrupt
  model, naming conventions).
- [**../epic-common/README.md**](../epic-common/README.md) +
  [**../epic-common/MANUAL.md**](../epic-common/MANUAL.md): the fixed
  contract design and how families interoperate.
- [**../docs/adding-a-device.md**](../docs/adding-a-device.md): the
  verification-gated playbook for adding a new device or family.
