<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/epicurus-logo-dark-mode.svg">
    <img src="docs/assets/epicurus-logo-light-mode.svg" width="120" alt="Epicurus logo: a chip-temple inside a laurel wreath">
  </picture>
</p>

<h1 align="center">Epicurus</h1>

<p align="center"><em>Built down to what the datasheet requires.</em></p>

<p align="center">

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Toolchain: MPLAB XC8](https://img.shields.io/badge/toolchain-MPLAB%20XC8-green.svg)](https://www.microchip.com/mpgb/xc8.html)
[![ci](https://github.com/apojomovsky/epicurus/actions/workflows/ci.yml/badge.svg)](https://github.com/apojomovsky/epicurus/actions/workflows/ci.yml)

</p>

8-bit PIC microcontrollers are not glamorous. They ship by the billion
into thermostats, motor controllers, and blinking status LEDs, with no
vendor HAL and no abstraction, just a register map and a datasheet PDF.
Epicurus argued that a good life comes from reducing things to their
essentials, not adding to them. This library takes that literally: one
register-level driver per peripheral, faithful to the datasheet, and one
contract that holds across every family it's ported to, however little
those families have in common.

## Contents

- [Why](#why)
- [Supported devices](#supported-devices)
- [Quick start](#quick-start)
  - [Host simulation](#host-simulation)
  - [Docker (no local installs)](#docker-no-local-installs)
  - [Real hardware](#real-hardware)
  - [Using Epicurus in your own project](#using-epicurus-in-your-own-project)
- [Modules](#modules)
- [Documentation](#documentation)
- [Development](#development)
- [License](#license)

## Why

The shape of that, concretely:

- **Datasheet-faithful, not clever.** Every register, bit name, and
  reset value is taken 1-to-1 from Microchip's own datasheets, cited in
  the source and in each family's `MANUAL.md`.
- **The host is a first-class target.** Every module builds and runs as
  a host program under CMake/ctest, so logic gets exercised long before
  it touches a programmer.
- **One contract, several families.** `epic-common/` holds everything
  architecture-blind; each family HAL implements the same names and
  signatures over different registers, so higher-level modules are
  written once and build against any of them unchanged.
- **Zero framework tax.** No RTOS, no dynamic allocation, no C++. Plain
  C99, cooperative scheduling, static storage, every module usable on
  its own.
- **Proven on real silicon, not just compiled.** CI runs three stages on
  every push: host build and test, a real XC8 cross-compile of every
  module against every supported part, and a real run under MPLAB SIM
  (mdb, headless) that checks actual register and UART output.

## Supported devices

| Family | Parts | HAL module | Notes |
|---|---|---|---|
| PIC16F87XA | 16F873A / 874A / 876A / 877A | [pic16f87xa-hal](pic16f87xa-hal/) | Full peripheral coverage. DFP ships with XC8. |
| PIC18F2455 family | 18F2455 / 2550 / 4455 / 4550 | [pic18fxx5x-hal](pic18fxx5x-hal/) | Full peripheral coverage. Needs the PIC18Fxxxx DFP. |
| PIC16F193X | 16F1933 / 1934 / 1936 / 1937 / 1938 / 1939 | [pic16f193x-hal](pic16f193x-hal/) | Full peripheral coverage: GPIO, Timer0/1/2/4/6, CCP1-5, EUSART, MSSP, ADC, Comparator, EEPROM, DAC, FVR, SR latch, CPS, LCD. Needs the PIC12-16F1xxx DFP. |

All three families share a single API contract (epic-common): same
function names and signatures, family-specific register bodies. Family-agnostic
modules (scheduler, math, serial, Modbus, ...) build against any family
by selecting the HAL at build time; a few target one family only where
the peripheral doesn't exist (USB, SD card), called out in the
[Modules](#modules) table. Adding a new family follows the procedure in
[docs/adding-a-device.md](docs/adding-a-device.md).

## Quick start

### Host simulation

No hardware, no MPLAB tooling, just CMake 3.16+ and a C99 compiler:

```sh
cmake -B build -S epic-taskmgr
cmake --build build
./build/example_multi_blink
```

```
[t=  5] fast  #1
[t= 10] fast  #2
[t= 10] med   #1
[t= 20] fast  #4
[t= 20] med   #2
[t= 20] slow  #1
[t= 40] super  spawned blip
[t= 40] fast  #8
[t= 41] blip  #1
[t= 60] slow  #3
done: fast=12 med=6 slow=3 blips=1 (ticks=61, tasks=4)
```

Four blinks at distinct rates on RB0-RB3, plus a priority-0 supervisor
that spawns a one-shot blip at runtime at t=40. Point the same task
manager at the PIC18 family with `-DEPIC_FAMILY=PIC18` (see
[epic-taskmgr/README.md](epic-taskmgr/README.md)).

### Docker (no local installs)

Don't want XC8/MPLAB X/CMake on your own machine? The root `Makefile`
runs the whole workflow, host tests, real-target XC8 builds, the `mdb`
verification gate, and a dev shell, inside one Docker image:

```sh
make check-vendor    # one-time: tells you which 2 files to grab from Microchip
make image           # build the toolchain image locally
make test            # every module's host-sim tests
make shell           # interactive shell, repo mounted at /repo
```

See [Development](#development) for the full command reference and
[docs/docker-dev-plan.md](docs/docker-dev-plan.md) for the design.

### Real hardware

A manifest-driven build driver, not per-module Makefiles: `epic-common/manifest/modules.toml` is the single source of truth for what
each module compiles and which parts it supports.

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 16F877A --run
```

Also 873A / 874A / 876A. Produces `build/16F877A-multi-blink.hex`,
program with MPLAB X or any programmer. See
[epic-taskmgr/README.md](epic-taskmgr/README.md) for wiring, the family
`MANUAL.md` (e.g. [pic16f87xa-hal/MANUAL.md](pic16f87xa-hal/MANUAL.md))
for peripheral bring-up, [epic-common/manifest/README.md](epic-common/manifest/README.md)
for the manifest schema, and [docs/adding-a-device.md](docs/adding-a-device.md)
for adding a new part or family.

### Using Epicurus in your own project

Grab the bundle for your family from
[Releases](https://github.com/apojomovsky/epicurus/releases), unpack it,
and point one variable at it:

```make
EPICURUS_DIR := third_party/epicurus
EPICURUS_MCU := 16F877A
EPICURUS_MODULES := serial tick
include $(EPICURUS_DIR)/epicurus.mk

SRCS := main.c $(EPICURUS_SRCS)
CFLAGS += $(EPICURUS_CFLAGS)
```

Dependencies resolve automatically, and asking for a module on a part it
does not fit fails immediately with the reason rather than as a wall of
XC8 linker errors. Each bundle carries its own `QUICKSTART.md`,
`SUPPORT.md`, and `MPLABX.md`, plus a reference MPLAB X project under
`examples/`.

MPLAB X and the MPLAB extension for VS Code are supported too: open
`examples/epicurus-demo.X`, or follow `MPLABX.md` to add Epicurus to an
existing project.

## Modules

Every module below ships its own `README.md` (what/why), most also have
`docs/ARCHITECTURE.md` and `docs/API.md`; the HALs additionally have a
per-peripheral `MANUAL.md`. "Family-agnostic" means one implementation
builds unchanged against multiple families; where a module targets only
one family, that's called out.

**Core & HALs**

| Module | Description |
|---|---|
| [epic-common](epic-common/) | Shared layer every family reuses: status codes, host/target harness, CMake/Make fragments. |
| [pic16f87xa-hal](pic16f87xa-hal/) | HAL for PIC16F873A/874A/876A/877A. Full peripheral coverage: GPIO, Timers, CCP, MSSP, ADC, Comparator, EEPROM, PSP, WDT. |
| [pic18fxx5x-hal](pic18fxx5x-hal/) | HAL for PIC18F2455/2550/4455/4550. Full peripheral coverage: GPIO, Timer0-3, ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP. |
| [pic16f193x-hal](pic16f193x-hal/) | HAL for PIC16F1933/1934/1936/1937/1938/1939 (Enhanced Mid-range). Full peripheral coverage: GPIO, Timer0/1/2/4/6, CCP1-5, EUSART, MSSP, ADC, Comparator, EEPROM, DAC, FVR, SR latch, CPS, LCD. |

**Scheduling & control flow**

| Module | Description |
|---|---|
| [epic-taskmgr](epic-taskmgr/) | Cooperative scheduler: periodic and one-shot tasks, priority-ordered, race-free. Family-agnostic. |
| [epic-fsm](epic-fsm/) | Table-driven finite state machine, the whole machine is one `static const` transition table. No HAL dependency. |

**Timing & math**

| Module | Description |
|---|---|
| [epic-tick](epic-tick/) | 1 ms timebase (`HAL_GetTick`/`HAL_Delay` equivalent) on a Timer2 auto-reload ISR. Family-agnostic. |
| [epic-math](epic-math/) | Fixed-point math: multiply, divide, BCD, sqrt, numerical diff/integration, RNGs. Host reference plus PIC16/PIC18 inline-asm backends behind one API. |

**Communication**

| Module | Description |
|---|---|
| [epic-serial](epic-serial/) | Interrupt-driven ring-buffered UART + `printf` retarget. Family-agnostic. |
| [epic-bus](epic-bus/) | I2C/SPI "MEM" register-access idiom on top of MSSP/SSP. Family-agnostic. |
| [epic-modbus](epic-modbus/) | Modbus RTU slave: core function codes, T3.5 framing, CRC-16, optional RS-485 driver-enable. Built on `epic-serial` + `epic-tick`. |
| [epic-console](epic-console/) | Line-based serial command dispatcher over `epic-serial`: tokenization, table-driven dispatch, echo/backspace editing. |
| [epic-usb](epic-usb/) | USB CDC-ACM virtual serial port, wraps the vendored M-Stack USB device stack. PIC18Fxx5x-only (no USB peripheral on PIC16F87XA). |

**Storage**

| Module | Description |
|---|---|
| [epic-settings](epic-settings/) | EEPROM-backed settings blobs with CRC-16 validation and first-boot defaults. Family-agnostic. |
| [epic-sdcard](epic-sdcard/) | SD/MMC-over-SPI block storage, wraps the vendored M-Stack storage driver. PIC18Fxx5x-only (RAM constraint). |

**Signal processing & control**

| Module | Description |
|---|---|
| [epic-adcfilter](epic-adcfilter/) | ADC oversample-and-decimate plus an O(1) moving-average filter. No HAL dependency. |
| [epic-debounce](epic-debounce/) | Instantiable digital-input debouncer, poll-driven, built on `epic-tick`'s real timebase. No HAL dependency. |
| [epic-pid](epic-pid/) | Fixed-point (Q8.8) single-loop PID with anti-windup, derivative-on-measurement, and bumpless auto/manual transfer. No HAL dependency. |
| [epic-encoder](epic-encoder/) | Interrupt-driven x4 quadrature decoder, instantiable, built on the HAL's GPIO change-interrupt. No HAL family split. |

**Peripherals**

| Module | Description |
|---|---|
| [epic-lcd](epic-lcd/) | HD44780-compatible character LCD driver with configurable transport: 4-bit GPIO, 8-bit GPIO, or SPI via 74HC595. |

Note: the higher-level modules (taskmgr, tick, serial, ...) build
against the two mature families (PIC16F87XA, PIC18F2455). Wiring them
to PIC16F193X is a natural follow-up now that its own peripheral
coverage is complete.

## Documentation

- [epic-common/MANUAL.md](epic-common/MANUAL.md), family-agnostic
  conventions, the harness, the handle pattern, the shared interrupt
  model. Read this first.
- Per-family `MANUAL.md` ([PIC16F87XA](pic16f87xa-hal/MANUAL.md),
  [PIC18Fxx5x](pic18fxx5x-hal/MANUAL.md),
  [PIC16F193X](pic16f193x-hal/MANUAL.md)), datasheet-cited
  per-peripheral register reference.
- [docs/multi-family-plan.md](docs/multi-family-plan.md), the refactor
  that extracted `epic-common/` and added the PIC18F2455 family behind a
  fixed contract.
- [docs/adding-a-device.md](docs/adding-a-device.md), the operational,
  verification-gated guide for adding a new device variant or family
  (used for PIC16F193X, see [docs/pic16f193x-plan.md](docs/pic16f193x-plan.md)).
- [docs/ci-plan.md](docs/ci-plan.md), the CI design and its
  phase-by-phase findings; [docs/docker-dev-plan.md](docs/docker-dev-plan.md),
  the Docker-first local dev flow and the same toolchain image CI uses.
- Datasheet references (not vendored in this repo) are listed under
  [License](#license).

## Development

**Native.** `./scripts/bootstrap.sh` sets up a fresh clone: installs the
host toolchain the CMake builds need and a pre-commit hook (trailing
newline/whitespace, no-em-dash, `cppcheck` on staged `.c` files).
`--check-only` reports what's missing without installing anything. See
[scripts/README.md](scripts/README.md) for what the hook checks. Real
targets additionally need MPLAB X IDE v6.x and MPLAB XC8 v3.x
(`xc8-cc`), installed by hand (proprietary, license-gated); PIC18 also
needs the PIC18Fxxxx DFP, PIC16F193X the PIC12-16F1xxx DFP (neither
ships with XC8).

**Docker**, avoids all of the above except two files only a human can
fetch (Microchip's download pages block scripted access):

```sh
make check-vendor    # confirms the 2 required installer files are present
                      # and tells you where to get them if not

make image            # build the toolchain image locally (once; cached after)
make test             # host-sim build + test, every module
make test MODULE=epic-lcd   # ... or just one

make xc8-build MODULE=epic-tick MCU=16F877A   # real-target build
make mdb-test MODULE=epic-tick MCU=16F877A DEVICE=PIC16F877A  # the mdb gate

make shell             # interactive shell, repo mounted at /repo
```

Maintainers with `write:packages` access to this repo's GHCR packages
can publish an updated toolchain image with `make ci-image-push
GHCR_OWNER=<owner>` (after `docker login ghcr.io`), the same private tag
CI's workflows pull from; CI itself never builds this image. Full design
and the "why" behind every piece: [docs/docker-dev-plan.md](docs/docker-dev-plan.md).

## License

MIT, see [LICENSE](LICENSE).

The Microchip datasheets
[DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
(c) 2003 Microchip Technology Inc.,
[DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)
(c) 2009 Microchip Technology Inc., and
[DS41364B](https://ww1.microchip.com/downloads/en/DeviceDoc/41364B.pdf)
(c) 2009 Microchip Technology Inc. are vendor documentation; register
and bit names in the code follow them directly. They are not vendored in
this repo, follow the links above to Microchip's own hosted copies.
