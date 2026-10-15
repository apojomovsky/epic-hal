<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/epic-hal-logo-dark-mode.svg">
    <img src="docs/assets/epic-hal-logo-light-mode.svg" width="120" alt="epic-hal logo: a chip-temple inside a laurel wreath">
  </picture>
</p>

# epic-hal

A register-level HAL plus drop-in modules for 8-bit PIC microcontrollers, fully open source and MIT licensed.

[![CI](https://github.com/apojomovsky/epic-hal/actions/workflows/ci.yml/badge.svg)](https://github.com/apojomovsky/epic-hal/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) [![Release](https://img.shields.io/github/v/release/apojomovsky/epic-hal)](https://github.com/apojomovsky/epic-hal/releases)

## Why epic-hal

- **Fully open source, MIT licensed.** The HAL, the modules and the build scripts are plain C99 and Python you can read, change and ship.
- **One API across PIC families.** A HAL (hardware abstraction layer) hides each chip's registers behind the same function names, so code for one family builds for the others with only the include path changed.
- **Drop-in modules.** Timebase, serial, scheduler, Modbus, PID and more are folders of `.c` files you add to a build. No RTOS, no dynamic allocation.
- **Tested before silicon.** Every module builds as a host program, and CI runs real-target builds in the simulator, checking registers and UART output.

**Status:** Ready for hobby projects on the supported parts. See [What's in 1.0](https://apojomovsky.github.io/epic-docs/status/).

## The shortest example

Blink an LED on pin RB0 of a PIC16F877A, driven by the 1 ms timebase module:

```c
--8<-- "examples/epic-hal-demo-pic16f87xa.X/main.c"
```

The installer below scaffolds a blink like this for your part. Build it from its directory:

```bash
--8<-- "docs/snippets/build.sh"
```

The hex file lands under `build/`. Program it with a PICkit or similar programmer. With a 20 MHz crystal on the oscillator pins and an LED plus resistor from RB0 to ground, the LED toggles every 500 ms.

## Install

The installer downloads the HAL bundle for your part, verifies its checksum and writes `third_party/epic-hal/`, a `Makefile`, a working `main.c` and an MPLAB X project (`myapp.X`) into the current directory. The installer needs `python3`. To build, put [epic-cc](https://github.com/apojomovsky/epic-cc/releases), the open source compiler, on your `PATH`; no Microchip download is needed. Pass `--modules` to choose which modules to add.

```bash
--8<-- "docs/snippets/install.sh"
```

--8<-- "docs/snippets/ways-to-use-epic8.md"

## Where next

- [Getting started](https://apojomovsky.github.io/epic-docs/getting-started/): your first blink, in the simulator and on a chip.
- [Guides](https://apojomovsky.github.io/epic-docs/guides/): how to do specific things.
- Reference: every [part family](docs/reference/families.md), every [module](docs/reference/modules.md) and the [XC8 device packs and release bundles](docs/reference/packs.md), generated from the manifest.
- [How it works](https://apojomovsky.github.io/epic-docs/explanation/): the design, for the curious.

## Contributing and licence

Contributions are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md) and [Contributing](https://apojomovsky.github.io/epic-docs/contributing/). epic-hal is MIT licensed; see [LICENSE](LICENSE).
