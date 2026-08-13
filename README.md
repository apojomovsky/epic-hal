<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/epicurus-logo-dark-mode.svg">
    <img src="docs/assets/epicurus-logo-light-mode.svg" width="120" alt="Epicurus logo: a chip-temple inside a laurel wreath">
  </picture>
</p>

<h1 align="center">Epicurus</h1>

<p align="center"><em>Built down to what the datasheet requires.</em></p>

<p align="center">

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) [![Toolchain: MPLAB XC8](https://img.shields.io/badge/toolchain-MPLAB%20XC8-green.svg)](https://www.microchip.com/mpgb/xc8.html) [![Release](https://img.shields.io/github/v/release/apojomovsky/epicurus)](https://github.com/apojomovsky/epicurus/releases) [![ci](https://github.com/apojomovsky/epicurus/actions/workflows/ci.yml/badge.svg)](https://github.com/apojomovsky/epicurus/actions/workflows/ci.yml)

</p>

A register-level HAL and a shelf of drop-in modules for 8-bit PIC
microcontrollers. Download the bundle for your family, open the
reference MPLAB X project, and you are building against a
datasheet-faithful driver layer with one API across three PIC families,
plus the things firmware always needs: a scheduler, a 1 ms timebase,
UART and bit-banged serial, Modbus, PID, fixed-point math, and more.

## Getting started (one command)

    curl -fsSL https://github.com/apojomovsky/epicurus/releases/latest/download/install.sh | sh -s -- pic16f87xa

That downloads the PIC16F87XA bundle, verifies its checksum, and scaffolds
a project in your current directory. It leaves you with:

- `third_party/epicurus/`: the vendored library, pinned to the version you got,
- `myapp.X`: a ready MPLAB X project for the part and modules you picked,
- `Makefile` and `main.c`: a working blink.

Then either build it or open it:

    make

That needs MPLAB XC8 (the free tier is enough); the installer tells you if
`xc8-cc` is already on your PATH. Or open `myapp.X` in MPLAB X or the MPLAB
extension for VS Code and Build there.

The scaffolder is a plain Python 3 script (stdlib only), so the one-liner
also needs `python3` on PATH; the installer checks for it and tells you how
to finish by hand if it is missing.

The other families: replace `pic16f87xa` with `pic18fxx5x` or `pic16f193x`.
List them with `... | sh -s -- --list`, pin a release with
`... | sh -s -- pic16f87xa v0.1.0`, and override the defaults with
`--part` (e.g. `16F873A`) and `--modules` (e.g. `serial`). The installer
refuses to clobber an existing `third_party/epicurus` unless you pass
`--force`, and `EPICURUS_DIR` changes where it lands.

Prefer to inspect before running? Download the script, read it, then run it:

    curl -fsSL -o install.sh https://github.com/apojomovsky/epicurus/releases/latest/download/install.sh
    less install.sh && sh install.sh pic16f87xa

## What you get

- **One API, three families.** The same names and signatures on
  [PIC16F87XA](pic16f87xa-hal/), [PIC18F2455](pic18fxx5x-hal/), and
  [PIC16F193X](pic16f193x-hal/). Each family HAL implements the contract
  over its own registers, every bit cited to Microchip's datasheet.
  Code written against one builds against the others unchanged.
- **No framework tax.** Plain C99, static storage, no RTOS, no dynamic
  allocation, no C++. A module is a folder of `.c` files you can read.
- **Logic proven before silicon.** Every module also builds and runs as
  a host program, and CI cross-compiles everything for real parts and
  runs it under MPLAB SIM, checking actual registers and UART output.
- **One command to a `.hex`.** `curl ... | sh -s -- <family>` downloads,
  verifies, and scaffolds a project; build with `make` or MPLAB X.

## Not using the one-liner?

### 1. Download a bundle and run `epicurus init`

Bundles live on the [Releases](https://github.com/apojomovsky/epicurus/releases)
page:

| Bundle | Parts inside |
|---|---|
| `epicurus-pic16f87xa-<version>.tar.gz` | 16F873A / 874A / 876A / 877A |
| `epicurus-pic18fxx5x-<version>.tar.gz` | 18F2455 / 2550 / 4455 / 4550 |
| `epicurus-pic16f193x-<version>.tar.gz` | 16F1933 / 1934 / 1936 / 1937 / 1938 / 1939 |

The `<version>` is the release tag (e.g. `v0.1.0`); the badge above
always shows the latest one.

Download and unpack one, then from your project directory:

    /path/to/epicurus/epicurus init --bundle /path/to/epicurus

Answer family, part, and modules. It writes `main.c`, a filled `Makefile`,
and a ready MPLAB X `.X` in your current directory for your exact part +
module subset, with the bundle vendored wherever you pointed `--bundle`.
Open `myapp.X` in MPLAB X (or the MPLAB extension for VS Code) and Build,
or `make`. Or, with the CLI installed globally:
`pipx install git+https://github.com/apojomovsky/epicurus`, then
`epicurus init --bundle path/to/bundle`.

## Advanced: without the scaffolder

Prefer to wire a project by hand, or add Epicurus to one you already
have? These two paths skip the scaffolder.

### Open the reference project in MPLAB X

Unpack the bundle, then open `examples/epicurus-demo.X` (File > Open
Project). Pick your exact part under Project Properties, and Build. It
produces a `.hex` you can program with MPLAB IPE or any PICkit.

<details>
<summary>New to MPLAB X?</summary>

You need MPLAB X IDE and the MPLAB XC8 compiler, both free from
Microchip (the free XC8 tier is enough). The reference project is
pre-wired: sources, include paths, and configuration words are already
set. Selecting your part under Project Properties is the only manual
step.
</details>

### Or skip the IDE: a six-line Makefile

Just `xc8-cc` and `make`, no MPLAB X and no license:

```make
EPICURUS_DIR := third_party/epicurus
EPICURUS_MCU := 16F877A
EPICURUS_MODULES := serial tick
include $(EPICURUS_DIR)/epicurus.mk

SRCS := main.c $(EPICURUS_SRCS)
CFLAGS += $(EPICURUS_CFLAGS)

app.hex: $(SRCS)
	xc8-cc $(CFLAGS) $^ -o $@ -ginhx32
```

Run `make`, program the result. Dependencies resolve automatically
(`modbus` pulls in `serial` and `tick`), and asking for a module on a
part it does not fit fails immediately with the reason instead of a
wall of XC8 linker errors. Each bundle's `SUPPORT.md` has the full
per-part table.

Adding Epicurus to an existing MPLAB X project instead? The bundle's
`MPLABX.md` walks through it.

## What the API feels like

Four programs, each complete. The same source builds against any of the
three families: swap the include path at build time, nothing else
changes.

### Blink a LED on a 1 ms timebase

```c
#include <xc.h>
#include "epic_tick.h"
#include "peripherals/pic16f87xa_gpio.h"

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_tick_init(FOSC_HZ);            /* FOSC_HZ comes from the build */

    uint32_t last = epic_tick_get();
    for (;;) {
        if (epic_tick_get() - last >= 500u) {
            last = epic_tick_get();
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
    }
}
```

`epic_tick_init` sets up a Timer2 auto-reload interrupt that keeps a
millisecond counter; `epic_tick_get` reads it. This is the reference
project's `main.c` in full, the smallest thing that proves your toolchain
is wired up.

### Echo bytes over UART

```c
#include <xc.h>
#include "epic_serial.h"

int main(void)
{
    epic_serial_init(FOSC_HZ, 115200u);

    uint8_t buf[16];
    for (;;) {
        int n = epic_serial_read(buf, sizeof buf);
        if (n > 0) {
            epic_serial_write(buf, n);
        }
    }
}
```

`epic_serial` is interrupt-driven with a ring buffer; it also retargets
`printf` through the same pipe. Everything the code above calls is
plain C functions, no IDE glue.

### Run two tasks on a cooperative scheduler

```c
#include <xc.h>
#include "epic_hal.h"
#include "epic_taskmgr.h"

static void toggle(void *arg)
{
    EPIC_GPIO_TogglePin(GPIOB, (uint16_t)(uintptr_t)arg);
}

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_OUTPUT);
    epic_taskmgr_init();

    epic_taskmgr_spawn(toggle, (void *)(uintptr_t)GPIO_PIN_0, 100u, 0u);
    epic_taskmgr_spawn(toggle, (void *)(uintptr_t)GPIO_PIN_1, 300u, 0u);

    epic_taskmgr_attach_timer0(61u, TIMER0_PRESCALER_1_256); /* ~10 ms tick */
    EPIC_IRQ_Restore(1);                                     /* arm IRQs    */
    epic_taskmgr_run();                                      /* never returns */
}
```

`epic_taskmgr` is a priority-ordered, race-free cooperative scheduler:
periodic and one-shot tasks, `EPIC_TASKMGR_MAX_TASKS` fixed slots, no
per-task stack. The 10-line core of
[example_multi_blink](epic-taskmgr/examples/example_multi_blink.c).

### Oversample and average an ADC channel

```c
#include <xc.h>
#include "peripherals/pic16f87xa_adc.h"
#include "epic_adcfilter.h"

static uint16_t read_ch3(void *ctx)
{
    (void)ctx;
    EPIC_ADC_SelectChannel(ADC_CHANNEL_AN3);
    EPIC_ADC_Start();
    while (EPIC_ADC_IsConversionInProgress()) { }
    return EPIC_ADC_Read();
}

int main(void)
{
    ADC_HandleTypeDef adc = ADC_HANDLE_DEFAULT;
    EPIC_ADC_Init(&adc);

    static uint16_t buf[8];
    epic_adcfilter_avg_t avg;
    epic_adcfilter_avg_init(&avg, buf, 8u);

    for (;;) {
        uint16_t v = epic_adcfilter_avg_push(
            &avg, epic_adcfilter_oversample(read_ch3, NULL, 1u));
        (void)v;
    }
}
```

`epic_adcfilter` decimates the raw samples and keeps an O(1) moving
average, both over a callback you provide. The HAL layer is just
select, start, poll, read.

## What you can build

Modules are grouped by what they do for you. Everything below is
family-agnostic unless noted; each ships its own README, and the HALs
carry a datasheet-cited register reference.

**Timing & control**

| Module | What it does |
|---|---|
| [epic-tick](epic-tick/) | 1 ms timebase on a Timer2 auto-reload ISR (`HAL_GetTick` equivalent). |
| [epic-taskmgr](epic-taskmgr/) | Cooperative scheduler: periodic and one-shot tasks, priority-ordered, race-free. |
| [epic-debounce](epic-debounce/) | Instantiable digital-input debouncer on the real timebase. |
| [epic-encoder](epic-encoder/) | Interrupt-driven x4 quadrature decoder, instantiable. |
| [epic-fsm](epic-fsm/) | Table-driven finite state machine, the whole machine is one `static const` table. |
| [epic-pid](epic-pid/) | Fixed-point (Q8.8) PID with anti-windup, derivative-on-measurement, bumpless auto/manual. |
| [epic-adcfilter](epic-adcfilter/) | ADC oversample-and-decimate plus an O(1) moving-average filter. |

**Communication**

| Module | What it does |
|---|---|
| [epic-serial](epic-serial/) | Interrupt-driven ring-buffered UART + `printf` retarget. |
| [epic-swuart](epic-swuart/) | Bit-banged software UART on CCP capture/compare, two channels on PIC16F193X. |
| [epic-bus](epic-bus/) | I2C/SPI register-access idiom on top of MSSP/SSP. |
| [epic-modbus](epic-modbus/) | Modbus RTU slave: core function codes, T3.5 framing, CRC-16, RS-485 driver-enable. |
| [epic-console](epic-console/) | Line-based serial command dispatcher over `epic-serial`. |
| [epic-usb](epic-usb/) | USB CDC-ACM virtual serial port. PIC18Fxx5x only. |

**Storage**

| Module | What it does |
|---|---|
| [epic-settings](epic-settings/) | EEPROM-backed settings blobs with CRC-16 validation and first-boot defaults. |
| [epic-sdcard](epic-sdcard/) | SD/MMC over SPI block storage. PIC18Fxx5x only (RAM constraint). |

**Math**

| Module | What it does |
|---|---|
| [epic-math](epic-math/) | Fixed-point math: multiply, divide, BCD, sqrt, numerical diff/integration, RNGs, with PIC16/PIC18 inline-asm backends behind one API. |

**Peripherals**

| Module | What it does |
|---|---|
| [epic-lcd](epic-lcd/) | HD44780-compatible character LCD: 4-bit GPIO, 8-bit GPIO, or SPI via 74HC595. |

The full catalog, including the three HALs and `epic-common`, lives in
the table below. The higher-level modules build against the two mature
families today; wiring them to PIC16F193X is in progress now that its
peripheral coverage is complete.

## Supported devices

| Family | Parts | HAL | Peripheral coverage |
|---|---|---|---|
| PIC16F87XA | 16F873A / 874A / 876A / 877A | [pic16f87xa-hal](pic16f87xa-hal/) | GPIO, Timers 0-2, CCP, MSSP, EUSART, ADC, Comparator, EEPROM, PSP, WDT |
| PIC18F2455 | 18F2455 / 2550 / 4455 / 4550 | [pic18fxx5x-hal](pic18fxx5x-hal/) | GPIO, Timers 0-3, ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP |
| PIC16F193X | 16F1933 / 1934 / 1936 / 1937 / 1938 / 1939 | [pic16f193x-hal](pic16f193x-hal/) | GPIO, Timers 0/1/2/4/6, CCP1-5, EUSART, MSSP, ADC, Comparator, EEPROM, DAC, FVR, SR latch, CPS, LCD |

## Documentation

- [epic-common/MANUAL.md](epic-common/MANUAL.md): the shared conventions,
  the handle pattern, the harness, the interrupt model. Read this first.
- Per-family `MANUAL.md`:
  [PIC16F87XA](pic16f87xa-hal/MANUAL.md),
  [PIC18Fxx5x](pic18fxx5x-hal/MANUAL.md),
  [PIC16F193X](pic16f193x-hal/MANUAL.md): datasheet-cited register
  reference, one page per peripheral.
- [epic-common/README.md](epic-common/README.md) +
  [epic-common/MANUAL.md](epic-common/MANUAL.md): how the shared
  contract was extracted and the PIC18F2455 family added behind it.

## Contributing

Bug reports, datasheet-cited corrections, and new devices are welcome.
The repo is agent-friendly and plan-first: non-trivial work starts with
a short-lived design doc (deleted on completion), and everything is
verified by the CI pipeline (host tests, real XC8 cross-compiles,
MPLAB SIM runs).
See [AGENTS.md](AGENTS.md) for the conventions and
[DEVELOPMENT.md](DEVELOPMENT.md) for the toolchain and build workflow.

## License

MIT, see [LICENSE](LICENSE). The Microchip datasheets and application
notes this library is built from are Microchip's property and are not
vendored in this repository; links to Microchip's own hosted copies are
listed in each module's `MANUAL.md`.
