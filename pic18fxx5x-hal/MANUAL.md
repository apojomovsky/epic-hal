# PIC18F2455 family HAL, Manual

Family-agnostic conventions, the handle pattern, status codes, the harness,
and the host-sim/target build-time split: see `epic-common/MANUAL.md`. This
manual covers only what is genuinely specific to the **PIC18F2455 / 2550 /
4455 / 4550** family, verified against the Microchip datasheet **DS39632E**.

---

## Contents

1. [What this is](#1-what-this-is)
2. [The big picture](#2-the-big-picture)
3. [Quick start](#3-quick-start)
4. [Build systems](#4-build-systems)
5. [The host simulation backend](#5-the-host-simulation-backend)
6. [Interrupts](#6-interrupts)
7. [Core: WDT, Sleep, BOR/POR](#7-core-wdt-sleep-borpor)
8. [GPIO](#8-gpio)
9. [Timer0](#9-timer0)
10. [Timer1](#10-timer1)
11. [Timer2](#11-timer2)
12. [Timer3](#12-timer3)
13. [ECCP1 / CCP2, Capture, Compare, PWM](#13-eccp1--ccp2-capture-compare-pwm)
14. [MSSP, SPI and I²C](#14-mssp-spi-and-i²c)
15. [EUSART](#15-eusart)
16. [ADC](#16-adc)
17. [Comparator](#17-comparator)
18. [Data EEPROM](#18-data-eeprom)
19. [Streaming Parallel Port (SPP)](#19-streaming-parallel-port-spp)
20. [The SFR layer](#20-the-sfr-layer)
21. [Device selection](#21-device-selection)
22. [The examples](#22-the-examples)
23. [Known gaps and gotchas](#23-known-gaps-and-gotchas)
24. [Appendix: datasheet section index](#24-appendix-datasheet-section-index)

---

## 1. What this is

Four pin-compatible parts sharing one datasheet (DS39632E):

| Part      | Pins    | Flash | RAM   | EEPROM | I/O | ADC ch | CCP/ECCP | SPP | PORTD/E |
|-----------|---------|-------|-------|--------|-----|--------|----------|-----|---------|
| 18F2455   | 28      | 24 KB | 2048B | 256B   | 24  | 10     | 2/0      | no  | no      |
| 18F2550   | 28      | 32 KB | 2048B | 256B   | 24  | 10     | 2/0      | no  | no      |
| 18F4455   | 40/44   | 24 KB | 2048B | 256B   | 35  | 13     | 1/1      | yes | yes     |
| 18F4550   | 40/44   | 32 KB | 2048B | 256B   | 35  | 13     | 1/1      | yes | yes     |

All four have the USB module, MSSP (SPI + I²C master), EUSART, two analog
comparators, and the dual-priority interrupt scheme (vectors at 0008h high,
0018h low; DS39632E §9.0). USB and the extended instruction set are out of
scope for this HAL (large enough to be their own effort).

**Not present on this family (present on PIC16F87XA, do not expect it here):**
a voltage-reference (Vref) module/driver. PIC18F2455/2550/4455/4550 has no
on-chip Vref peripheral analogous to PIC16F87XA's — the comparator's
CVRCON (comparator voltage reference) is a separate module not yet ported,
not a Vref equivalent.

## 2. The big picture

```
pic18fxx5x-hal/
├── include/
│   ├── pic18fxx5x.h              family header: device select, SFR mapping;
│   │                              pulls shared status codes from epic-common
│   ├── pic18fxx5x_sfr.h          SFR address map + bit names (1:1 DS39632E)
│   ├── pic18fxx5x_sim.h          simulation backend public API
│   ├── epic_hal.h                 family-neutral top-level include (pulls in
│   │                              every peripheral, used by family-agnostic
│   │                              consumers like the task manager)
│   ├── host/pic18_platform.h     SFRs -> memory array, real weak attribute
│   ├── target/pic18_platform.h   SFRs -> volatile deref, no weak
│   ├── core/
│   │   ├── pic18_irq.h                 PIC18_IRQn enum + EPIC_IRQ_* backend
│   │   ├── hal_irq.h                   family-neutral pointer to pic18_irq.h
│   │   ├── pic18fxx5x_wdt_sleep.h       WDT/Sleep/BOR/POR helpers (RCON-based)
│   │   └── hal_wdt_sleep.h              family-neutral pointer
│   └── peripherals/              one .h per peripheral, Cube-style, plus a
│                                  hal_<ppp>.h family-neutral pointer for the
│                                  ones a family-agnostic consumer needs
├── src/
│   ├── core/                     pic18_irq.c, pic18_irq_dispatch.c,
│   │                              pic18_isr_vector.c (XC8 only), wdt/sleep
│   ├── peripherals/              implementations of peripherals/ headers
│   └── sim/pic18_sim.c            host simulation backend (host build only)
├── tests/                        example_blink, example_smoke, one
│                                  per-peripheral host-sim smoke test
├── mcu/pic18fxx5x-mplabx/        XC8 Makefile (thin caller of epic-common/mk)
└── CMakeLists.txt                host build (thin caller of epic-common/cmake)
```

The `epic-common`/per-family split, the include-path-selected platform
header trick, and the host/target execution-model split are identical in
substance to PIC16F87XA's, see `epic-common/MANUAL.md` §2. The one thing
that genuinely differs at this level is interrupt vectoring:

### 2.1 Two interrupt vectors, priority-gated

Unlike PIC16F87XA's single vector at 0004h, this family has **two**
vectors: 0008h (high-priority) and 0018h (low-priority), gated by
`RCON<IPEN>` (DS39632E §9.0). Both vectors delegate to the same shared
`epic_dispatch_all_irqs()` (`src/core/pic18_isr_vector.c` on target, the
harness's registered sim callback on host) — the hardware has already
separated high- from low-priority sources by which vector it took, so
calling the full dispatch from both is correct; each peripheral handler
still checks its own flag. See [§6](#6-interrupts) for the priority API.

## 3. Quick start

### Host simulation

```sh
cd pic18fxx5x-hal
cmake -B build -S .
cmake --build build
./build/example_blink
```

`RB0 toggled 9 times`, exit 0 (Timer0 8-bit, 1:256 prescaler, 600k sim
cycles / 65536 ≈ 9 overflows). `./build/example_smoke` also runs (harness
seam only, prints `smoke: 10 ticks, device <PART>`).

### Real target (XC8)

The HAL has no standalone real-target build of its own any more
(`pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/Makefile` is gone); build it
through one of this family's modules, which compile the full HAL
either way:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
python3 scripts/epic_build.py build --module epic-tick --mcu 18F4550 --run   # also 18F2455 / 18F2550 / 18F4455
```

Produces `build/18F4550-tick.hex` with vectors placed at 0008h/0018h.
Requires the **PIC18Fxxxx Device Family Pack**, which XC8 does not
bundle (unlike the PIC16Fxxx DFP): see §4 and
`mcu/pic18fxx5x-mplabx/README.md`.

## 4. Build systems

Same CMake shape as PIC16F87XA for the host side, including the
no-`#ifdef` host/target link-time split described in
`epic-common/MANUAL.md` §2. The real-target side is manifest-driven
(`epic-common/manifest/README.md`); the `xc8-cc` driver compiles each
translation unit to a `.p1` (p-code) intermediate, not a `.o`, and
needs `-mdfp` for a Device Family Pack, same as the PIC16 build (see
`pic16f87xa-hal/MANUAL.md` §4.2 for those XC8 v3.x mechanics, identical
here). PIC18-specific `epic_build.py build` flags:

| Flag        | Default    | Meaning                                    |
|-------------|------------|---------------------------------------------|
| `--mcu`     | (required) | Target part (2455/2550/4455/4550).          |
| `--fosc-hz` | `48000000` (the family's default) | Oscillator frequency. |
| `--dfp-dir` | see below  | PIC18Fxxxx DFP path for `-mdfp`.             |

**The PIC18Fxxxx DFP is not bundled with XC8** and must be installed once
(via MPLAB X *Tools → Packs → Pack Manager*, search `PIC18Fxxxx_DFP`, or
manually from [packs.download.microchip.com](https://packs.download.microchip.com/)
into `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`). Without
it, the XC8 build fails with `error: (2104) no device-support files found`.

The Phase 1 config-word defaults are conservative (HS oscillator, WDT on,
LVP off, XINST off, code/write/read protection off); the block-3
code-protection settings (`CP3`/`WRT3`/`EBTR3`) are emitted only for the
32 KB parts (18F2550/18F4550), since the 24 KB parts stop at block 2
(DS39632E §23.1).

## 5. The host simulation backend

Public API in `pic18fxx5x_sim.h`, backed by a flat 4096-byte memory array
`pic18_sim_sfr[]` (`src/sim/pic18_sim.c`) indexed by the physical 12-bit
address — no BSR/Access-Bank translation modeled; every MVP-and-beyond SFR
this HAL uses lives in the Access Bank (0xF60-0xFFF) so none is needed
(`docs/multi-family-plan.md`, Phase 2 resolved question). The function-name
shape mirrors `pic16f87xa_sim_*` (`pic18_sim_reset/step/drive_input/
read_output/set_irq_callback`), plus one `drive_*` hook per peripheral that
needs test-injected completion:

```c
void    pic18_sim_reset(void);
void    pic18_sim_step(uint32_t ticks);
void    pic18_sim_drive_input(char port, uint8_t pin, uint8_t level);
uint8_t pic18_sim_read_output(char port, uint8_t pin);   /* reads LATx for outputs */
void    pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb);

void    pic18_sim_drive_ssp_rx(uint8_t data);
void    pic18_sim_drive_usart_rx(uint8_t data);
void    pic18_sim_drive_comp(uint8_t c1out, uint8_t c2out);
void    pic18_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);
void    pic18_sim_drive_eeprom_done(uint8_t addr, uint8_t data);
uint8_t pic18_sim_eeprom_read(uint8_t addr);
void    pic18_sim_drive_adc_done(uint16_t result);
#if PIC18FXX5X_FAMILY_HAS_SPP
void    pic18_sim_drive_spp(uint8_t wrspp, uint8_t rdspp);
#endif
```

What it models: Timer0 (8/16-bit, prescaler, overflow → `TMR0IF`), Timer1/2/3
stepping (prescalers, postscaler for Timer2, overflow → the matching PIR
flag), GPIO drive/read (`pic18_sim_read_output` reads `LATx` for output
pins, per the LATx-not-PORTx GPIO model, [§8](#8-gpio)). The `drive_*` hooks
let a test act as the outside world for MSSP, EUSART, the comparator,
EEPROM, ADC and (40/44-pin only) SPP, the same role the PIC16 sim's
`drive_*` helpers play, see `epic-common/MANUAL.md` §5 for the generic
dispatch/writing-a-sim-test pattern, identical here.

## 6. Interrupts

*DS39632E §9.0.*

`RCON<IPEN>` (bit 7) selects the mode: `IPEN=0` is PIC16-compatible
(single vector at 0008h, `GIE`/`PEIE`, no priority); `IPEN=1` (this HAL's
default) enables the two-vector, per-source-priority scheme, with `GIEH`
gating high-priority sources and `GIEL` gating low-priority ones. `INT0`
has no priority bit and is always high-priority.

```c
typedef enum {
    PIC18_IRQ_INT0      = 0,   PIC18_IRQ_INT1      = 1,
    PIC18_IRQ_INT2      = 2,   PIC18_IRQ_RB        = 3,
    PIC18_IRQ_TMR0      = 4,   PIC18_IRQ_TMR1      = 5,
    PIC18_IRQ_TMR2      = 6,   PIC18_IRQ_TMR3      = 7,
    PIC18_IRQ_CCP1      = 8,   PIC18_IRQ_SSP       = 9,
    PIC18_IRQ_USART_TX  = 10,  PIC18_IRQ_USART_RX  = 11,
    PIC18_IRQ_ADC       = 12,  PIC18_IRQ_CCP2      = 13,
    PIC18_IRQ_CMP       = 14,  PIC18_IRQ_EEPROM    = 15,
#if PIC18FXX5X_FAMILY_HAS_SPP
    PIC18_IRQ_SPP       = 16,  /* 40/44-pin only */
#endif
} PIC18_IRQn;

uint8_t EPIC_IRQ_Disable(void);                              /* clears GIEH+GIEL, returns prior state */
void    EPIC_IRQ_Restore(uint8_t prev_state);                 /* also ensures IPEN=1 */
void    EPIC_IRQ_Enable(PIC18_IRQn irq);
void    EPIC_IRQ_DisableSrc(PIC18_IRQn irq);
void    EPIC_IRQ_ClearFlag(PIC18_IRQn irq);
uint8_t EPIC_IRQ_GetFlag(PIC18_IRQn irq);
void    EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio);  /* no-op for INT0 */
```

`EPIC_IRQ_Restore(1)` is the drop-in equivalent of PIC16's `GIE = 1` (see
`epic-common/MANUAL.md` §6 for the shared enable/disable API and §7 for
the ISR-writing pattern, identical here) — it sets both master enables and ensures
priority mode is active. `EPIC_IRQ_SetPriority` writes the matching bit in
`INTCON2`/`INTCON3`/`IPR1`/`IPR2`; sources reset to high priority
(`INTCON2`/`INTCON3`/`IPR1`/`IPR2` reset all-ones, DS39632E Table 5-1).
`EPIC_IRQ_SetPriority` is the one shared-contract extension PIC18 needs and
PIC16 implements as a no-op (`docs/multi-family-plan.md`, Phase 2 resolved
question).

Sources, by register: `INTCON` (TMR0, INT0, RB change), `INTCON3` (INT1,
INT2), `PIR1` (TMR1, TMR2, CCP1, SSP, USART RX/TX, ADC, SPP), `PIR2` (TMR3,
CCP2, Comparator, EEPROM).

### XC8 dual-priority ISR syntax

Resolved against the XC8 C Compiler User Guide for PIC (DS50002737K)
§5.9.1.2 "Writing Dual-priority or Legacy Mode ISRs":

```c
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void) { epic_dispatch_all_irqs(); }
void __interrupt(low_priority)  PIC18_IRQ_HandlerLow(void)  { epic_dispatch_all_irqs(); }
```

No priority argument defaults to high priority; Microchip recommends always
specifying it. The legacy `#pragma interrupt`/`#pragma interruptlow` form is
obsolete in XC8 v2+. Both vectors delegate to the one shared
`epic_dispatch_all_irqs()` (`src/core/pic18_irq_dispatch.c`), matching
PIC16's single-dispatcher shape, see `epic-common/MANUAL.md` §2.3.

## 7. Core: WDT, Sleep, BOR/POR

*DS39632E §4.0 (RCON, Register 4-1), §9.0 (`RCON<IPEN>`), §3.0 (Sleep).*

```c
void    EPIC_WDT_Refresh(void);     // asm("clrwdt") on target; no-op on host
void    EPIC_Sleep_Enter(void);     // asm("sleep")  on target; no-op on host
uint8_t EPIC_BOR_GetStatus(void);   // RCON<BOR> (bit 0)
void    EPIC_BOR_ClearFlag(void);
uint8_t EPIC_POR_GetStatus(void);   // RCON<POR> (bit 1)
void    EPIC_POR_ClearFlag(void);
```

Same function names/signatures as PIC16F87XA (portable caller code), but
PIC18 folds the reset-status bits into **`RCON`** rather than a separate
`PCON`: `RCON<TO>` (bit 3, WDT time-out, 1 = not timed out), `RCON<PD>`
(bit 2, Sleep detection, 1 = not in Sleep), `RCON<POR>` (bit 1), `RCON<BOR>`
(bit 0). `RCON<IPEN>` (bit 7) is the priority-mode bit, [§6](#6-interrupts).

## 8. GPIO

*DS39632E §10.0.*

Same `EPIC_GPIO_*` names/signatures as PIC16F87XA, but a real hardware
improvement: **writes go through `LATx`, not `PORTx`.** PIC18 exposes the
output latch as its own mapped register, so there's no PORTx
read-modify-write to corrupt an input pin's state on a shared write (the
PIC16 driver's read-modify-write-of-the-latch caveat does not apply here).

```c
typedef enum {
    GPIOA=0, GPIOB=1, GPIOC=2,
#if PIC18FXX5X_FAMILY_HAS_PORTD
    GPIOD=3,   /* 40/44-pin only */
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
    GPIOE=4,   /* 40/44-pin only */
#endif
} GPIO_TypeDef;

void EPIC_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);
void EPIC_GPIO_DeInit(GPIO_TypeDef port);
void EPIC_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);  // writes LATx
void EPIC_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);                      // LATx ^= mask
GPIO_PinState EPIC_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);               // reads PORTx
void EPIC_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);                      // writes LATx
uint8_t EPIC_GPIO_ReadPort(GPIO_TypeDef port);                                   // reads PORTx
void EPIC_GPIO_SetPullups(GPIO_PullTypeDef pull);   // INTCON2<RBPU>, inverted, PORTB only
```

PORTA is 6-bit (RA0-RA5), PORTE is 3-bit; PORTD/PORTE exist only on
40/44-pin parts (18F4455/4550). Direction is `TRISx` (1 = input, 0 =
output), same encoding as PIC16.

### 8.1 PORTB change interrupt (RB<7:4>)

*DS39632E §9.0 / §10.2, INTCON<RBIF>/<RBIE>.*

Same names/signatures as PIC16F87XA's hook (the fixed contract across
families), PIC18 register-level body. A whole-port callback for the
RB<7:4> interrupt-on-change source, the same shape as Timer2's
one-callback-per-handle but simpler (there is only one PORTB, so no
handle struct). Added so family-agnostic modules (notably `epic-encoder`)
can consume the RB-change interrupt without a family-specific include;
the family-neutral shim `peripherals/hal_gpio.h` pulls this header in.

```c
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));
void RB_IRQHandler(void) EPIC_WEAK;
```

`EPIC_GPIO_RegisterChangeCallback` stores one callback slot (NULL is safe,
the handler no-ops). `RB_IRQHandler` is the weak ISR the dispatcher fans
out to; the default body reads PORTB, clears RBIF, then calls the
registered callback with that already-read byte. On PIC18 the RB source
also has a priority bit (`INTCON2<RBIP>`), set via
`EPIC_IRQ_SetPriority(PIC18_IRQ_RB, prio)`; it defaults to high priority
after reset like every other source (INTCON2 resets to all-ones,
DS39632E Table 5-1), and only takes effect in priority mode (`IPEN = 1`,
the mode `EPIC_IRQ_Restore` enables).

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
`EPIC_IRQ_Enable(PIC18_IRQ_RB)` and `EPIC_IRQ_Restore(1)`.

**Host sim.** The sim does not auto-assert RBIF on a PORTB mismatch
(faithfully modeling the datasheet "snapshot on every PORTB read" would
require intercepting every CPU read of PORTB through the `EPIC_REG8`
macro, disproportionate for the one feature that needs it). Tests that
exercise the handler assert RBIF directly in INTCON, then call
`RB_IRQHandler` / `epic_dispatch_all_irqs` and check the callback's byte
and the post-call flag state. This proves the handler's read/clear/callback
ordering, the part that actually matters. See
`tests/example_rb_change.c` and `epic-encoder/docs/ARCHITECTURE.md`.

## 9. Timer0

*DS39632E §11.0, Register 11-1 (T0CON), Table 11-1 (prescaler encoding).*

Same API as PIC16F87XA's Timer0, but controlled by its own dedicated
`T0CON` register (not `OPTION_REG`), with the prescaler dedicated to Timer0
(not shared with the WDT as on PIC16), and an added 8/16-bit mode bit.

```c
typedef struct {
    TIMER0_BitModeTypeDef      Mode;              /* 8-bit (default) or 16-bit, T0CON<T08BIT> */
    TIMER0_ClockSourceTypeDef  ClockSource;       /* internal Fosc/4 or T0CKI */
    TIMER0_ClockEdgeTypeDef    ClockEdge;         /* T0CKI edge */
    TIMER0_PrescalerTypeDef    Prescaler;         /* 1:2..1:256 */
    bool                       PrescalerAssigned; /* true -> prescaler assigned to TMR0 */
    uint8_t                    ReloadValue;       /* low byte (8-bit) or TMR0L */
    void (*OverflowCallback)(void);
} TIMER0_HandleTypeDef;

EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);
uint8_t  EPIC_TIMER0_ReadCounter(void);     // TMR0L only, even in 16-bit mode
void     EPIC_TIMER0_WriteCounter(uint8_t value);
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);
void     TIMER0_IRQHandler(void) EPIC_WEAK;
```

**Default is 8-bit mode** (`T08BIT=1`), matching the PIC16 Timer0 model the
task manager uses as its tick source — a drop-in for existing PIC16 caller
code. Prescaler encoding matches PIC16: `T0PS2:T0PS0=000` is 1:2, not 1:1;
"no prescaler" is `PrescalerAssigned=false` (PSA=1, Timer0 gets the raw
clock). Overflow sets `INTCON<TMR0IF>` (not a PIR bit).

### Example

```c
TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;   // 8-bit, internal, 1:256, reload 0
h.OverflowCallback = on_t0_overflow;
EPIC_TIMER0_Init(&h);
EPIC_TIMER0_Start(&h);
EPIC_IRQ_Restore(1);
```

At FOSC = 20MHz (FCY = 5MHz), 1:256 overflows every 256 × 256 × 0.2µs ≈
13ms — `example_blink` observes ~9 toggles in 600,000 sim cycles
(65536 cycles/overflow).

## 10. Timer1

*DS39632E §12.0, Register 12-1 (T1CON).*

Same API as PIC16F87XA's Timer1 (16-bit, T1OSC crystal support). T1CON adds
two bits in the top byte PIC16 left unimplemented: `RD16` (T1CON<7>,
16-bit read/write mode — this driver sets it on init so the atomic
read-latches-high-byte idiom works) and `T1RUN` (T1CON<6>, read-only
system-clock-status, ignored).

```c
typedef struct {
    TIMER1_ClockSourceTypeDef  ClockSource;   /* internal Fosc/4 or external/T1OSC */
    TIMER1_ClockSyncTypeDef    ClockSync;     /* sync or async external */
    TIMER1_OscillatorTypeDef   Oscillator;    /* T1OSC crystal on/off */
    TIMER1_PrescalerTypeDef    Prescaler;     /* 1:1/1:2/1:4/1:8 */
    uint16_t                   ReloadValue;
    void (*OverflowCallback)(void);
} TIMER1_HandleTypeDef;

EPIC_StatusTypeDef EPIC_TIMER1_Init/DeInit/Start/Stop(...);
uint16_t EPIC_TIMER1_ReadCounter(void);      // atomic (RD16 latches TMR1H on TMR1L read)
void     EPIC_TIMER1_WriteCounter(uint16_t value);
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);
void     TIMER1_IRQHandler(void) EPIC_WEAK;
```

Overflow sets `PIR1<TMR1IF>`. Reset: `T1CON = 0x00`; the driver sets RD16
on init. `example_timer1` (host sim): internal clock, 1:1, expects TMR1IF
every 65536 cycles, verified over 3 overflows.

## 11. Timer2

*DS39632E §12.0, Register 12-2 (T2CON), Register 12-3 (PR2).*

Same API as PIC16F87XA's Timer2 (8-bit, `PR2` period register, postscaler)
— simpler here because PIC18 puts `PR2` in the Access Bank (0xFCB), no bank
switching (PIC16's PR2 lives in Bank 1). `T2CON`'s layout and reset value
(0x00) match PIC16.

```c
typedef struct {
    TIMER2_PrescalerTypeDef   Prescaler;    /* 1:1/1:4/1:16 */
    TIMER2_PostscalerTypeDef  Postscaler;   /* 1:1..1:16 */
    uint8_t                   Period;       /* PR2, 0..255 */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

EPIC_StatusTypeDef EPIC_TIMER2_Init/DeInit/Start/Stop(...);
uint8_t  EPIC_TIMER2_ReadCounter(void);
void     EPIC_TIMER2_WriteCounter(uint8_t value);
uint8_t  EPIC_TIMER2_ReadPeriod(void);
void     EPIC_TIMER2_WritePeriod(uint8_t period);
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);
void     TIMER2_IRQHandler(void) EPIC_WEAK;
```

`TMR2IF` (`PIR1<1>`) fires every `prescaler × (PR2+1) × postscaler`
instruction cycles and drives the CCP/ECCP PWM time base ([§13](#13-eccp1--ccp2-capture-compare-pwm)).
Reset: `T2CON=0x00`, `PR2=0xFF`. `example_timer2`: PR2=249, 1:1/1:1 → 250
cycles/period, 5 overflows verified, first overflow observed at cycle 250
(±2 slack for sim bookkeeping).

## 12. Timer3

*DS39632E §14.0, Register 14-1 (T3CON).*

A second 16-bit timer alongside Timer1, not present on PIC16F87XA. Mirrors
Timer1's API (`TIMER3_HandleTypeDef`, `EPIC_TIMER3_*`, weak
`TIMER3_IRQHandler`), differences:

- No oscillator field — Timer3 shares Timer1's T1OSC crystal
  (`T1CON<T1OSCEN>`); `T3CON` has no `T3OSCEN` bit.
- `T3CON<T3CCP2:T3CCP1>` (bits 6, 3) select whether Timer1 or Timer3 is the
  capture/compare time base for CCP1/ECCP1 and CCP2. This driver leaves
  them at reset (00 = Timer1 for both); the CCP driver manages them when
  Timer3 is needed as the time base.
- `RD16` (T3CON<7>) set by this driver, same idiom as Timer1.
- Overflow sets **`PIR2<TMR3IF>`** (Timer1 sets `PIR1<TMR1IF>`).

```c
typedef struct {
    TIMER3_ClockSourceTypeDef  ClockSource;
    TIMER3_ClockSyncTypeDef    ClockSync;
    TIMER3_PrescalerTypeDef    Prescaler;
    uint16_t                   ReloadValue;
    void (*OverflowCallback)(void);
} TIMER3_HandleTypeDef;

EPIC_StatusTypeDef EPIC_TIMER3_Init/DeInit/Start/Stop(...);
uint16_t EPIC_TIMER3_ReadCounter(void);
void     EPIC_TIMER3_WriteCounter(uint16_t value);
uint16_t EPIC_TIMER3_PrescalerToRatio(TIMER3_PrescalerTypeDef p);
void     TIMER3_IRQHandler(void) EPIC_WEAK;
```

Timer3 + Timer1 can form a 32-bit timer (DS39632E §14.0) when both are
configured identically; this driver exposes Timer3 as an independent
16-bit timer only. `example_timer3`: internal clock, 1:8 prescaler → 524288
cycles/overflow, 3 overflows verified.

## 13. ECCP1 / CCP2, Capture, Compare, PWM

*DS39632E §16.0, Register 16-1 (CCP1CON), Register 16-2 (ECCP1DEL),
Register 16-3 (ECCP1AS).*

Two CCP modules, asymmetric: **CCP1 is Enhanced (ECCP1)**, CCP2 is plain.
The API mirrors PIC16's `EPIC_CCP_*` (same `CCP_InstanceTypeDef`,
`CCP_ModeTypeDef`, `CCP_PWMConfigTypeDef`, weak `CCP1_IRQHandler`/
`CCP2_IRQHandler`); the handle adds three ECCP-only fields CCP2 ignores.
PIC18 also adds a `CCP_MODE_COMPARE_TOGGLE` (0010) mode PIC16 lacks.

**ECCP1-only features** (DS39632E §16.4-16.6), none present on PIC16's
plain CCP:
- **Multi-output PWM** (`CCP_PWMOutputTypeDef`/`P1M`): single, half-bridge,
  full-bridge forward/reverse.
- **Programmable dead-band delay + auto-restart** (`ECCP1DEL`): `Delay` is
  a 7-bit instruction-cycle count (`PDC6:PDC0`); `AutoRestart` (`PRSEN`)
  auto-clears the shutdown state when the source deasserts, vs. requiring
  firmware to call `EPIC_CCP_Restart`.
- **Auto-shutdown** (`ECCP1AS`): source select (comparators and/or the
  external `FLT0` pin) plus per-pin-pair states (drive 0, drive 1,
  tri-state) applied on a shutdown event.

```c
typedef struct {
    CCP_InstanceTypeDef            Instance;
    CCP_ModeTypeDef                Mode;
    uint16_t                       CompareValue;
    CCP_PWMConfigTypeDef           PWM;            /* { Period, Duty } */
    CCP_PWMOutputTypeDef           PWMOutputMode;  /* ECCP1 (CCP1) only */
    CCP_DeadBandConfigTypeDef      DeadBand;       /* ECCP1 (CCP1) only */
    CCP_AutoShutdownConfigTypeDef  AutoShutdown;   /* ECCP1 (CCP1) only */
    void (*EventCallback)(void);
} CCP_HandleTypeDef;

EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);
void     EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);
void     EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty);   // 0..1023
void     EPIC_CCP_ConfigDeadBand(CCP_InstanceTypeDef inst, uint8_t delay, bool auto_restart);  // no-op for CCP2
void     EPIC_CCP_ConfigAutoShutdown(CCP_InstanceTypeDef inst, CCP_AutoShutdownSourceTypeDef source,
                                     CCP_PinStateTypeDef pins_ac, CCP_PinStateTypeDef pins_bd);  // no-op for CCP2
uint8_t  EPIC_CCP_IsShutdown(CCP_InstanceTypeDef inst);
void     EPIC_CCP_Restart(CCP_InstanceTypeDef inst);   // no-op for CCP2
void     CCP1_IRQHandler(void) EPIC_WEAK;
void     CCP2_IRQHandler(void) EPIC_WEAK;
```

Timer resources (Table 16-1): Capture/Compare use Timer1 **or** Timer3
(`T3CON<T3CCP2:T3CCP1>` select, default Timer1); PWM always uses Timer2 —
start it first with a period matching `h->PWM.Period`.

### Example: ECCP1 half-bridge PWM with dead-band

```c
TIMER2_HandleTypeDef th = TIMER2_HANDLE_DEFAULT;
th.Period = 99;  EPIC_TIMER2_Init(&th);         // PR2=99 -> 100-cycle period

CCP_HandleTypeDef ch = { .Instance = CCP_INSTANCE_1, .Mode = CCP_MODE_PWM,
                          .PWM = { .Period = 99, .Duty = 50 },            // 50%
                          .PWMOutputMode = CCP_PWM_OUTPUT_HALF_BRIDGE,
                          .DeadBand = { .Delay = 12, .AutoRestart = true } };
EPIC_CCP_Init(&ch);
EPIC_TIMER2_Start(&th);
```

Verified register image: `CCPR1L = 50>>2 = 0x0C`, `CCP1CON =
P1M(10)<<6 | duty[1:0]<<4 | PWM(1100) = 0xAC`, `ECCP1DEL = PRSEN|12 = 0x8C`.

## 14. MSSP, SPI and I²C

*DS39632E §19.0, Registers 19-1..19-6.*

One MSSP module (RC3/SCK/SCL, RC4/SDI/SDA, RC5/SDO). Same API as PIC16's
`EPIC_SSP_*`; the only real differences are that every MSSP register is in
the Access Bank (no bank switching) and the control register is
**`SSPCON1`** (PIC16's is `SSPCON`). ⚠ Register-level only — no I²C state
machine; the caller drives Start/Stop/ACK by hand, same as PIC16.

```c
typedef struct {
    SSP_ModeTypeDef           Mode;
    SSP_ClockEdgeTypeDef      ClockEdge;       /* SPI only */
    SSP_ClockPolarityTypeDef  ClockPolarity;   /* SPI only */
    SSP_SamplePhaseTypeDef    SamplePhase;     /* SPI master only */
    uint8_t                   SSPADD;          /* I2C slave addr / master baud reload */
    void (*TransferCallback)(void);
} SSP_HandleTypeDef;

EPIC_StatusTypeDef EPIC_SSP_Init/DeInit(...);
uint16_t EPIC_SSP_WriteByte(uint8_t data);   // 0xFFFF on WCOL (not written)
uint8_t  EPIC_SSP_ReadByte(void);
uint8_t  EPIC_SSP_IsBufferFull(void);
uint8_t  EPIC_SSP_HasWriteCollision(void);
void     EPIC_SSP_ClearWriteCollision(void);
uint16_t SSP_ComputeSSPADD(uint32_t fosc_hz, uint32_t fscl_hz);  // Fscl = Fosc/(4*(SSPADD+1))
void     EPIC_SSP_Start(void);  void EPIC_SSP_RepeatedStart(void);  void EPIC_SSP_Stop(void);
void     EPIC_SSP_ReceiveEnable(void);  void EPIC_SSP_AcknowledgeEnable(void);
uint8_t  EPIC_SSP_AcknowledgeStatus(void);
void     SSP_IRQHandler(void) EPIC_WEAK;
```

`example_ssp` (host sim) verifies: `SSP_ComputeSSPADD(16MHz, 100kHz) == 39`,
`SSP_ComputeSSPADD(16MHz, 400kHz) == 9`; SPI master Fosc/4 init sets
`SSPCON1 == 0x20`; write/read loopback via `pic18_sim_drive_ssp_rx`; I²C
master `Start`/`Stop` set `SSPCON2<SEN>`/`<PEN>`.

## 15. EUSART

*DS39632E §20.0, Registers 20-1 (TXSTA), 20-2 (RCSTA), 20-3 (BAUDCON),
§20.1 (BRG, Table 20-1).*

Same API as PIC16's `EPIC_USART_*`. The PIC18 EUSART adds, all via `BAUDCON`
+ the `SPBRGH` high byte:
- **16-bit baud generator** (`BAUDCON<BRG16>`), extending the divisor range
  with `SPBRG:SPBRGH` (vs. PIC16's 8-bit-only `SPBRG`).
- **Auto-baud detection** (`BAUDCON<ABDEN>`): hardware measures the next
  incoming byte and loads `SPBRG:SPBRGH`; `ABDEN` self-clears when done.
- **9-bit address-detect mode** (`RCSTA<ADDEN>`).

```c
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16);   // 4th param vs. PIC16: BRG16 select

typedef struct {
    USART_ModeTypeDef          Mode;
    USART_ClockSourceTypeDef   ClockSource;    /* sync only */
    USART_BaudRateHighTypeDef  BaudHigh;
    USART_BaudGenTypeDef       BaudGen;        /* 8- or 16-bit BRG */
    USART_DataWidthTypeDef     DataWidth;
    uint8_t                    SPBRG, SPBRGH;
    uint8_t                    AddressDetect;  /* ADDEN */
    uint8_t                    AutoBaud;       /* ABDEN on init */
    void (*TxCpltCallback)(void);
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

EPIC_StatusTypeDef EPIC_USART_Init/DeInit(...);
void    EPIC_USART_Transmit(uint8_t data);           // TXIF cleared by writing TXREG, not reading
uint8_t EPIC_USART_GetTX9D(void);  void EPIC_USART_SetTX9D(uint8_t bit9);
uint8_t EPIC_USART_IsTxShiftRegisterEmpty(void);
uint8_t EPIC_USART_Receive(void);                     // clears RCIF, advances 2-deep FIFO
uint8_t EPIC_USART_GetRX9D(void);
uint8_t EPIC_USART_HasOverrun(void);  void EPIC_USART_ClearOverrun(void);   // cycles CREN
void    EPIC_USART_StartAutoBaud(void);
uint8_t EPIC_USART_IsAutoBaudBusy(void);
uint8_t EPIC_USART_HasAutoBaudOverflow(void);  void EPIC_USART_ClearAutoBaudOverflow(void);
void    USART_RX_IRQHandler(void) EPIC_WEAK;
void    USART_TX_IRQHandler(void) EPIC_WEAK;
```

Baud formulas (Table 20-1): `BRG16=0,BRGH=0: Fosc/(64(X+1))`;
`BRG16=0,BRGH=1` and `BRG16=1,BRGH=0`: `Fosc/(16(X+1))`; `BRG16=1,BRGH=1`:
`Fosc/(4(X+1))`. `example_usart` verifies all four rows numerically (e.g.
16MHz/9600 baud → X=103 at BRGH=1/BRG16=0, X=25 at BRGH=0; 16MHz/115200 →
X=33 at BRG16=1/BRGH=1) and that the 16-bit BRG genuinely extends range: at
1200 baud/BRGH=1, the 8-bit BRG overflows (X=832 > 255 → `0xFFFF`) while
16-bit BRG accepts it. Only one EUSART instance exists on this family.

## 16. ADC

*DS39632E §21.0, Registers 21-1 (ADCON0), 21-2 (ADCON1), 21-3 (ADCON2).*

10-bit successive-approximation ADC — 10 channels (AN0-4, AN8-12) on
28-pin parts, 13 (AN0-12) on 40/44-pin. A **fresh design, not a mirror**
of PIC16's ADC: three control registers instead of two.

```c
typedef struct {
    ADC_ChannelTypeDef       Channel;        /* AN0..AN12 (4-bit CHS) */
    ADC_ClockSourceTypeDef   ClockSource;    /* ADCON2<ADCS2:0> */
    ADC_AcquisitionTypeDef   Acquisition;    /* ADCON2<ACQT2:0>, new vs PIC16 */
    ADC_ResultFormatTypeDef  ResultFormat;   /* ADCON2<ADFM> (moved off ADCON1) */
    ADC_VReferenceTypeDef    VReference;     /* ADCON1<VCFG1:VCFG0>, split +/- */
    uint8_t                  PinConfig;      /* raw PCFG3:PCFG0, Table 21-3 */
    void (*ConvCpltCallback)(uint16_t result);
} ADC_HandleTypeDef;

EPIC_StatusTypeDef EPIC_ADC_Init/DeInit(...);
uint16_t EPIC_ADC_Start(void);                    // 0xFFFF if already in progress
void     EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);
uint8_t  EPIC_ADC_IsConversionInProgress(void);   // GO/DONE
uint8_t  EPIC_ADC_IsConversionDone(void);         // ADIF
void     EPIC_ADC_ClearITFlag(void);
uint16_t EPIC_ADC_Read(void);                     // always returns 0..1023 regardless of ADFM
void     ADC_IRQHandler(void) EPIC_WEAK;
```

`PinConfig` is a raw 4-bit `PCFG3:PCFG0` code (Table 21-3 is large; the
driver takes it as-is rather than a curated enum — look up the table for
the analog/digital split you want). Vref is split into `VCFG0` (Vref+:
VDD vs AN3) and `VCFG1` (Vref-: VSS vs AN2), separate from `PCFG`.
`example_adc` verifies the init register image (`ADCON0=0x09`,
`ADCON1=0x00`, `ADCON2=0x89` for AN2/Fosc8/2Tad/right-justified/VDD-VSS),
both justifications reading back the same 0..1023 value, Vref bit
placement, channel-select, and DeInit zeroing all three registers.

## 17. Comparator

*DS39632E §22.0, Register 22-1 (CMCON), Figure 22-1 (the eight modes).*

Two on-chip comparators, same API and same eight `CMCON<CM2:CM0>` modes as
PIC16F87XA's comparator — the only difference is the register sits in the
Access Bank (0xFB4, no bank switching). The comparator voltage reference
(`CVRCON`) is a separate, not-yet-ported module (see [§1](#1-what-this-is)'s
note on the absent Vref driver).

```c
typedef struct {
    COMP_ModeTypeDef  Mode;
    bool              C1Inverted;   /* C1INV */
    bool              C2Inverted;   /* C2INV */
    bool              CIS;          /* comparator input switch */
    void (*ChangeCallback)(void);   /* fires on CMIF, PIR2<6> */
} COMP_HandleTypeDef;

EPIC_StatusTypeDef EPIC_COMP_Init/DeInit(...);
uint8_t EPIC_COMP_C1Out(void);  uint8_t EPIC_COMP_C2Out(void);
uint8_t EPIC_COMP_IsChangeFlag(void);  void EPIC_COMP_ClearChangeFlag(void);
void    COMP_IRQHandler(void) EPIC_WEAK;
```

POR default `CMCON = 0x07` (off). `example_comp` verifies
`CMCON == 0x3A` for two-independent + both inverted + CIS
(`mode(010) | CIS(0x08) | C1INV(0x10) | C2INV(0x20)`), output readback via
`pic18_sim_drive_comp`, and DeInit restoring `0x07`.

## 18. Data EEPROM

*DS39632E §7.0, Register 7-1 (EECON1), §7.1 (read), §7.2 (write/unlock).*

256 bytes. Same API as PIC16's `EPIC_EEPROM_*`. PIC18 moves the registers
into the Access Bank and adds `EEPGD`/`CFGS` select bits to `EECON1` (both
kept 0 by this driver for data-EEPROM access, as opposed to program-memory
access). No `EEADRH` on this family (`EEADR` is a full 8-bit address,
0..255).

```c
EPIC_StatusTypeDef EPIC_EEPROM_Init(void (*callback)(void));   // sets PIE2<EEIE> if non-NULL
EPIC_StatusTypeDef EPIC_EEPROM_DeInit(void);
uint8_t  EPIC_EEPROM_ReadByte(uint8_t addr);
EPIC_StatusTypeDef EPIC_EEPROM_WriteByte(uint8_t addr, uint8_t data);   // hides 0x55->0xAA unlock
void     EPIC_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);
EPIC_StatusTypeDef EPIC_EEPROM_WriteBuffer(uint8_t start, const uint8_t *buf, uint8_t len);
uint8_t  EPIC_EEPROM_IsWriteComplete(void);   // EEIF, PIR2<4>
void     EPIC_EEPROM_ClearITFlag(void);
void     EEPROM_IRQHandler(void) EPIC_WEAK;
```

Writes are non-blocking, same as PIC16: `WriteByte` returns once `WR` is
set; poll `IsWriteComplete` or use the IRQ. `example_eeprom` verifies read
of a preloaded cell, the unlock sequence (`EECON2` ends at `0xAA`),
completion via `pic18_sim_drive_eeprom_done`, and a 3-byte buffer
round-trip.

## 19. Streaming Parallel Port (SPP)

*DS39632E §18.0, Registers 18-1 (SPPCON), 18-2 (SPPCFG), 18-3 (SPPEPS).*

The PIC18 analog of PIC16F87XA's PSP — but USB-oriented, not a general
parallel bus. **40/44-pin only** (PIC18F4455/4550); including the header
on a 28-pin part is a compile-time `#error` (`PIC18FXX5X_FAMILY_HAS_SPP`).
Register-level only: programs `SPPCON`/`SPPCFG`/`SPPEPS`, byte-level
`SPPDATA` access, busy/read-occurred/write-occurred status, `SPPIF` — the
actual USB streaming protocol (endpoint management, CLK1/CLK2 toggling,
USB-vs-MCU ownership handoff) is left to the user, the same way MSSP
leaves the I²C state machine to the user.

```c
typedef struct {
    SPP_OwnershipTypeDef    Ownership;    /* MCU or USB owns the port (SPPOWN) */
    SPP_ClockConfigTypeDef  ClockConfig;  /* CLKCFG1:0 */
    bool                    CSEnable;     /* RB4 as SPP CS output */
    bool                    CLK1Enable;   /* RE0 as SPP CLK1 output */
    uint8_t                 WaitStates;   /* WS3:0, 0..15 -> 0..30 wait states */
    uint8_t                 Endpoint;     /* ADDR3:0, 0..15 */
    void (*TransferCallback)(void);
} SPP_HandleTypeDef;

EPIC_StatusTypeDef EPIC_SPP_Init/DeInit(...);
void    EPIC_SPP_WriteByte(uint8_t ep, uint8_t data);
uint8_t EPIC_SPP_ReadByte(uint8_t ep);
uint8_t EPIC_SPP_IsBusy(void);                 // SPPEPS<SPPBUSY>
uint8_t EPIC_SPP_HasWriteOccurred(void);        // SPPEPS<WRSPP>
uint8_t EPIC_SPP_HasReadOccurred(void);         // SPPEPS<RDSPP>
uint8_t EPIC_SPP_IsInterruptFlag(void);  void EPIC_SPP_ClearITFlag(void);
void    SPP_IRQHandler(void) EPIC_WEAK;
```

`example_spp` verifies init register images for both MCU-owned
(`SPPCON=0x01`) and USB-owned (`SPPCON=0x03`, clock config write/read, CS +
CLK1 enabled, 4 wait states) configurations, byte write/read against a
chosen endpoint, and simulated write/read status events.

## 20. The SFR layer

`pic18fxx5x_sfr.h` defines, 1:1 with DS39632E, using the **same
`PIC_REG_*`/`PIC_*_*` macro naming as PIC16F87XA** so cross-family code
that must drop to raw registers stays visually consistent (the addresses
of course differ). Every SFR this HAL touches lives in the **Access Bank**
(0xF60-0xFFF) — no bank-select helper is needed the way PIC16's
`pic_select_bank()` is, since there's no bank switching to do for any
register this HAL uses.

```c
EPIC_REG8(addr)                 // lvalue for the register at addr
EPIC_SFR_PTR(addr)              // address of that register
epic_sfr_read8(addr)            // read
epic_sfr_write8(addr, value)    // write (used over compound RMW where XC8
                                 // can't lower a compound assignment on a
                                 // volatile cast-lvalue at a runtime SFR
                                 // address — the Phase 2 EUSART codegen lesson)
```

Family capability macros (`PIC18FXX5X_FAMILY_HAS_PORTD/_PORTE/_SPP/_USB`,
flash/RAM/EEPROM/IO/ADC-channel counts) gate the parts that differ; see
[§21](#21-device-selection).

## 21. Device selection

Define exactly one of `PIC18F2455`, `PIC18F2550`, `PIC18F4455`,
`PIC18F4550` before including any HAL header; defaults to `PIC18F4550` (the
family's most-featured part) if none is defined, `#error`s if more than
one is. Sets:

| Macro                          | 2455 | 2550 | 4455 | 4550 |
|---------------------------------|------|------|------|------|
| `_FLASH_BYTES` / `_FLASH_INSTR`| 24576/12288 | 32768/16384 | 24576/12288 | 32768/16384 |
| `_RAM_BYTES`                    | 2048 | 2048 | 2048 | 2048 |
| `_EEPROM_B`                     | 256  | 256  | 256  | 256  |
| `_IO_PINS`                      | 24   | 24   | 35   | 35   |
| `_ADC_CH`                       | 10   | 10   | 13   | 13   |
| `_HAS_PORTD` / `_HAS_PORTE`     | 0    | 0    | 1    | 1    |
| `_HAS_SPP`                      | 0    | 0    | 1    | 1    |
| `_HAS_USB`                      | 1    | 1    | 1    | 1    |

`EPIC_FAMILY_RAM_BYTES` aliases `PIC18FXX5X_FAMILY_RAM_BYTES` under the
family-neutral name a consumer like the task manager scales against.

## 22. The examples

All under `tests/`. `example_blink` builds for both host and target (via
the harness) and is what the XC8 Makefile links; every other example is a
host-only smoke test that talks to `pic18_sim_*` directly and is not part
of the target build.

- **`example_blink`** — Timer0 (8-bit, 1:256) + GPIO + interrupt, `RB0`
  toggled 9 times in 600k cycles. The one example built for XC8.
- **`example_smoke`**, harness seam only (`epic_harness_init/tick/
  running/log/report`), proves an empty family backend links.
- **`example_timer1`** — internal clock, 1:1, 3 overflows at 65536 cycles
  each.
- **`example_timer2`** — PR2=249, 1:1/1:1, 5 overflows at 250 cycles each.
- **`example_timer3`** — internal clock, 1:8, 3 overflows at 524288 cycles
  each.
- **`example_ccp_pwm`** — ECCP1 half-bridge PWM + dead-band, verifies the
  CCP1CON/CCPR1L/ECCP1DEL register image and Timer2-period-count timing.
- **`example_ssp`** — SSPADD formula, SPI master init/write/read
  loopback, I²C master Start/Stop.
- **`example_usart`** — BRG math (all four Table 20-1 rows, including the
  16-bit-BRG range extension), init, TX/RX, auto-baud, address-detect.
- **`example_comp`** — mode/inversion/CIS register image, output readback,
  change flag.
- **`example_eeprom`** — read, write-unlock sequence, completion, buffer
  round-trip.
- **`example_adc`** — 3-register init image, start/done, both
  justifications, Vref bit placement, channel select, DeInit.
- **`example_spp`** — init register image (MCU- and USB-owned), byte
  write/read, status flags. 40/44-pin only.

Run all host examples with `cmake --build build && for t in
build/example_*; do "$t"; done` (exit 0 = pass), same as PIC16F87XA.

## 23. Known gaps and gotchas

- **No Vref driver.** This family has no on-chip voltage-reference module
  analogous to PIC16F87XA's Vref; don't expect one. `CVRCON` (comparator
  Vref) is a separate, not-yet-ported module.
- **MSSP I²C is register-level**, same as PIC16 — no blocking transfer
  state machine; drive Start/Stop/ACK and poll `BF`/`ACKSTAT` yourself
  ([§14](#14-mssp-spi-and-i²c)).
- **`IRQ_Enable` does not set the master enable(s).** Confirmed true here
  too (see `epic-common/MANUAL.md`'s general interrupt gotcha): arming a
  source via `EPIC_IRQ_Enable` is not enough, `EPIC_IRQ_Restore(1)` (which
  also ensures `IPEN=1`) is what actually lets interrupts fire.
- **No per-peripheral `HAL_PPP_MspInit`.** Confirmed true here too: ISR
  vector wiring is centralized in the one shared `epic_dispatch_all_irqs()`
  called from both the high- and low-priority vectors; extension is via
  handle callbacks, not per-peripheral weak vector hooks.
- **CCP2 silently ignores ECCP-only handle fields.** `PWMOutputMode`,
  `DeadBand`, `AutoShutdown` only apply to CCP1 (ECCP1); passing non-default
  values for CCP2 has no effect (no error returned).
- **`T3CCP2:T3CCP1` left at reset (Timer1).** To use Timer3 as the
  capture/compare time base for CCP1/CCP2, configure `T3CON` directly —
  the CCP driver does not expose this select.
- **EUSART RMW uses split read+write, not compound assignment**, because
  XC8 cannot lower a compound assignment on a volatile cast-lvalue at a
  runtime SFR address (a real XC8 v3.10 codegen limitation hit during
  Phase 4, not a stylistic choice).
- **Real-silicon validation deferred.** No PIC18 board on hand during
  development; every peripheral is verified on the host sim and the XC8
  build's vector placement (0008h/0018h), but not yet on real hardware —
  flagged explicitly in `docs/multi-family-plan.md`, not silently skipped.

## 24. Appendix: datasheet section index

Where each HAL module lives in DS39632E:

| HAL module                  | DS39632E section              |
|------------------------------|--------------------------------|
| Data EEPROM                  | §7.0 (Register 7-1)            |
| PORTA-E / LATA-E / TRISA-E   | §10.0                           |
| Timer0                       | §11.0 (Register 11-1, Table 11-1) |
| Timer1                       | §12.0 (Register 12-1)          |
| Timer2                       | §12.0 (Register 12-2/12-3)     |
| Timer3                       | §14.0 (Register 14-1)          |
| SPP                          | §18.0 (Registers 18-1..18-3)   |
| ECCP1 / CCP2                 | §16.0 (Registers 16-1..16-3)   |
| MSSP (SPI / I²C)             | §19.0 (Registers 19-1..19-6)   |
| EUSART                       | §20.0 (§20.1 BRG, Table 20-1)  |
| A/D Converter                | §21.0 (Registers 21-1..21-3)   |
| Comparator                   | §22.0 (Register 22-1, Figure 22-1) |
| Reset Control (RCON)         | §4.0 (Register 4-1)            |
| Interrupts                   | §9.0                           |
| Power-down / Sleep           | §3.0                            |
| Code protection (block 3)    | §23.1                          |
| Reset values                 | Table 5-1                       |

When a header or driver makes a claim you want to verify, the citation in
its doc comment names the exact section/register/table above.
