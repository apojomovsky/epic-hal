# MPLAB X / XC8 project template

This directory holds a minimal MPLAB X IDE project that builds the
PIC16F87XA HAL for a real PIC16F877A target.

## What works on real silicon

The HAL compiles on XC8 because:

- The SFR macros resolve to volatile direct-access on real targets via
  `include/target/pic16f87xa_platform.h` (the Makefile puts
  `include/target` ahead of `include` on the include path).
- `pic16f87xa_sim.c` and the host-side harness / WDT-sleep
  implementations are simply not in this build's source list, XC8 never
  links them.
- The weak ISRs (`TIMER0_IRQHandler`, `ADC_IRQHandler`, …) and the
  `clrwdt` / `sleep` asm helpers compile to native instructions on
  XC8.

## Using this project

### Command line

```sh
# Adjust EPIC_DIR if you place the firmware project differently.
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

# Build for PIC16F877A (default).
make

# Build for any other part.
make MCU=16F874A    # 40/44-pin, smaller flash
make MCU=16F876A    # 28-pin, larger flash
make MCU=16F873A    # 28-pin, smaller flash
```

The result is `build/16F877A-firmware.hex`. Program it with your
favourite tool (`ippe`, `PK2CMD`, MPLAB IPE, …).

### MPLAB X IDE

The `nbproject/configurations.xml` file describes a "Standalone
Project" that includes all HAL sources plus `tests/example_blink.c`
as the application entry point.

1. **File → Open Project…** in MPLAB X.
2. Point at this directory; the IDE will detect the project.
3. Confirm the device (PIC16F877A) and toolchain (XC8 v3.10).
4. Make and Program Device.

If MPLAB X complains about the project not being an MPLAB X project
(it is), you can also:

1. **File → New Project → Microchip Embedded / Standalone Project**.
2. Pick **PIC16F877A** as the device and **XC8** as the toolchain.
3. Right-click the *Source Files* folder → **Add Existing Items** and
   pick all the .c files under `../../src/` and one application .c
   from `../../tests/`.
4. Right-click the *Header Files* folder → **Add Existing Items** and
   pick all the .h files under `../../include/`.
5. Add `../../include` to *Project Properties → XC8 Compiler → Include
   Directories* so `#include "pic16f87xa.h"` works.

## Adjusting for your board

The configuration word in the `Makefile` (and the `nbproject` project
properties) sets:

- FOSC = HS (high-speed crystal, ≤ 20 MHz)
- WDTE = ON (Watchdog Timer enabled, refresh via `EPIC_WDT_Refresh()`)
- PWRTE = ON (Power-up Timer, 72 ms)
- BOREN = ON (Brown-out Reset at 4.0 V)
- LVP = OFF (low-voltage programming disabled)
- WRT = OFF (flash write protection off)

Change these for your hardware. The full Configuration Word layout is
DS39582B §14.1, Register 14-1.
