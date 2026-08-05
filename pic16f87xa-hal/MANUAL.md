# PIC16F87XA HAL, Manual

A hardware abstraction layer for the **PIC16F873A / 874A / 876A / 877A**
family, modelled on the STM32Cube HAL and verified against the Microchip
datasheet **DS39582B**. Every constant, register address, bit name and
reset value in this HAL is taken 1:1 from that datasheet; each header cites
the section it implements. If the HAL and the datasheet disagree, the HAL
is wrong, please treat that as a bug.

This manual covers what's genuinely specific to this family: the device
variants, the peripheral register reference, the SFR map, and this
family's own known gaps. **Read
[`epic-common/MANUAL.md`](../epic-common/MANUAL.md) first**, naming
conventions, the handle pattern, status codes, the harness, the host/
target build-time split, the host-sim API shape, and the shared interrupt
model are explained there once and apply here unchanged.

---

## Contents

1. [What this is](#1-what-this-is)
2. [The big picture](#2-the-big-picture)
3. [Quick start](#3-quick-start)
4. [Build systems](#4-build-systems)
5. [Datasheet citations](#5-datasheet-citations)
6. [What the simulator models](#6-what-the-simulator-models)
7. [Interrupts](#7-interrupts)
8. [Core: WDT, Sleep, BOR/POR](#8-core-wdt-sleep-borpor)
9. [GPIO](#9-gpio)
10. [Timer0](#10-timer0)
11. [Timer1](#11-timer1)
12. [Timer2](#12-timer2)
13. [CCP1 / CCP2, Capture, Compare, PWM](#13-ccp1--ccp2--capture-compare-pwm)
14. [USART](#14-usart)
15. [MSSP, SPI and I²C](#15-mssp--spi-and-i²c)
16. [ADC](#16-adc)
17. [Comparator](#17-comparator)
18. [Voltage reference (Vref)](#18-voltage-reference-vref)
19. [Data EEPROM](#19-data-eeprom)
20. [Parallel Slave Port (PSP)](#20-parallel-slave-port-psp)
21. [The SFR layer](#21-the-sfr-layer)
22. [Device selection](#22-device-selection)
23. [The examples](#23-the-examples)
24. [Known gaps and gotchas](#24-known-gaps-and-gotchas)
25. [Appendix: datasheet section index](#25-appendix-datasheet-section-index)

---

## 1. What this is

The PIC16F87XA is a mid-range 8-bit Microchip family: four pin-compatible
parts that share one datasheet (DS39582B) and differ mainly in package,
flash/RAM/EEPROM size, and how many analog channels and ports they expose:

| Part     | Pins | Flash | RAM  | EEPROM | ADC ch | PORTD/PORTE | PSP |
|----------|------|-------|------|--------|--------|-------------|-----|
| 16F873A  | 28   | 4 KW  | 192B | 128B   | 5      | no          | no  |
| 16F874A  | 40   | 4 KW  | 192B | 128B   | 8      | yes         | yes |
| 16F876A  | 28   | 8 KW  | 368B | 256B   | 5      | no          | no  |
| 16F877A  | 40   | 8 KW  | 368B | 256B   | 8      | yes         | yes |

See `epic-common/MANUAL.md` §1 for why this HAL exists at all (the general
"absorbs the register-level machinery" pitch) — it's the same reasoning
for every family in this repo, not repeated here.

---

## 2. The big picture

```
pic16f87xa-hal/
├── include/
│   ├── pic16f87xa.h            family header: device select, SFR mapping;
│   │                           pulls shared status codes / bit helpers from
│   │                           epic-common/hal_status.h, includes platform hdr
│   ├── pic16f87xa_sfr.h        SFR address map + bit names (1:1 DS39582B)
│   ├── pic16f87xa_sim.h        simulation backend public API
│   ├── host/                   platform header selected by the host build
│   │   └── pic16f87xa_platform.h   (SFRs → memory array, weak attribute)
│   ├── target/                 platform header selected by the XC8 build
│   │   └── pic16f87xa_platform.h   (SFRs → volatile deref, no weak)
│   ├── core/                    CPU-level features (PIC16-specific)
│   │   ├── pic16_irq.h                IRQn enum + EPIC_IRQ_* backend
│   │   └── pic16f87xa_wdt_sleep.h      WDT/Sleep/BOR/POR helpers
│   └── peripherals/            one .h per peripheral, Cube-style
├── src/
│   ├── core/                    implementations of core/ headers
│   ├── peripherals/            implementations of peripherals/ headers
│   └── sim/                     host simulation backend (host build only)
├── tests/                        end-to-end examples / smoke tests
├── mcu/pic16f87xa-mplabx/       XC8 Makefile + MPLAB X project
└── CMakeLists.txt               host build (gcc + cmake)
```

The general shape of this tree, the SFR-access-by-include-path trick, the
host/target link-time split, is explained once in `epic-common/MANUAL.md`
§2 and applies here unchanged. The one genuinely PIC16-specific piece:

### 2.1 One interrupt vector, one dispatcher

The PIC16F87XA has a **single** interrupt vector at 0x0004 (DS39582B
§14.11). On a real target, `src/core/pic16_isr_vector.c` installs the XC8
`__interrupt()` handler, which calls `epic_dispatch_all_irqs()`. On
the host, the harness registers that same function as the simulator's IRQ
callback. Either way, the one dispatcher fans out to every peripheral's
`*IRQHandler`, and each of those is a no-op unless its own flag is set.
Applications never wire interrupt vectors themselves. (Contrast with
`pic18fxx5x-hal`, which has two vectors and a priority model — see its own
manual §2 and `epic-common/MANUAL.md` §6 for what's shared about the
interrupt story across families.)

---

## 3. Quick start

### Host simulation

```sh
cd pic16f87xa-hal
cmake -B build -S .
cmake --build build
./build/example_blink
```

You should see `RB0 toggled 9 times.` and an exit code of 0. That
`example_blink` is a real Timer0-interrupt blink, executed against the
in-memory simulator, no PIC hardware involved.

### Real target (XC8)

Assuming MPLAB XC8 v3.10 is installed:

```sh
cd pic16f87xa-hal/mcu/pic16f87xa-mplabx
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make MCU=16F877A
# → build/16F877A-firmware.hex, program with MPLAB X, ippe, PK2CMD, …
```

To build a different example for the target, override `APP_SOURCES`:

```sh
make MCU=16F876A APP_SOURCES=../../tests/example_idle_blink.c
```

See [§4](#4-build-systems) for the full set of build variables.

---

## 4. Build systems

### 4.1 Host build, CMake

```sh
cmake -B build -S .                 # configure (default device: 16F877A)
cmake --build build                 # build the HAL lib + every example
```

Variables:

| Variable                 | Default      | Meaning                                                       |
|--------------------------|--------------|---------------------------------------------------------------|
| `PIC16F87XA_DEVICE`      | `PIC16F877A` | Which part the library is compiled for.                       |

The CMake build links `include/host` first, links the sim-side harness
and WDT/Sleep no-ops, and links `src/sim/pic16f87xa_sim.c`. It builds one
executable per example (`example_blink`, `example_idle_blink`, …) plus
`example_blink_<DEV>` for each of the four devices, so you can smoke-test
the device-conditional code paths.

To add your own application as a host test, append to `CMakeLists.txt`:

```cmake
pic16f87xa_add_example(my_app tests/my_app.c)
```

The helper `pic16f87xa_add_example` adds the executable, gives it the
device define, and links the HAL library (which propagates the
`include/host` include path).

### 4.2 Target build, XC8 Makefile

```sh
make MCU=16F877A            # default; also 873A / 874A / 876A
make clean
```

Variables (all overridable on the command line):

| Variable        | Default              | Meaning                                                |
|-----------------|----------------------|--------------------------------------------------------|
| `MCU`           | `16F877A`            | Target part (873A/874A/876A/877A).                     |
| `FOSC_HZ`       | `20000000`           | Oscillator frequency, passed as `-DFOSC_HZ`.           |
| `DFP_DIR`       | XC8 v3.10 install path | Device Family Pack dir for `-mdfp`. Empty → omit.   |
| `APP_SOURCES`   | `example_blink.c`    | Your application source(s).                            |
| `EPIC_DIR`       | `../..`              | Where the HAL tree lives.                              |
| `BUILD_DIR`     | `build`              | Output directory.                                      |

Output: `build/<MCU>-firmware.hex` (Intel HEX, via `-ginhx32`).

The Makefile generates a Configuration Word source at build time
(`build/config_<MCU>.c`) with these directives; adjust to your board:

```c
#pragma config FOSC = HS     // high-speed crystal (≤ 20 MHz)
#pragma config WDTE = ON     // watchdog enabled, refresh with EPIC_WDT_Refresh()
#pragma config PWRTE = ON    // power-up timer 72 ms
#pragma config BOREN = ON    // brown-out reset at 4.0 V
#pragma config LVP = OFF     // low-voltage programming off
#pragma config WRT = OFF     // flash write protection off
```

The full Configuration Word layout is DS39582B §14.1 (Register 14-1).

**XC8 v3.x notes.** The `xc8-cc` driver compiles each translation unit to
a `.p1` (p-code) intermediate, not a `.o`; the hex comes from a single
link step over the `.p1` set with `-ginhx32`. v3.x also moved device
support into a Device Family Pack, so the compiler needs `-mdfp`. If your
XC8 install lives elsewhere, pass `make DFP_DIR=/path/to/dfp` (or empty
to omit the flag for an older toolchain).

---

## 5. Datasheet citations

Every header and implementation file annotates the DS39582B section it
implements. When in doubt about a register bit, grep the header for the
section number, e.g. `core/pic16_irq.h` cites §14.11 on every
IRQ enumerator. If you find a deviation from the cited section, that is a
bug.

---

## 6. What the simulator models

The simulator (`src/sim/pic16f87xa_sim.c`, public API in
`pic16f87xa_sim.h`) is a 512-byte memory array (`pic16f87xa_sim_sfr[]`)
standing in for the PIC's banked register file. See `epic-common/MANUAL.md`
§5 for the shape of its public API and how host tests use it; what it
actually models, on this family, is:

- **Timer0**, increments per instruction cycle at the OPTION_REG
  prescaler ratio; sets `TMR0IF` on 0xFF→0x00 rollover.
- **Timer1**, 16-bit increment; advances in internal-clock mode *and*
  in external/T1OSC mode (the latter at a *simplified* rate, the sim
  does not reproduce the 32.768 kHz crystal's real timing, only the
  overflow/IRQ plumbing; see [§24](#24-known-gaps-and-gotchas)).
- **Timer2**, counts against `PR2`, applies prescaler and postscaler,
  sets `TMR2IF` on period match.
- **GPIO**, PORTA..PORTE read-modify-write semantics: writes update the
  latch only; reads return the externally-driven level for input pins
  and the latch for output pins. PORTA is 6-bit, PORTE 3-bit, others
  8-bit, enforced.
- **USART**, re-asserts `TXIF` each cycle when `TXEN` is set (modelling
  instantaneous transmit completion); `RCIF` is set by the test via
  `pic16f87xa_sim_drive_usart_rx`.
- **EEPROM**, a 256-byte backing store; tests poke bytes and completion
  via the `drive_eeprom_*` helpers.

---

## 7. Interrupts

The PIC16F87XA has one interrupt vector at 0x0004 (DS39582B §14.11). See
`epic-common/MANUAL.md` §6 for the shared `EPIC_IRQ_*` API, the priority
vocabulary (unused on this single-vector family, `EPIC_IRQ_SetPriority` is
a no-op here), and §7 for the ISR-driven-peripheral recipe — both apply
unchanged. The one per-family piece is the IRQ identity enum:

```c
typedef enum {
    PIC16_IRQ_RB       = 0,   // RB<7:4> change
    PIC16_IRQ_INT      = 1,   // RB0/INT external
    PIC16_IRQ_TMR0     = 2,   // Timer0 overflow
    PIC16_IRQ_TMR1     = 3,   // Timer1 overflow
    PIC16_IRQ_TMR2     = 4,   // Timer2 == PR2 match
    PIC16_IRQ_CCP1     = 5,
    PIC16_IRQ_CCP2     = 6,
    PIC16_IRQ_SSP      = 7,   // SSP TX/RX/I²C
    PIC16_IRQ_BCL      = 8,   // SSP bus collision
    PIC16_IRQ_USART_TX = 9,
    PIC16_IRQ_USART_RX = 10,
    PIC16_IRQ_ADC      = 11,
    PIC16_IRQ_EEPROM   = 12,
    PIC16_IRQ_CMP      = 13,  // comparator output change
    PIC16_IRQ_PSP      = 14,  // 40/44-pin only
} PIC16_IRQn;
```

`epic_dispatch_all_irqs()` (in `src/core/pic16_irq_dispatch.c`, built on
both host and target) calls every peripheral `*IRQHandler` in turn.

---

## 8. Core: WDT, Sleep, BOR/POR

```c
void   EPIC_WDT_Refresh(void);     // asm("clrwdt") on target; no-op on host
void   EPIC_Sleep_Enter(void);     // asm("sleep")  on target; no-op on host
uint8_t EPIC_BOR_GetStatus(void);  // PCON<BOR>, was the last reset a brown-out?
void   EPIC_BOR_ClearFlag(void);
uint8_t EPIC_POR_GetStatus(void);  // PCON<POR>, was the last reset a power-on?
void   EPIC_POR_ClearFlag(void);
```

`EPIC_WDT_Refresh` executes `clrwdt`. If the configuration word has
`WDTE = ON` (the Makefile's default), you **must** call this more often
than the WDT timeout or the chip resets. In an interrupt-driven idle loop
that sleeps, refresh the WDT from the wake-up ISR so it does not expire
while the CPU is asleep (see `example_idle_blink`).

`EPIC_Sleep_Enter` executes `sleep`; the CPU halts until any enabled
interrupt wakes it (DS39582B §14.14). Note that **Timer0 (internal clock)
stops in Sleep**, it cannot wake the CPU. To wake from Sleep you need a
peripheral with its own clock: Timer1 with the external 32.768 kHz T1OSC
crystal (asynchronous mode) is the canonical choice, along with INT/RB
change, the WDT, SSP (I²C slave), EEPROM-done, etc.

`EPIC_BOR_GetStatus` / `EPIC_POR_GetStatus` read the PCON reset-reason bits
(§14.10). Clear them after reading so you can distinguish the next reset's
cause.

---

## 9. GPIO

*DS39582B §4.1-§4.5, Tables 4-1..4-9.*

### 9.1 Types

```c
typedef enum { GPIOA=0, GPIOB=1, GPIOC=2, GPIOD=3, GPIOE=4 } GPIO_TypeDef;
// GPIOD / GPIOE exist only on 40/44-pin parts.

#define GPIO_PIN_0   EPIC_BIT(0)   // … GPIO_PIN_7
#define GPIO_PIN_All 0xFFU

typedef enum { GPIO_PIN_RESET=0, GPIO_PIN_SET=1 } GPIO_PinState;
typedef enum { GPIO_MODE_INPUT=0x1, GPIO_MODE_OUTPUT=0x2, GPIO_MODE_ANALOG=0x3 } GPIO_ModeTypeDef;
typedef enum { GPIO_NOPULL=0, GPIO_PULLUP=1 } GPIO_PullTypeDef;   // PORTB only
```

### 9.2 Functions

```c
void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);
void EPIC_GPIO_DeInit(GPIO_TypeDef port);                          // all pins → input, latch cleared
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);                  // OPTION_REG<RBPU>, PORTB only
```

`pins` is a bitmask of `GPIO_PIN_*` (you can OR them to affect several
pins at once). Pin widths are enforced: PORTA is 6-bit, PORTE is 3-bit,
others 8-bit; out-of-range bits in `pins` are masked off.

### 9.3 Notes and gotchas

- The PORTx registers are read-modify-write, but **writes only update the
  latch**; reads return the externally-driven level for inputs and the
  latch for outputs. The driver follows the datasheet's recommended
  `BSF/BCF` idiom, it does not read back the pin to set a bit.
- `GPIO_MODE_ANALOG` sets TRIS=1 (input) the same as `GPIO_MODE_INPUT`;
  the difference is documentary. To actually release a PORTA pin to the
  ADC, set `ADCON1<PCFG3:PCFG0>` (via `EPIC_ADC_Init`'s `Reference`
  field) **before** configuring the pin.
- Pull-ups are PORTB-only and controlled by `OPTION_REG<RBPU>`, which is
  inverted (`RBPU=1` disables). `EPIC_GPIO_SetPullups` handles that.

### 9.4 PORTB change interrupt (RB<7:4>)

*DS39582B §14.11.3, INTCON<RBIF>/<RBIE>.*

A whole-port callback hook for the RB<7:4> interrupt-on-change source,
the same shape as Timer2's one-callback-per-handle but simpler (there is
only one PORTB, so no handle struct). Added so family-agnostic modules
(notably `epic-encoder`) can consume the RB-change interrupt without a
family-specific include; the family-neutral shim `peripherals/hal_gpio.h`
pulls this header in.

```c
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));
void RB_IRQHandler(void) EPIC_WEAK;
```

`EPIC_GPIO_RegisterChangeCallback` stores one callback slot (NULL is safe,
the handler no-ops). `RB_IRQHandler` is the weak ISR the dispatcher fans
out to; the default body reads PORTB, clears RBIF, then calls the
registered callback with that already-read byte.

**Read before clear, mandatory.** The mismatch comparator latches the
value of PORTB at the last CPU read, so reading PORTB is what ends the
current mismatch condition and re-arms the next one. Clearing RBIF first
(or not reading at all) risks an immediate spurious re-interrupt or a
silently-missed change. The handler therefore reads PORTB into a local
*before* `EPIC_IRQ_ClearFlag`, and the callback receives that already-read
value, never a second later read (which by then may not reflect the byte
the mismatch logic cleared against). Do not reorder this.

The dispatcher calls `RB_IRQHandler` on every interrupt (it checks RBIF
and returns immediately when not pending), so it costs only a flag test
when the source is idle. To arm it on a real target, also
`EPIC_IRQ_Enable(PIC16_IRQ_RB)` and `EPIC_IRQ_Restore(1)`.

**Host sim.** The sim does not auto-assert RBIF on a PORTB mismatch
(faithfully modeling the datasheet "snapshot on every PORTB read" would
require intercepting every CPU read of PORTB through the `EPIC_REG8`
macro, disproportionate for the one feature that needs it). Tests that
exercise the handler assert RBIF directly in INTCON, then call
`RB_IRQHandler` / `epic_dispatch_all_irqs` and check the callback's byte
and the post-call flag state. This proves the handler's read/clear/callback
ordering, the part that actually matters. See
`tests/example_rb_change.c` and `epic-encoder/docs/ARCHITECTURE.md`.

---

## 10. Timer0

*DS39582B §5.0, §5.3, Register 5-1, Table 5-1.*

8-bit timer/counter with a prescaler **shared with the WDT**.

### 10.1 Handle

```c
typedef struct {
    TIMER0_ClockSourceTypeDef  ClockSource;        // TIMER0_CLOCK_INTERNAL / _EXTERNAL (RA4/T0CKI)
    TIMER0_ClockEdgeTypeDef     ClockEdge;         // _RISING / _FALLING (external only)
    TIMER0_PrescalerTypeDef     Prescaler;         // TIMER0_PRESCALER_1_2 .. _1_256
    bool                        PrescalerAssigned; // true = prescaler → Timer0; false → WDT
    uint8_t                     ReloadValue;       // 0..255, written to TMR0 on Start
    void (*OverflowCallback)(void);                // called on each 0xFF→0x00 rollover
} TIMER0_HandleTypeDef;
```

Prescaler values are the `PS2:PS0` encoding. Note `PS2:PS0=000` is **1:2**
for Timer0, not 1:1, 1:1 means "don't assign the prescaler to Timer0"
(`PrescalerAssigned = false`), which sends the prescaler to the WDT and
gives Timer0 the raw clock.

### 10.2 Functions

```c
EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h);  // writes ReloadValue, sets T0CS
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);
uint8_t  EPIC_TIMER0_ReadCounter(void);
void     EPIC_TIMER0_WriteCounter(uint8_t value);   // also clears the prescaler
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);  // 1, 2, …, 256
void     TIMER0_IRQHandler(void) EPIC_WEAK;
```

### 10.3 Gotchas

- **Writing TMR0 clears the prescaler** when it is assigned to Timer0
  (§5.3 note). `Start` and `WriteCounter` both write TMR0, so they reset
  the prescaler count.
- Switching the prescaler from Timer0 to WDT while the WDT is active needs
  a specific sequence to avoid a spurious reset (§5.3 footnote 1). The
  driver does not touch `PSA` once WDT is running; if you must, follow the
  datasheet example.
- Timer0 on the **internal clock stops in Sleep**, it cannot wake the
  CPU. Use Timer1 + T1OSC for sleep wake-ups.

### 10.4 Example

```c
TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;     // internal, 1:256, reload 0
h.OverflowCallback = on_t0_overflow;
EPIC_TIMER0_Init(&h);
EPIC_TIMER0_Start(&h);
EPIC_IRQ_Restore(1);                          // GIE on
```

At FOSC = 20 MHz (FCY = 5 MHz), 1:256 overflows every
256 × 256 × 0.2 µs ≈ 13 ms.

---

## 11. Timer1

*DS39582B §6.0, Register 6-1.*

16-bit timer with its own prescaler and an optional 32.768 kHz crystal
oscillator (T1OSC), the only timer that keeps running in Sleep.

### 11.1 Handle

```c
typedef struct {
    TIMER1_ClockSourceTypeDef  ClockSource;   // _INTERNAL (Fosc/4) / _EXTERNAL (pin or T1OSC)
    TIMER1_ClockSyncTypeDef    ClockSync;     // _SYNC_EXTERNAL / _ASYNC_EXTERNAL (async needed for Sleep)
    TIMER1_OscillatorTypeDef   Oscillator;    // _OFF / _ON (T1OSC crystal on T1OSI/T1OSO)
    TIMER1_PrescalerTypeDef    Prescaler;     // _1_1 / _1_2 / _1_4 / _1_8
    uint16_t                   ReloadValue;   // 16-bit initial counter
    void (*OverflowCallback)(void);
} TIMER1_HandleTypeDef;
```

### 11.2 Functions

```c
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void);
uint16_t EPIC_TIMER1_ReadCounter(void);    // atomic, read-twice idiom per §6.4.1
void     EPIC_TIMER1_WriteCounter(uint16_t value);
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);  // 1, 2, 4, 8
void     TIMER1_IRQHandler(void) EPIC_WEAK;
```

### 11.3 Notes

- **Asynchronous mode** (`ClockSync = TIMER1_ASYNC_EXTERNAL`) is required
  for the counter to keep running while the CPU is in Sleep, it bypasses
  the Fosc/4 synchroniser. It is the default when T1OSC is enabled.
- With the T1OSC crystal at 32.768 kHz and a reload of `0x8000` (32768),
  Timer1 overflows every 1 s. That is the basis of the low-power idle
  blink (`example_idle_blink`).

---

## 12. Timer2

*DS39582B §7.0, Register 7-1.*

8-bit timer with a period register `PR2` and a postscaler. Its overflow
(`TMR2IF`) is the PWM time base for CCP1/CCP2.

### 12.1 Handle

```c
typedef struct {
    TIMER2_PrescalerTypeDef   Prescaler;    // _1_1 / _1_4 / _1_16
    TIMER2_PostscalerTypeDef   Postscaler;   // _1_1 .. _1_16
    uint8_t                    Period;       // PR2, 0..255
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;
```

`TMR2IF` fires every `prescaler × (PR2+1) × postscaler` instruction
cycles. TMR2 resets to 0 when it matches PR2 (it never holds PR2+1).

### 12.2 Functions

```c
EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER2_Stop(void);
uint8_t  EPIC_TIMER2_ReadCounter(void);
void     EPIC_TIMER2_WriteCounter(uint8_t value);
uint8_t  EPIC_TIMER2_ReadPeriod(void);
void     EPIC_TIMER2_WritePeriod(uint8_t period);
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);
void     TIMER2_IRQHandler(void) EPIC_WEAK;
```

---

## 13. CCP1 / CCP2, Capture, Compare, PWM

*DS39582B §8.0, Register 8-1, Tables 8-1..8-4.*

Two identical CCP modules except for the special-event trigger: CCP1's
resets Timer1; CCP2's resets Timer1 **and** starts an A/D conversion
(§8.2.4).

Timer resources (Table 8-1): **Capture** and **Compare** need Timer1;
**PWM** needs Timer2 (start Timer2 with the matching period first).

### 13.1 Handle

```c
typedef struct {
    CCP_InstanceTypeDef   Instance;       // CCP_INSTANCE_1 (RC2) / _2 (RC1)
    CCP_ModeTypeDef       Mode;            // see below
    uint16_t              CompareValue;    // 16-bit compare/capture value
    CCP_PWMConfigTypeDef  PWM;             // { uint16_t Period; uint16_t Duty; } (PWM only)
    void (*EventCallback)(void);
} CCP_HandleTypeDef;
```

Modes (`CCPxCON<3:0>`):

| Mode                          | Value | Meaning                            |
|-------------------------------|-------|------------------------------------|
| `CCP_MODE_OFF`                | 0x0   | disabled                           |
| `CCP_MODE_CAPTURE_FALLING`    | 0x4   | capture every falling edge         |
| `CCP_MODE_CAPTURE_RISING`     | 0x5   | capture every rising edge          |
| `CCP_MODE_CAPTURE_4TH`        | 0x6   | every 4th rising edge               |
| `CCP_MODE_CAPTURE_16TH`       | 0x7   | every 16th rising edge              |
| `CCP_MODE_COMPARE_SET`        | 0x8   | set output on match                 |
| `CCP_MODE_COMPARE_CLEAR`      | 0x9   | clear output on match                |
| `CCP_MODE_COMPARE_SOFT_IF`    | 0xA   | software interrupt only             |
| `CCP_MODE_COMPARE_TRIGGER`    | 0xB   | special event trigger                |
| `CCP_MODE_PWM`                | 0xC   | PWM                                 |

There is **no `CCP_HANDLE_DEFAULT`**, CCP is the one peripheral that
expects you to fill the handle explicitly (Instance and Mode are
load-bearing).

### 13.2 Functions

```c
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);
void     EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);
void     EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty);  // 0..1023
void     CCP1_IRQHandler(void) EPIC_WEAK;
void     CCP2_IRQHandler(void) EPIC_WEAK;
```

### 13.3 PWM usage

```c
TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
t2.Prescaler = TIMER2_PRESCALER_1_1;
t2.Period    = 99;                 // PR2 = 99 → 100-cycle PWM period
EPIC_TIMER2_Init(&t2);
EPIC_TIMER2_Start(&t2);

CCP_HandleTypeDef ccp = { .Instance = CCP_INSTANCE_1, .Mode = CCP_MODE_PWM,
                           .PWM = { .Period = 99, .Duty = 50 } };  // 50% of 100→ ~50%
EPIC_CCP_Init(&ccp);               // RC2/CCP1 now outputs PWM
EPIC_CCP_SetPWMDuty(CCP_INSTANCE_1, 512);   // later: 50% of 1024
```

Duty is 10-bit (0..1023). For duty 0 the output stays low for the whole
period; for duty greater than the period it stays high (§8.3.2 note).

---

## 14. USART

*DS39582B §10.0, Registers 10-1/10-2, §10.1 (BRG).*

One USART instance. Async full-duplex (TX=RC6, RX=RC7), sync
master/slave, 8- or 9-bit.

### 14.1 Handle and baud computation

```c
typedef struct {
    USART_ModeTypeDef          Mode;          // _ASYNCHRONOUS / _SYNCHRONOUS
    USART_ClockSourceTypeDef   ClockSource;   // _MASTER / _SLAVE (sync only)
    USART_BaudRateHighTypeDef  BaudHigh;       // _LOW (÷64) / _HIGH (÷16), async only
    USART_DataWidthTypeDef     DataWidth;      // _8BITS / _9BITS
    uint8_t                    SPBRG;         // 0..255, pre-computed
    void (*TxCpltCallback)(void);
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode, USART_BaudRateHighTypeDef brgh);
```

Baud formulas (§10.1): async `Fosc/(64×(X+1))` (BRGH=0) or
`Fosc/(16×(X+1))` (BRGH=1); sync `Fosc/(4×(X+1))`. `ComputeSPBRG`
returns 0..255, or `0xFFFF` if the desired baud needs X > 255.

### 14.2 Functions

```c
EPIC_StatusTypeDef EPIC_USART_Init(const USART_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_USART_DeInit(void);
void    EPIC_USART_Transmit(uint8_t data);    // writes TXREG; clears TXIF
uint8_t EPIC_USART_GetTX9D(void);
void    EPIC_USART_SetTX9D(uint8_t bit9);     // set BEFORE Transmit for 9-bit
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);   // TRMT
uint8_t EPIC_USART_Receive(void);              // reads RCREG; clears RCIF, advances FIFO
uint8_t EPIC_USART_GetRX9D(void);
void    USART_RX_IRQHandler(void) EPIC_WEAK;
void    USART_TX_IRQHandler(void) EPIC_WEAK;
```

### 14.3 Notes

- `TXIF` is **not** cleared by reading, only by writing TXREG. `Transmit`
  does that for you.
- `RCIF` is cleared by reading `RCREG`; `Receive` does that and advances
  the 2-deep FIFO.
- For 9-bit transmit, set `SetTX9D` *before* `Transmit`.

---

## 15. MSSP, SPI and I²C

*DS39582B §9.0, Registers 9-1..9-5.*

One MSSP module on RC3/SCK/SCL, RC4/SDI/SDA, RC5/SDO. ⚠ The driver is
**register-level only**, it does not implement an I²C state machine.
For SPI, a transfer is automatic once `SSPBUF` is written; for I²C master
you drive Start/Stop/ACK by hand (see below).

### 15.1 Handle

```c
typedef struct {
    SSP_ModeTypeDef           Mode;            // see below
    SSP_ClockEdgeTypeDef      ClockEdge;       // SPI: CKE
    SSP_ClockPolarityTypeDef  ClockPolarity;   // SPI: CKP
    SSP_SamplePhaseTypeDef    SamplePhase;     // SPI master: SMP
    uint8_t                   SSPADD;          // I²C slave addr / master baud reload
    void (*TransferCallback)(void);
} SSP_HandleTypeDef;
```

`SSP_Mode` (SSPCON<3:0>): `SSP_MODE_SPI_MASTER_FOSC_4`/`_16`/`_64`/`_TMR2`,
`SSP_MODE_SPI_SLAVE_SS_DIS`/`_SS_EN`, `SSP_MODE_I2C_SLAVE_7BIT`/`_10BIT`,
`SSP_MODE_I2C_MASTER_FW` (firmware-controlled), `_SLAVE_7BIT_SS`,
`_SLAVE_10BIT_SS`, `SSP_MODE_I2C_MASTER_FOSC` (hardware).

### 15.2 Functions

```c
EPIC_StatusTypeDef EPIC_SSP_Init(const SSP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_SSP_DeInit(void);

/* SPI / common */
uint16_t EPIC_SSP_WriteByte(uint8_t data);   // returns 0xFFFF on WCOL (byte not written)
uint8_t  EPIC_SSP_ReadByte(void);
uint8_t  EPIC_SSP_IsBufferFull(void);         // SSPSTAT<BF>
uint8_t  EPIC_SSP_HasWriteCollision(void);
void     EPIC_SSP_ClearWriteCollision(void);

/* I²C master helpers */
uint16_t SSP_ComputeSSPADD(uint32_t fosc_hz, uint32_t fscl_hz);  // Fscl = Fosc/(4×(SSPADD+1))
void     EPIC_SSP_Start(void);          void     EPIC_SSP_RepeatedStart(void);
void     EPIC_SSP_Stop(void);          void     EPIC_SSP_ReceiveEnable(void);
void     EPIC_SSP_AcknowledgeEnable(void);
uint8_t  EPIC_SSP_AcknowledgeStatus(void);
void     SSP_IRQHandler(void) EPIC_WEAK;
```

### 15.3 I²C master by hand

```c
EPIC_SSP_Start();
EPIC_SSP_WriteByte((addr << 1) | 1);          // read address
while (!EPIC_SSP_IsBufferFull()) { }
(void)EPIC_SSP_ReadByte();
EPIC_SSP_Stop();
```

You are responsible for polling `BF` and `ACKSTAT` and for the
Start/Repeated-Start/Stop sequence. This mirrors the datasheet's
firmware-controlled master; a higher-level blocking transfer helper is
not provided (see [§24](#24-known-gaps-and-gotchas)).

---

## 16. ADC

*DS39582B §11.0, Registers 11-1/11-2, Table 11-1.*

10-bit successive-approximation ADC: 5 channels on 28-pin parts, 8 on
40/44-pin. References are VDD/VSS or VREF+ on AN2/AN3. Result in
ADRESH:ADRESL.

### 16.1 Handle

```c
typedef struct {
    ADC_ChannelTypeDef          Channel;        // ADC_CHANNEL_AN0 .. _AN7 (_AN5.._AN7: 40/44 only)
    ADC_ClockSourceTypeDef      ClockSource;     // ADC_CLOCK_FOSC_2/8/32/_RC/_4/_16/_64
    ADC_ResultFormatTypeDef     ResultFormat;    // _LEFT / _RIGHT justified
    ADC_ReferenceTypeDef        Reference;       // PCFG3:PCFG0, see Register 11-2 table
    void (*ConvCpltCallback)(uint16_t result);
} ADC_HandleTypeDef;
```

`Reference` is the raw 4-bit `PCFG3:PCFG0` value; the enum names
(`ADC_REFERENCE_VDD_VSS_8CH`, `ADC_REFERENCE_VREF_2CH`, …) map 1:1 to the
datasheet's configuration table, which also determines which analog pins
are available. Pick the reference first, then configure the freed PORTA
pins as analog via `EPIC_GPIO_Init`.

### 16.2 Functions

```c
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);
uint16_t EPIC_ADC_Start(void);                  // sets GO/DONE; 0xFFFF if already in progress
void     EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);
uint8_t  EPIC_ADC_IsConversionInProgress(void);  // GO/DONE == 1
uint8_t  EPIC_ADC_IsConversionDone(void);         // ADIF == 1
void     EPIC_ADC_ClearITFlag(void);
uint16_t EPIC_ADC_Read(void);                    // 0..1023 (right-justified; left is shifted down)
void     ADC_IRQHandler(void) EPIC_WEAK;
```

### 16.3 The acquisition-time step

The driver splits channel-select and start so the
acquisition time (§11.1, ~20 µs at VDD = 5 V) can elapse between them:

```c
EPIC_ADC_SelectChannel(ADC_CHANNEL_AN0);
/* wait ≥ T_acq */
EPIC_ADC_Start();
while (!EPIC_ADC_IsConversionDone()) { }
uint16_t val = EPIC_ADC_Read();
```

Or use the completion IRQ: set `ConvCpltCallback` and enable interrupts;
the callback receives the 10-bit result.

---

## 17. Comparator

*DS39582B §12.0, Register 12-1, Figure 12-1.*

Two on-chip comparators with eight operating modes selected by
`CMCON<CM2:CM0>`. Inputs on PORTA (RA0..RA3, RA5); outputs on RA4
(C1OUT) and RA5 (C2OUT) when enabled. Interrupt on output change
(`CMIF`).

### 17.1 Handle and functions

```c
typedef struct {
    COMP_ModeTypeDef Mode;            // COMP_MODE_RESET / _ONE_WITH_OUT / _TWO_INDEP / … / _OFF
    bool             C1Inverted;       // C1INV
    bool             C2Inverted;       // C2INV
    bool             CIS;              // comparator input switch
    void (*ChangeCallback)(void);
} COMP_HandleTypeDef;

EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_COMP_DeInit(void);
uint8_t EPIC_COMP_C1Out(void);            // CMCON<C1OUT>
uint8_t EPIC_COMP_C2Out(void);
uint8_t EPIC_COMP_IsChangeFlag(void);
void    EPIC_COMP_ClearChangeFlag(void);
void    COMP_IRQHandler(void) EPIC_WEAK;
```

POR default is `CMCON = 0x07` (comparators off). Pick a mode from the
datasheet's Figure 12-1; e.g. `COMP_MODE_TWO_WITH_OUT` for two independent
comparators with outputs on RA4/RA5.

---

## 18. Voltage reference (Vref)

*DS39582B §13.0, Register 13-1.*

A 16-tap resistor ladder. Low range: 0..0.75×VDD in steps of VDD/24.
High range (`CVRR=1`): 0.25..0.75×VDD in steps of VDD/32. Output routed
to RA2/AN2/VREF- when `OutputEnable` is set.

```c
typedef struct {
    VREF_RangeTypeDef Range;        // _LOW / _HIGH
    uint8_t           Value;        // 0..15, ladder tap
    bool              OutputEnable; // route to RA2
    bool              Enabled;      // CVREN
} VREF_HandleTypeDef;

EPIC_StatusTypeDef EPIC_VREF_Init(const VREF_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_VREF_DeInit(void);
uint32_t EPIC_VREF_MilliVolts(uint32_t vdd_mv, VREF_RangeTypeDef range, uint8_t value);
```

`EPIC_VREF_MilliVolts` computes the nominal output for a given VDD (mV),
range, and tap, handy for tests and for printing what you configured.

---

## 19. Data EEPROM

*DS39582B §3.0, Table 3-1, Register 3-1, §3.4/§3.5.*

128 bytes (873A/874A) or 256 bytes (876A/877A) of non-volatile data
EEPROM. The driver **hides the mandatory unlock sequence** (write 0x55
then 0xAA to EECON2 before WR, §3.4, Example 3-1).

### 19.1 API

EEPROM takes a raw callback, not a handle:

```c
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void));   // enables PIE2<EEIE> if callback
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);

uint8_t  EPIC_EEPROM_ReadByte(uint8_t addr);                          // RD
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);  // unlock + WR
void     EPIC_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);
EPIC_StatusTypeDef EPIC_EEPROM_WriteBuffer(uint8_t start, const uint8_t *buf, uint8_t len);

uint8_t  EPIC_EEPROM_IsWriteComplete(void);   // EEIF
void     EPIC_EEPROM_ClearITFlag(void);
void     EEPROM_IRQHandler(void) EPIC_WEAK;
```

### 19.2 Notes

- Writes are **non-blocking**: `WriteByte` returns as soon as `WR` is set.
  Detect completion by polling `EPIC_EEPROM_IsWriteComplete()` (EEIF,
  PIR2<4>) or via the EEPROM interrupt. Do not start a new write before
  the previous one finishes.
- `WriteByte` returns `EPIC_ERROR` if a previous write was aborted
  (`WRERR` set).

---

## 20. Parallel Slave Port (PSP)

*DS39582B §4.5, Register 4-9. 40/44-pin parts only.*

An 8-bit parallel bus on PORTD (PSP0..PSP7) plus three control lines on
PORTE: RE0/RD, RE1/WR, RE2/CS. An external master reads/writes the part
through these pins when `TRISE<PSPMODE>` is set.

```c
EPIC_StatusTypeDef EPIC_PSP_Init(void (*callback)(void));   // enables PSPIE if callback
EPIC_StatusTypeDef EPIC_PSP_DeInit(void);
void     EPIC_PSP_Enable(void);          void     EPIC_PSP_Disable(void);
uint8_t  EPIC_PSP_IsInputBufferFull(void);    // TRISE<IBF>
uint8_t  EPIC_PSP_IsOutputBufferFull(void);   // TRISE<OBF>
uint8_t  EPIC_PSP_HasInputOverflow(void);     // TRISE<IBOV>
void     EPIC_PSP_ClearInputOverflow(void);
void     PSP_IRQHandler(void) EPIC_WEAK;
```

Including `pic16f87xa_psp.h` on a 28-pin part is a **compile error**
(`#error "…for 40/44-pin PIC16F87XA parts only"`); guard with
`PIC16F87XA_FAMILY_HAS_PSP` if you share code across the family. The
driver only programs configuration, an external master drives CS/RD/WR
on real silicon; on the sim, tests drive state through the
`pic16f87xa_sim_*` helpers.

---

## 21. The SFR layer

When the HAL does not cover something, you can still talk to registers
directly through the platform macros and the SFR map
(`pic16f87xa_sfr.h`).

### 21.1 Address and bit constants

`pic16f87xa_sfr.h` defines, 1:1 with DS39582B:

- `PIC_REG_<NAME>` for every SFR address, `PIC_REG_PORTA` (0x05),
  `PIC_REG_TMR0` (0x01), `PIC_REG_T1CON` (0x10), `PIC_REG_EECON1`
  (0x18C), … grouped by bank (0-3). Family-dependent registers
  (`PORTD/PORTE/TRISD/TRISE`) are guarded by `PIC16F87XA_FAMILY_HAS_*`.
- `PIC_<REG>_<BIT>` for bit masks, `PIC_INTCON_GIE`, `PIC_T1CON_T1OSCEN`,
  `PIC_EECON1_WREN`, …
- `PIC_<REG>_POR_VALUE` for power-on reset values, `PIC_STATUS_POR_VALUE`
  (0x18), `PIC_INTCON_POR_VALUE` (0x00), …
- a `pic_select_bank(uint8_t bank)` inline to set STATUS<RP1:RP0>.

### 21.2 The access macros (platform header)

```c
EPIC_REG8(addr)                 // an lvalue for the register at addr
EPIC_SFR_PTR(addr)              // address of that register
epic_sfr_read8(addr)            // read
epic_sfr_write8(addr, value)    // write
EPIC_BIT_SET(reg, mask)         // reg |= mask
EPIC_BIT_CLR(reg, mask)         // reg &= ~mask
EPIC_BIT_TGL(reg, mask)         // reg ^= mask
EPIC_BIT_READ(reg, mask)        // reg & mask
EPIC_BIT(n)                     // (1u << n)
```

On the host `EPIC_REG8(0x05)` is `pic16f87xa_sim_sfr[0x05]`; on a
target it is `*(volatile uint8_t *)(uintptr_t)0x05`. The same source
compiles to both, which is the whole point of the platform layer.

### 21.3 Example: raw register access

```c
#include "pic16f87xa_sfr.h"

EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
uint8_t status = EPIC_REG8(PIC_REG_STATUS);
```

Prefer the HAL where it exists; reach for these only for bits the HAL
does not expose.

---

## 22. Device selection

Define exactly **one** of `PIC16F873A`, `PIC16F874A`, `PIC16F876A`,
`PIC16F877A` before including any HAL header. If none is defined,
`pic16f87xa.h` defaults to `PIC16F877A`; if more than one is defined, it
is a `#error`.

The choice sets a family of `PIC16F87XA_FAMILY_*` macros you can branch
on:

| Macro                         | 873A | 874A | 876A | 877A |
|-------------------------------|------|------|------|------|
| `_FLASH_KW`                   | 4    | 4    | 8    | 8    |
| `_RAM_BYTES`                   | 192  | 192  | 368  | 368  |
| `_EEPROM_B`                    | 128  | 128  | 256  | 256  |
| `_ADC_CH`                      | 5    | 8    | 5    | 8    |
| `_HAS_PORTD` / `_HAS_PORTE`    | 0    | 1    | 0    | 1    |
| `_HAS_PSP`                     | 0    | 1    | 0    | 1    |

These are *device-capability* guards (which chip), not build-mode guards
(host vs. target), they are the one kind of `#if` that legitimately
selects code in this HAL, because the absent peripherals genuinely do not
exist on the smaller parts. The CMake build compiles the library for the
default device; the XC8 Makefile compiles it for `MCU`.

---

## 23. The examples

All live under `tests/`. The first two build for **both** host and target
(via the harness, `epic-common/MANUAL.md` §4); the rest are host-only
smoke tests that talk to the simulator directly.

- **`example_blink`**, Timer0 overflow ISR toggles RB0. The canonical
  "the HAL drives a real application" test. Basic blink (~38 Hz at
  20 MHz).
- **`example_idle_blink`**, Timer1 on the 32.768 kHz T1OSC crystal,
  asynchronous, in Sleep. The CPU is awake only a handful of cycles per
  second; the rest is power-down. Demonstrates the low-power, mostly-idle
  pattern and the harness.
- **`example_timer1`**, Timer1 internal clock, counts overflows; checks
  the 65536-cycle period.
- **`example_timer2`**, Timer2 with PR2=249, 1:1 pre/post → 250-cycle
  period.
- **`example_ccp_pwm`**, PWM on RC2/CCP1 from a Timer2 time base, 50%
  duty; verifies the configuration registers and the period.
- **`example_usart`**, async USART transmit/receive against the sim's
  `drive_usart_rx`.
- **`example_ssp`**, SPI/I²C exercise via `drive_ssp_rx`.
- **`example_adc`**, ADC conversion via `drive_adc_done`.
- **`example_comp_vref`**, comparator + voltage reference.
- **`example_eeprom`**, EEPROM read/write/buffer via the sim EEPROM
  store.
- **`example_psp`**, PSP (40/44-pin only).
- **`example_wdt_sleep`**, WDT refresh, Sleep, and BOR/POR status helpers.

Run all host examples with:

```sh
cmake --build build
for t in build/example_*; do "$t"; done   # exit code 0 = pass
```

---

## 24. Known gaps and gotchas

- **T1OSC timing in the sim is approximate.** The simulator does not
  reproduce the 32.768 kHz crystal's real rate; it advances external-clock
  Timer1 at a simplified per-cycle rate so T1OSC-based firmware *runs* on
  the host, but do not infer real timing from sim step counts. The
  overflow/IRQ plumbing is faithful; the wall-clock rate is not.
- **MSSP I²C is register-level.** There is no blocking
  read/write/transfer state machine. You drive Start/Stop/ACK and poll
  `BF`/`ACKSTAT` yourself (§15.3).
- **Timer0 stops in Sleep.** Use Timer1 + T1OSC (async) to wake from
  Sleep (§8).
- **WDT vs. Sleep.** With `WDTE = ON` the WDT keeps running in Sleep; in
  a sleeping loop, refresh it from the wake-up ISR so it does not expire
  mid-sleep (as `example_idle_blink` does). Alternatively set `WDTE = OFF`
  in the config word.
- **Writing TMR0 clears its prescaler** (§5.3 note), `Start` and
  `WriteCounter` reset it.
- **CCP has no `HANDLE_DEFAULT`**, fill `Instance` and `Mode` explicitly.
- **PSP is 40/44-pin only**, including its header on a 28-pin part is a
  deliberate `#error`.

See `epic-common/MANUAL.md` §6 for two HAL-wide design decisions that used
to be listed here (`EPIC_IRQ_Enable` not setting the global enable, and
there being no `HAL_PPP_MspInit`) — they're true of every family, not a
PIC16F87XA-specific gap, so they moved to the shared manual.

---

## 25. Appendix: datasheet section index

Where each HAL module lives in DS39582B:

| HAL module                 | DS39582B section             |
|----------------------------|------------------------------|
| Register file / SFR map    | Figure 2-3, 2-4; Table 3-1   |
| Data EEPROM                | §3.0                         |
| PORTA / B / C / D / E      | §4.1-§4.5 (Tables 4-1..4-9)  |
| PSP (PORTE/TRISE)          | §4.5 (Register 4-9)          |
| Timer0                     | §5.0, §5.3                   |
| Timer1                     | §6.0                         |
| Timer2                     | §7.0                         |
| CCP1 / CCP2                | §8.0 (Tables 8-1..8-4)       |
| MSSP (SPI / I²C)           | §9.0                         |
| USART                      | §10.0 (§10.1 BRG)            |
| ADC                        | §11.0                        |
| Comparator                 | §12.0 (Figure 12-1)          |
| Voltage reference          | §13.0                        |
| Configuration Word         | §14.1 (Register 14-1)        |
| PCON (BOR/POR)             | §14.10 (Register 14-2)       |
| Interrupts                 | §14.11 (Figure 14-10)        |
| WDT                        | §14.13                       |
| Sleep / Power-down         | §14.14                       |
| Reset values               | Table 14-6                   |

When a header or driver makes a claim you want to verify, the citation in
its comment is the section to check against
[DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf).
