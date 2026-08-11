# PIC16F87XA HAL

Hardware abstraction layer for the **PIC16F873A / 874A / 876A / 877A** family,
inspired by the STM32Cube HAL API. Every constant, register address and
behaviour is taken 1-to-1 from the datasheet [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf).

➜ **[MANUAL.md](MANUAL.md)** is the full, human-readable manual:
architecture, both builds, the simulator, the harness, and a per-peripheral
reference. Start there if you are new to the HAL.

## XC8 codegen gotchas (settled)

Banked SFR access must go through the `EPIC_BANKn_READ8/WRITE8` (and PIE)
inline-asm macros, never plain C: under XC8 v4.00 a banked C
read/write/RMW misdirects to the Bank 0 alias. Load the operand into W
before the switch; the asm selects the bank absolutely and restores Bank
0 on exit. The `__interrupt` entry normalizes to Bank 0 before dispatch,
so an interrupt taken mid-window never runs in the wrong bank.
`i1_`-prefixed symbols in `.s`/`.map` are XC8's per-call-graph function duplication, expected.

## Status

Done so far:

- ✅ Family header (device select, status codes, SFR macros)
- ✅ SFR map with bit definitions
- ✅ Pre-processor SFR mapping layer (host sim + XC8 target, same source)
- ✅ Host simulation backend (Timer0/1/2 + GPIO + IRQ hook)
- ✅ Interrupt core (EPIC_NVIC-style enable/disable/clear/status)
- ✅ GPIO driver (GPIOA..GPIOE, pull-ups, init/read/write/toggle)
- ✅ **Timer0 driver** (handle, prescaler, clock source, weak ISR)
- ✅ **Timer1 driver** (16-bit, prescaler, T1OSC, weak ISR)
- ✅ **Timer2 driver** (PR2, prescaler, postscaler, weak ISR)
- ✅ **CCP1 / CCP2 driver** (Capture / Compare / PWM, weak ISRs)
- ✅ **USART driver** (async + sync, SPBRG baud-rate computation, weak ISRs)
- ✅ **MSSP driver** (SPI master/slave + I²C master/slave, register-level)
- ✅ **ADC driver** (10-bit, 5/8 channels, TAD clock + reference config, weak ISR)
- ✅ **Comparator driver** (2 comparators, 8 modes, weak ISR)
- ✅ **Vref driver** (16-tap ladder, low/high range, mV helper)
- ✅ **EEPROM driver** (read/write/buffer, mandatory 0x55/0xAA unlock, weak ISR)
- ✅ **PSP driver** (40/44-pin only, TRISE PSPMODE, IBF/OBF/IBOV flag helpers)
- ✅ **WDT / BOR / Sleep helpers** (clrwdt / sleep asm, PCON BOR/POR flags)
- ✅ **Shared interrupt dispatch** (`epic_dispatch_all_irqs`), fans the single vector at 0x0004 out to every peripheral IRQHandler. On a real target the XC8 `__interrupt()` (`src/core/pic16_isr_vector.c`) calls it; on the host the harness registers it as the sim IRQ callback. One source of truth.
- ✅ **Test/firmware harness** (`epic_harness.h`, in the shared `epic-common/` layer, included as `core/epic_harness.h`), lets every example build for the host sim and a real XC8 target with **no `#ifdef` in the example code**. The build links the host harness (`pic16_harness_sim.c`, this tree) or the target harness (`epic_harness_target.c`, family-blind no-ops shared in `epic-common/`); the harness abstracts the only two execution-model differences (pumping simulated time vs. real time, terminating test vs. firmware that runs forever).
- ✅ **MPLAB X / XC8 project template** (Makefile + `nbproject/configurations.xml`)
- ✅ End-to-end tests: `example_blink`, `example_idle_blink`, `example_timer1`, `example_timer2`, `example_ccp_pwm`, `example_usart`, `example_ssp`, `example_adc`, `example_comp_vref`, `example_eeprom`, `example_psp`, `example_wdt_sleep`

**All planned peripherals + XC8 build glue done.**

## Layout

```
pic16f87xa-hal/
├── include/
│   ├── pic16f87xa.h              Family header, device selection, SFR mapping
│   │                             macros; pulls in the shared status codes /
│   │                             bit helpers from epic-common/hal_status.h
│   ├── pic16f87xa_sfr.h          SFR address map + bit names (1-to-1 with DS39582B)
│   ├── pic16f87xa_sim.h          Simulation backend public API
│   ├── core/                     CPU-level features (interrupts, config bits)
│   └── peripherals/             One .h per peripheral, Cube-style API
├── src/
│   ├── core/                     Implementation of core/ headers
│   ├── peripherals/             Implementation of peripherals/ headers
│   └── sim/                     Host simulation backend
├── tests/                        End-to-end smoke tests
└── CMakeLists.txt                Host build (gcc + cmake)
```

## Build (host simulation)

```sh
cmake -B build -S .
cmake --build build

./build/example_blink
./build/example_blink_PIC16F873A
./build/example_blink_PIC16F874A
./build/example_blink_PIC16F876A
./build/example_blink_PIC16F877A
```

## Build (real target)

The HAL compiles against XC8 via the same sources. The host/target split
is done at **build time, not with `#ifdef`**: the XC8 Makefile puts
`include/target` ahead of `include` on the include path, so
`pic16f87xa_platform.h` resolves to the real-target version (SFR macros
= direct volatile dereference) and the target-side harness / WDT-sleep /
interrupt-vector implementations are linked. The MPLAB X project lives
under `mcu/pic16f87xa-mplabx/`; its Makefile produces `<MCU>-firmware.hex`
via `xc8-cc`.

## The simulation middleware

The same source file works on host and on-target with **no `#ifdef` around
code**: the build selects the platform by include path, and the
execution-model differences by which harness implementation it links.

`include/pic16f87xa.h` pulls in `pic16f87xa_platform.h`, which exists in
two same-named copies:

```c
/* include/host/pic16f87xa_platform.h  (CMake build) */
extern uint8_t pic16f87xa_sim_sfr[0x200];
#define epic_sfr_read8(addr)   (pic16f87xa_sim_sfr[(uint16_t)(addr)])
#define epic_sfr_write8(addr, v) pic16f87xa_sim_sfr[(uint16_t)(addr)] = (v)

/* include/target/pic16f87xa_platform.h  (XC8 build) */
#define epic_sfr_read8(addr)   (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) (*(volatile uint8_t *)(uintptr_t)(addr)) = (v)
```

CMake puts `include/host` first on the include path; the XC8 Makefile puts
`include/target` first, so the one `#include "pic16f87xa_platform.h"` in
`pic16f87xa.h` resolves to the right copy per build, with no preprocessor
branching.

Likewise the test/firmware harness (`epic_harness.h`, shared via
`epic-common/`) and the WDT/Sleep helpers are each provided by two
implementation files: a host `*_sim.c` linked by CMake and a target
`*_target.c` linked by the XC8 Makefile, so examples call
`epic_harness_init/tick/running/log`, `EPIC_WDT_Refresh`,
`EPIC_Sleep_Enter` unconditionally and the build picks the behaviour. The
harness's target implementation is family-blind (four no-ops) and lives
in `epic-common/`; the harness's host implementation (`pic16_harness_sim.c`)
and the WDT/Sleep pair stay in this tree. See `core/epic_harness.h` for
the rationale.

On host, every SFR read/write is a memory access into a 512-byte array
(`pic16f87xa_sim_sfr`). On a real PIC, the same macro dereferences the
literal address, exactly the same code the XC8 linker would generate
from `*(volatile uint8_t *)&PORTB`.

The simulation backend additionally:

- Implements Timer0 with prescaler and overflow interrupt.
- Models PORTA..PORTE read-modify-write semantics (writes only update
  the latch; reads depend on TRIS).
- Lets the host application drive external pin levels
  (`pic16f87xa_sim_drive_input`) and observe what the chip is
  driving (`pic16f87xa_sim_read_output`).
- Forwards simulated interrupts to the shared dispatcher
  (`epic_dispatch_all_irqs`, registered by the host harness).

This means **the same `tests/example_blink.c` compiles unchanged for
host simulation or for an XC8-built firmware targeting a real PIC**.

## Datasheet citations

Every register, bit name, reset value and reset condition is annotated
with the section of DS39582B it came from. Examples:

- `pic16f87xa_sfr.h` → `/* DS39582B Table 4-3, PORTB functions */`
- `pic16f87xa_gpio.c` → `EPIC_GPIO_Init()` cites §4.x
- `core/pic16_irq.h` → every IRQn cites §14.11

Any deviation from the datasheet is a bug.

## API conventions

Mirror STM32Cube as closely as makes sense for an 8-bit PIC:

- `HAL_PPP_Init(handle, init_struct)`, peripheral bring-up
- `HAL_PPP_DeInit(handle)`, peripheral tear-down
- `HAL_PPP_MspInit()`, `__weak` callback the user overrides to wire ISR vectors
- Pin/port addressing: `GPIOA..GPIOE`, `GPIO_PIN_0..GPIO_PIN_All`,
  `GPIO_PIN_SET/RESET`
- Status codes: `EPIC_OK / ERROR / BUSY / TIMEOUT / INVALID`
- IRQ enum: `PIC16_IRQ_TMR0 / _TMR1 / _CCP1 / ...`

Cube users get a familiar API; PIC users get familiar peripherals with a
consistent abstraction.
