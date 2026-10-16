<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/epic-hal-logo-dark-mode.svg">
    <img src="docs/assets/epic-hal-logo-light-mode.svg" width="120" alt="epic-hal logo: a chip-temple inside a laurel wreath">
  </picture>
</p>

# epic-hal

epic-hal is a hardware abstraction layer (HAL) for 8-bit PIC
microcontrollers. You call one set of C functions, like `EPIC_GPIO_Init` or
`epic_tick_get`, and they work the same on every supported PIC family.
Underneath, each family has a small port that drives that chip's registers.
On top sit ready-made modules (serial, timers, ADC, LCD, Modbus, PID and
more) that use only the HAL, so code you write once moves to another chip by
changing an include path.

It's built for small parts: no RTOS, no dynamic memory, buffer sizes you set
at compile time, and only the modules you add get compiled. MIT licensed,
and it builds with epic-cc or XC8.

[![CI](https://github.com/apojomovsky/epic-hal/actions/workflows/ci.yml/badge.svg)](https://github.com/apojomovsky/epic-hal/actions/workflows/ci.yml) [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) [![Release](https://img.shields.io/github/v/release/apojomovsky/epic-hal)](https://github.com/apojomovsky/epic-hal/releases)

## Why epic-hal

- **One API across PIC families.** A HAL (hardware abstraction layer) hides each chip's registers behind the same function names, so code for one family builds for the others with only the include path changed.
- **Drop-in modules.** Timebase, serial, scheduler, Modbus, PID and more are folders of `.c` files you add to a build. No RTOS, no dynamic allocation.
- **Tested before silicon.** Every module builds as a host program, and CI runs real-target builds in the simulator, checking registers and UART output.

**Status:** Ready for projects on the supported parts. See [What's in 1.0](https://apojomovsky.github.io/epic-docs/status/).

## The shortest example

Blink an LED on pin RB0 of a PIC16F877A, driven by the 1 ms timebase module:

```c
/*
 * Epic HAL reference project, PIC16F87XA: RB0 toggles every 500 ms
 * off the 1 ms tick, the minimal build smoke test. See MPLABX.md to add
 * Epic HAL to an existing project.
 */
#include <xc.h>

#include "epic_tick.h"
#include "peripherals/pic16f87xa_gpio.h"

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

/**
 * @brief Toggle RB0 every 500 ms using the 1 ms lib/tick timebase.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_tick_init(FOSC_HZ);

    uint32_t last = epic_tick_get();
    for (;;)
    {
        if ((epic_tick_get() - last) >= 500u)
        {
            last = epic_tick_get();
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
    }
}
```

The installer below scaffolds a blink like this for your part. Build it from its directory:

```bash
make
```

The hex file lands under `build/`. Program it with a PICkit or similar programmer. With a 20 MHz crystal on the oscillator pins and an LED plus resistor from RB0 to ground, the LED toggles every 500 ms.

## Install

The installer downloads the HAL bundle for your part, verifies its checksum and writes `third_party/epic-hal/`, a `Makefile`, a working `main.c` and an MPLAB X project (`myapp.X`) into the current directory. The installer needs `python3`. To build, put [epic-cc](https://github.com/apojomovsky/epic-cc/releases), the open source compiler, on your `PATH`; no Microchip download is needed. Pass `--modules` to choose which modules to add.

```bash
curl -fsSL https://github.com/apojomovsky/epic-hal/releases/latest/download/install.sh | sh -s -- 16F877A
```

## Ways to use epic8

- **Command line and Make:** compile with epic-cc and build with epic-hal's Makefiles. Start at [Getting started: command line](https://apojomovsky.github.io/epic-docs/getting-started/cli/).
- **PlatformIO:** add the epic8 platform to a `platformio.ini`. Start at [Getting started: PlatformIO](https://apojomovsky.github.io/epic-docs/getting-started/platformio/).

## Where next

- [Getting started](https://apojomovsky.github.io/epic-docs/getting-started/): your first blink, in the simulator and on a chip.
- [Guides](https://apojomovsky.github.io/epic-docs/guides/): how to do specific things.
- Reference: every [part family](docs/reference/families.md), every [module](docs/reference/modules.md) and the [XC8 device packs and release bundles](docs/reference/packs.md), generated from the manifest.
- [How it works](https://apojomovsky.github.io/epic-docs/explanation/): the design, for the curious.

## Contributing and licence

Contributions are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md) and [Contributing](https://apojomovsky.github.io/epic-docs/contributing/). epic-hal is MIT licensed; see [LICENSE](LICENSE).
