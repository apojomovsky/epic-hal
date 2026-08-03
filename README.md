# pic8-hal

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Language: C99](https://img.shields.io/badge/C-99-555.svg)](https://en.cppreference.com/w/c)
[![Toolchain: MPLAB XC8](https://img.shields.io/badge/toolchain-MPLAB%20XC8-green.svg)](https://www.microchip.com/mpgb/xc8.html)
[![Runs on: host & silicon](https://img.shields.io/badge/runs%20on-host%20%26%20silicon-orange.svg)](#quick-start)

A datasheet-faithful hardware abstraction layer and a set of firmware
building blocks for 8-bit PIC microcontrollers, in the spirit of
STM32Cube HAL but for parts with no vendor HAL of their own. 16 modules
span the HALs plus scheduling, fixed-point math, serial/Modbus/USB,
EEPROM and SD-card storage, PID, quadrature, debouncing, and character
LCD.

Every module dual-builds from the same source: a host simulation backend
(gcc/CMake, runs as a normal program, no hardware required) and a
real-target build (MPLAB XC8, produces a `.hex`). Applications never
`#ifdef` between them, the split happens at build time via include-path
and linked-file selection.

## Contents

- [Why](#why)
- [Supported devices](#supported-devices)
- [Quick start](#quick-start)
  - [Real hardware](#real-hardware)
- [Modules](#modules)
- [Documentation](#documentation)
- [Requirements](#requirements)
- [Development](#development)
  - [Docker quick start](#docker-quick-start)
- [License](#license)

## Why

8-bit PIC parts ship with no vendor HAL, so firmware for them tends to
grow into a one-off tangle of register poking, copy-pasted between
projects. This repo is a datasheet-faithful alternative: a hardware
abstraction layer in the spirit of STM32Cube HAL, plus the higher-level
building blocks (scheduler, math, serial, bus, storage, control) that sit
on top of it, written once and reused across parts.

The guiding decisions:

- **Be faithful to the datasheet, not clever.** Every register, bit name,
  and reset value is taken 1-to-1 from the Microchip datasheets
  ([DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
  for PIC16F87XA,
  [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)
  for PIC18F2455), cited in the source and in each family's `MANUAL.md`.
- **Make the host a first-class target.** Every module builds and runs as
  a host program under CMake/ctest, so firmware logic gets exercised long
  before it touches a programmer.
- **Share one contract across families.** `pic8-common/` holds everything
  architecture-blind (status codes, the harness, shared build fragments);
  each family HAL implements the same names and signatures over different
  registers. Higher-level modules (scheduler, protocol stacks, drivers)
  are written once and build against either family unchanged.
- **Keep the framework tax at zero.** No RTOS, no dynamic allocation, no
  C++. Plain C99, cooperative scheduling, static storage. Every module is
  usable on its own; nothing requires pulling in the rest of the tree.

## Supported devices

| Family | Parts | HAL module | Notes |
|---|---|---|---|
| PIC16F87XA | 16F873A / 874A / 876A / 877A | [pic16f87xa-hal](pic16f87xa-hal/) | Tested on a 16F877A. DFP ships with XC8. |
| PIC18F2455 family | 18F2455 / 2550 / 4455 / 4550 | [pic18fxx5x-hal](pic18fxx5x-hal/) | Tested on a 18F4550. Needs the PIC18Fxxxx DFP. |

The examples use PORTB only, so they run on every part of both families.
Family-agnostic modules (scheduler, math, serial, Modbus, ...) build
against either family by selecting the HAL at build time; a few modules
target one family only where the peripheral doesn't exist (USB, SD card),
called out in the [Modules](#modules) table.

## Quick start

Build and run the multi-blink example on the host simulator (CMake 3.16+
and a C99 compiler; the HAL is pulled in automatically). The default
targets the PIC16F87XA family:

```sh
cmake -B build -S pic8-taskmgr
cmake --build build
./build/example_multi_blink
```

You should see the scheduler dispatch tasks tick by tick:

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

Four blinks at distinct rates on RB0-RB3, plus a priority-0 supervisor that
spawns a one-shot blip at runtime at t=40. Point the same task manager at
the PIC18 family instead with `-DHAL_FAMILY=PIC18` (see
[pic8-taskmgr/README.md](pic8-taskmgr/README.md)).

### Real hardware

For a real PIC, use the MPLAB XC8 toolchain. There is one Makefile per
family:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

# PIC16F87XA family:
cd pic8-taskmgr/mcu/pic16f87xa-taskmgr-mplabx
make MCU=16F877A        # also 873A / 874A / 876A

# PIC18F2455 family (needs the PIC18Fxxxx DFP installed):
cd ../pic18fxx5x-taskmgr-mplabx
make MCU=18F4550        # also 2455 / 2550 / 4455
```

This produces `build/<MCU>-multi-blink.hex`. Wire LEDs on RB0-RB3 and a
20 MHz crystal, then program with MPLAB X or any programmer. See the task
manager [README](pic8-taskmgr/README.md) for full wiring, and the HAL
[MANUAL](pic16f87xa-hal/MANUAL.md) for peripheral bring-up.

## Modules

Every module below ships its own `README.md` (what/why), most also have
`docs/ARCHITECTURE.md` and `docs/API.md`; the two HALs additionally have a
per-peripheral `MANUAL.md`. "Family-agnostic" means one implementation
builds unchanged against either family; where a module targets only one
family, that's called out.

**Core & HALs**

| Module | Description |
|---|---|
| [pic8-common](pic8-common/) | Shared layer every family reuses: status codes, host/target harness, CMake/Make fragments. |
| [pic16f87xa-hal](pic16f87xa-hal/) | HAL for PIC16F873A/874A/876A/877A. Full peripheral coverage: GPIO, Timers, CCP, MSSP, ADC, Comparator, EEPROM, PSP, WDT. |
| [pic18fxx5x-hal](pic18fxx5x-hal/) | HAL for PIC18F2455/2550/4455/4550. Full peripheral coverage: GPIO, Timer0-3, ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP. |

**Scheduling & control flow**

| Module | Description |
|---|---|
| [pic8-taskmgr](pic8-taskmgr/) | Cooperative scheduler: periodic and one-shot tasks, priority-ordered, race-free. Family-agnostic. |
| [pic8-fsm](pic8-fsm/) | Table-driven finite state machine, the whole machine is one `static const` transition table. No HAL dependency. |

**Timing & math**

| Module | Description |
|---|---|
| [pic8-tick](pic8-tick/) | 1 ms timebase (`HAL_GetTick`/`HAL_Delay` equivalent) on a Timer2 auto-reload ISR. Family-agnostic. |
| [pic8-math](pic8-math/) | Fixed-point math: multiply, divide, BCD, sqrt, numerical diff/integration, RNGs. Host reference plus PIC16/PIC18 inline-asm backends behind one API. |

**Communication**

| Module | Description |
|---|---|
| [pic8-serial](pic8-serial/) | Interrupt-driven ring-buffered UART + `printf` retarget. Family-agnostic. |
| [pic8-bus](pic8-bus/) | I2C/SPI "MEM" register-access idiom on top of MSSP/SSP. Family-agnostic. |
| [pic8-modbus](pic8-modbus/) | Modbus RTU slave: core function codes, T3.5 framing, CRC-16, optional RS-485 driver-enable. Built on `pic8-serial` + `pic8-tick`. |
| [pic8-console](pic8-console/) | Line-based serial command dispatcher over `pic8-serial`: tokenization, table-driven dispatch, echo/backspace editing. |
| [pic8-usb](pic8-usb/) | USB CDC-ACM virtual serial port, wraps the vendored M-Stack USB device stack. PIC18Fxx5x-only (no USB peripheral on PIC16F87XA). |

**Storage**

| Module | Description |
|---|---|
| [pic8-settings](pic8-settings/) | EEPROM-backed settings blobs with CRC-16 validation and first-boot defaults. Family-agnostic. |
| [pic8-sdcard](pic8-sdcard/) | SD/MMC-over-SPI block storage, wraps the vendored M-Stack storage driver. PIC18Fxx5x-only (RAM constraint). |

**Signal processing & control**

| Module | Description |
|---|---|
| [pic8-adcfilter](pic8-adcfilter/) | ADC oversample-and-decimate plus an O(1) moving-average filter. No HAL dependency. |
| [pic8-debounce](pic8-debounce/) | Instantiable digital-input debouncer, poll-driven, built on `pic8-tick`'s real timebase. No HAL dependency. |
| [pic8-pid](pic8-pid/) | Fixed-point (Q8.8) single-loop PID with anti-windup, derivative-on-measurement, and bumpless auto/manual transfer. No HAL dependency. |
| [pic8-encoder](pic8-encoder/) | Interrupt-driven x4 quadrature decoder, instantiable, built on the HAL's GPIO change-interrupt. No HAL family split. |

**Peripherals**

| Module | Description |
|---|---|
| [pic8-lcd](pic8-lcd/) | HD44780-compatible character LCD driver with configurable transport: 4-bit GPIO, 8-bit GPIO, or SPI via 74HC595. |

## Documentation

- [pic8-common/MANUAL.md](pic8-common/MANUAL.md), family-agnostic
  conventions, the harness, the handle pattern, the shared interrupt
  model. Read this first.
- [pic16f87xa-hal/MANUAL.md](pic16f87xa-hal/MANUAL.md) /
  [pic18fxx5x-hal/MANUAL.md](pic18fxx5x-hal/MANUAL.md), per-peripheral
  register reference for each family.
- [docs/multi-family-plan.md](docs/multi-family-plan.md), the refactor
  that extracted `pic8-common/` and added the PIC18F2455 family behind a
  fixed contract (Phases 0-4 done, litmus test met).
- [docs/adding-a-device.md](docs/adding-a-device.md), the operational,
  verification-gated guide for adding a new device variant or a new family.
- [docs/ci-plan.md](docs/ci-plan.md), the CI build/test matrix design and
  its phase-by-phase findings.
- Datasheet references (not vendored in this repo) are listed under
  [License](#license).

## Requirements

- **Host simulation**: CMake >= 3.16 and any C99 compiler (gcc/clang).
- **Real target**: MPLAB X IDE v6.x and MPLAB XC8 v3.x (`xc8-cc`). Tested
  on a PIC16F877A and PIC18F4550 (see [Supported devices](#supported-devices)).
  The PIC18 build needs the PIC18Fxxxx Device Family Pack installed
  separately (the PIC16Fxxx DFP ships with XC8, see
  [pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md](pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md)).

## Development

`./scripts/bootstrap.sh` sets up a fresh clone: installs the host
toolchain the CMake builds need (`cmake`, `cppcheck`, ...) and a
pre-commit hook (trailing newline/whitespace, no-em-dash, `cppcheck` on
staged `.c` files). `--check-only` reports what's missing without
installing anything. See [scripts/README.md](scripts/README.md) for what
the hook checks and why `clang-format` isn't part of it yet.

### Docker quick start

Prefer not to install XC8/MPLAB X/CMake on your own machine? The root
`Makefile` runs the whole workflow, host tests, real-target XC8 builds,
the `mdb` (MPLAB SIM) verification gate, and a dev shell, inside the same
toolchain image CI uses (`docker/ci-toolchain/`). See
[docs/docker-dev-plan.md](docs/docker-dev-plan.md) for the design.

**One-time, human-only step**: download two files from Microchip's site
(a browser, not a script, myMicrochip login may be prompted) and drop
them in `docker/ci-toolchain/vendor/`:

- the XC8 v4.00 Linux installer, as `xc8-installer.run`, from
  https://www.microchip.com/mplab/compilers
- the MPLAB X IDE v6.35 Linux installer, tar'd up as a single
  `mplabx-installer.tar`, from https://www.microchip.com/mplab/mplab-x-ide

Neither can be fetched by a script or an agent: Microchip's download CDN
sits behind an Akamai bot-challenge that returns a 403 to any
non-browser client (confirmed, not assumed, see
[docs/ci-plan.md](docs/ci-plan.md) and
[docs/docker-dev-plan.md](docs/docker-dev-plan.md)). If an agent is
driving this and hits that wall, it should stop and ask you for the
files, not keep retrying.

```sh
make check-vendor    # confirms both files are present and correctly sized

make image            # build the toolchain image locally (once; cached after)
make test             # host-sim build + test, every module
make test MODULE=pic8-lcd   # ... or just one

make xc8-build MODULE=pic16f193x-hal MCU=16F1937   # real-target build
make mdb-test MODULE=pic8-tick/mcu/pic16f87xa-tick-mplabx \
  MCU=16F877A DEVICE=PIC16F877A DFP=Microchip.PIC16Fxxx_DFP  # the mdb gate

make shell             # interactive shell, repo mounted at /repo
```

Maintainers with `write:packages` access to this repo's GHCR packages can
publish an updated toolchain image with `make ci-image-push
GHCR_OWNER=<owner>` (after `docker login ghcr.io`), the same private tag
CI's workflows pull from; CI itself never builds this image.

## License

MIT, see [LICENSE](LICENSE).

The Microchip datasheets
[DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
(c) 2003 Microchip Technology Inc. and
[DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)
(c) 2009 Microchip Technology Inc. are vendor documentation; register and
bit names in the code follow them directly. They are not vendored in this
repo, follow the links above to Microchip's own hosted copies.
