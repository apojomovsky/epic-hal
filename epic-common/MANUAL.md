# epic-common, Manual

The family-agnostic conventions, patterns, and shared contract every
`<family>-hal` in this repo builds on. This manual is for humans: it
explains the ideas that are true regardless of which chip's SFRs sit
underneath. It does **not** cover any single peripheral driver, register
name, or datasheet section, those are genuinely per-family and live in
that family's own manual (`pic16f87xa-hal/MANUAL.md`,
`pic18fxx5x-hal/MANUAL.md`, and whichever comes next). Read this once,
then read your family's manual for the register-level reference; the
family manual will point back here for anything conceptual instead of
re-explaining it.

This split exists because `docs/multi-family-plan.md` already applied the
same reasoning to *code*: extract what's architecture-blind into
`epic-common/` once, keep what's genuinely register-specific per family
under a fixed contract (same names/signatures, different bodies). This
document is the documentation half of that same split; see
`docs/hal-manual-plan.md` for how it was carved out of what used to be one
PIC16-only manual.

---

## Contents

1. [What a HAL buys you here](#1-what-a-hal-buys-you-here)
2. [The shared/per-family split](#2-the-sharedper-family-split)
3. [Conventions](#3-conventions)
4. [The harness](#4-the-harness)
5. [The host simulation backend, the shape of it](#5-the-host-simulation-backend-the-shape-of-it)
6. [Interrupts, the shared model](#6-interrupts-the-shared-model)
7. [Writing an ISR-driven peripheral](#7-writing-an-isr-driven-peripheral)
8. [The examples convention](#8-the-examples-convention)
9. [Retargeting and porting](#9-retargeting-and-porting)

---

## 1. What a HAL buys you here

Writing bare-register firmware for an 8-bit PIC is doable but tedious and
unportable: SFRs live in a banked (or BSR-relative) register file, timing
quirks are scattered across the datasheet, and application code ends up as
a pile of raw bit-twiddling. Every HAL in this repo absorbs that machinery
so application code reads like an STM32Cube application:

```c
EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
```

…rather than hand-rolled register bit twiddling. People who know
STM32Cube feel at home; people who know PICs recognise the peripherals and
finally get the abstraction they usually build themselves.

Every family HAL has two consumers built in:

- a **host simulation backend**, so you can compile and *run* firmware on
  a Linux/macOS box with gcc, drive it from a C test, and watch timers
  tick and pins toggle, no hardware required; and
- a **real-target build** with MPLAB XC8, which produces a `.hex` you
  program onto the actual chip.

The same source files serve both. Crucially, the split between the two is
done **at build time** (which files are compiled and which header is on
the include path), never with `#ifdef` inside application code. That is a
design rule of every HAL in this repo and the reason its examples read so
cleanly. See [§2](#2-the-sharedper-family-split) and [§4](#4-the-harness).

---

## 2. The shared/per-family split

```
epic-common/                     shared layer reused by every family
├── include/core/              hal_status.h (EPIC_*, PIC8_BIT*),
│                              epic_harness.h (the 4-fn harness contract),
│                              epic_irq.h (the shared EPIC_IRQ_Priority enum)
├── src/core/                  epic_harness_target.c (family-blind no-ops)
├── cmake/  mk/                shared epic_family.cmake / epic_family.mk
└── MANUAL.md                  this document

<family>-hal/                    e.g. pic16f87xa-hal/, pic18fxx5x-hal/
├── include/
│   ├── <family>.h               family header: device select, SFR mapping;
│   │                            pulls shared status codes / bit helpers from
│   │                            epic-common/hal_status.h, includes platform hdr
│   ├── <family>_sfr.h           SFR address map + bit names (1:1 with that
│   │                            family's datasheet)
│   ├── <family>_sim.h           simulation backend public API
│   ├── host/                    platform header selected by the host build
│   ├── target/                  platform header selected by the XC8 build
│   ├── core/                    CPU-level features specific to that family
│   │                            (IRQn enum + EPIC_IRQ_* backend, WDT/Sleep)
│   └── peripherals/             one .h per peripheral, Cube-style
├── src/
│   ├── core/                    implementations of core/ headers
│   ├── peripherals/             implementations of peripherals/ headers
│   └── sim/                     host simulation backend (host build only)
├── tests/                       end-to-end examples / smoke tests
├── mcu/<family>-mplabx/         XC8 Makefile + MPLAB X project
├── MANUAL.md                    the per-family register-level reference
└── CMakeLists.txt               host build (gcc + cmake)
```

There are three ideas to internalise, true across every family:

### 2.1 The SFR access layer is selected by include path

Every family header ends with one unconditional line pulling in a
platform header, which exists in **two same-named copies**:

- `include/host/<family>_platform.h`, every SFR access indexes an
  in-memory array standing in for the register file, and `PIC8_WEAK` is
  the real `__attribute__((weak))`.
- `include/target/<family>_platform.h`, every SFR access is a direct
  `*(volatile uint8_t *)(uintptr_t)addr` dereference, and `PIC8_WEAK` is
  empty (XC8 has no weak symbols).

The CMake host build puts `include/host` *first* on the include path; the
XC8 Makefile puts `include/target` first. So the single `#include`
resolves to the right copy per build, with no preprocessor branching
anywhere in the HAL or examples. The macros defined there (`PIC8_REG8`,
`epic_sfr_read8/write8`, `PIC8_SFR_PTR`, or that family's equivalent
names, see your family's manual) are what every driver and every example
ultimately touches.

### 2.2 Execution-model differences are selected by linking a different file

A few things genuinely differ between "a host test" and "firmware":

- on the host, time advances only when you pump the simulator; on a
  target, the hardware clock advances on its own;
- a host test must terminate and report pass/fail; firmware's `main()`
  never returns and has no stdout;
- WDT/Sleep helpers are real instructions (`asm("clrwdt")`/`asm("sleep")`
  or that family's equivalent) on a target and no-ops on the host;
- the interrupt vector(s) only exist on a target.

Each of those is provided by **two implementation files**, one the CMake
build links (`*_sim.c`) and one the XC8 Makefile links (`*_target.c`).
The harness target implementation is family-blind (four no-ops), so it
lives in the shared `epic-common/` layer (`epic_harness_target.c`); the
harness sim side and the WDT/Sleep pair stay in each family's own tree.
The build picks which file to link, no `#ifdef` in application code
selects between them. The [harness](#4-the-harness) is the seam that
makes the examples one source for both builds.

### 2.3 One shared dispatcher pattern, family-specific vector count

Every family centralises interrupt dispatch behind a shared function that
fans out to every peripheral's own weak `*IRQHandler`, so applications
never wire interrupt vectors themselves. *How many* vectors and
dispatchers exist, and whether there's a priority model, is genuinely
per-family (PIC16F87XA has one vector, no priority; PIC18F2455/2550/4455/
4550 has two, high and low, with per-source priority bits) — see
[§6](#6-interrupts-the-shared-model) for what's shared about the
interrupt story and your family's manual for the vector-count specifics.

---

## 3. Conventions

### 3.1 Naming

Mirrors STM32Cube, across every family:

- `EPIC_<PPP>_<Verb>(...)` for driver functions
  (`EPIC_GPIO_Init`, `EPIC_TIMER0_Start`, `EPIC_ADC_Read`).
- `<PPP>_HandleTypeDef` for the configuration struct
  (`TIMER0_HandleTypeDef`, `ADC_HandleTypeDef`).
- `<PPP>_<Feature>TypeDef` for enums
  (`TIMER0_PrescalerTypeDef`, `ADC_ChannelTypeDef`).
- `<PPP>_HANDLE_DEFAULT` for a default initialiser you can override field
  by field (see [§3.3](#33-the-handle-pattern) for the one common
  exception, CCP/peripherals with no sane default).
- `<FAMILY>_<NAME>` for HAL-wide types and macros
  (`EPIC_StatusTypeDef`, `PIC8_REG8`).
- `PIC_REG_<NAME>` and `PIC_<REG>_<BIT>` for raw SFR addresses and bit
  masks (see your family's manual, "The SFR layer").

### 3.2 Status codes

```c
typedef enum {
    EPIC_OK      = 0x00,   // success
    EPIC_ERROR   = 0x01,   // generic error
    EPIC_BUSY    = 0x02,   // resource busy
    EPIC_TIMEOUT = 0x03,   // operation timed out
    EPIC_INVALID = 0x04,   // bad parameter / state
} EPIC_StatusTypeDef;
```

Defined once in `epic-common/include/core/hal_status.h`, shared by every
family. `Init`/`Start`/`Stop`/`DeInit` return this. The most common check
is `if (HAL_PPP_Init(&h) != EPIC_OK) { ... }`. A few helpers return data
directly with a sentinel instead (e.g. an ADC start that returns a
reserved value if a conversion was already in progress, or an SPI write
that returns a reserved value on a write collision — check your family's
manual for the specific peripheral).

### 3.3 The handle pattern

Most peripherals are configured through a `*_HandleTypeDef` filled by the
caller and passed to `HAL_PPP_Init`:

```c
TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;   // start from sane defaults
h.Prescaler         = TIMER0_PRESCALER_1_64;
h.OverflowCallback  = on_t0_overflow;
EPIC_TIMER0_Init(&h);
EPIC_TIMER0_Start(&h);
```

`Init` programs the SFRs but does **not** start the peripheral, call
`Start` afterwards. `DeInit` resets the peripheral to its power-on state.

A few peripherals take a raw callback pointer instead of a handle, because
their configuration is essentially just "do you want an interrupt or
not" (EEPROM and a parallel port peripheral are the recurring examples):

```c
EPIC_EEPROM_Init(my_eeprom_callback);
```

And at least one peripheral per family tends to have **no**
`_HANDLE_DEFAULT` at all, when every field is load-bearing and a "sane
default" would be misleading (CCP/ECCP, where Instance and Mode must
always be filled explicitly) — check your family's manual for which one.

### 3.4 Interrupts and callbacks

Each interrupt-capable peripheral declares a weak `PPP_IRQHandler`. The
HAL's default implementation of each handler checks the peripheral's
flag, clears it, and, if the handle provided a callback, calls it. So the
normal extension point is **the handle callback**, not the handler:

```c
static void on_t0_overflow(void) { /* runs in interrupt context */ }
h.OverflowCallback = on_t0_overflow;
```

On the host build the handlers are genuinely `__attribute__((weak))`, so
a user can also override the whole `TIMER0_IRQHandler` if needed. On XC8
there is no weak symbol, so the callback is the only extension point
(redefining the handler would be a duplicate symbol).

See [§6](#6-interrupts-the-shared-model) for the full interrupt model.

### 3.5 Datasheet citations

Every header and implementation file in every family HAL annotates the
datasheet section it implements. When in doubt about a register bit, grep
the header for the section number. If you find a deviation from the cited
section, that is a bug, treat "the HAL disagrees with the datasheet" as a
defect regardless of which family it's in.

---

## 4. The harness

The harness (`epic_harness.h`, in `epic-common/`, included as
`core/epic_harness.h`) is the seam that lets a single example source
build for *both* the host sim and a real XC8 target with **no `#ifdef` in
the example code**. It hides the two execution-model differences from
[§2.2](#22-execution-model-differences-are-selected-by-linking-a-different-file),
pumping simulated time vs. real time, and a terminating test vs. firmware
that runs forever, behind four functions plus a shared inline:

```c
void epic_harness_init(uint32_t cycles);   // host: reset sim + wire IRQ dispatch; target: no-op
void epic_harness_tick(void);              // host: sim_step(1);                target: no-op
int  epic_harness_running(uint32_t it);    // host: it < cycles;                target: always 1
void epic_harness_log(const char *fmt, ...); // host: printf;                 target: no-op
static inline int epic_harness_report(int ok) { return ok ? 0 : 1; }  // shared: map to exit code
```

Each has a host implementation (`*_harness_sim.c`, in the family tree,
linked by CMake) and a target implementation
(`epic_harness_target.c`, family-blind, in `epic-common/`, linked by
every XC8 Makefile). The build picks one.

### 4.1 Real-target `log()` implementations

A real-target `epic_harness_log()` may dispatch on its format string.
The family-blind `epic_harness_target.c` is a no-op; the PIC16F87XA
and PIC18Fxxxx sim-target harnesses write bytes to the UART; the
PIC16F193X sim-target harness drives RA0 (PORTA bit 0) from the
exact `PIC8_HARNESS_RESULT: PASS\n` / `...: FAIL\n` marker passed
by `epic_harness_report()` so the wrapper can read the result via
`print PORTA` in `MODE=gpio`. Future sim-target harnesses may define
their own dispatch.

The format-string dispatch is opt-in per file: not every
implementation does it. The marker strings are passed verbatim from
`epic_harness_report()` (a static inline in `epic_harness.h`). No
other call site in the codebase passes these literals, so the
dispatch never fires by accident.

### 4.2 The canonical example shape

```c
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* … peripheral setup, identical on both builds … */

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        /* … work, same code on both builds … */
    }
    epic_harness_log("toggled %u\n", (unsigned)count);
    return epic_harness_report(count >= 2);
}
```

On the **target**, `harness_running` always returns 1, so the loop never
exits and the log/report lines are unreachable; `harness_tick` is empty
because real time advances on its own. On the **host**, the loop runs for
`SIM_CYCLES` simulated cycles, `harness_tick` pumps the simulator each
iteration, and the log/report produce a pass/fail exit code.

`EPIC_Sleep_Enter` and `EPIC_WDT_Refresh` (or that family's equivalents) are
no-ops on the host, so an example can call them unconditionally, they are
real `sleep`/`clrwdt` instructions on the target and vanish on the host.
That is what lets an idle-blink-style example, that genuinely sleeps, be
one source.

When to reach for the harness: whenever you want an example that is both a
host smoke test *and* real firmware. For a pure host test (peripheral
fiddling that needs hardware you do not have), talk to the simulator
directly, see [§5](#5-the-host-simulation-backend-the-shape-of-it).

---

## 5. The host simulation backend, the shape of it

Each family's simulator (`src/sim/<family>_sim.c`, public API in
`<family>_sim.h`) is a memory array standing in for that chip's register
file, plus behavioural models for the peripherals that actually *do*
something over time. It lets you write and run firmware as an ordinary C
program on a host. *What* it models, exact peripheral behavior, timing
approximations, which registers are backed, is genuinely per-family and
documented in that family's own manual. The *shape* of the public API is
shared across families:

```c
void     <family>_sim_reset(void);                                  // zero + load POR values
void     <family>_sim_step(uint32_t ticks);                         // advance time

void     <family>_sim_drive_input(char port, uint8_t pin, uint8_t level);  // drive an input pin
uint8_t  <family>_sim_read_output(char port, uint8_t pin);                  // observe an output pin
void     <family>_sim_set_irq_callback(...);                               // (advanced)

void     <family>_sim_drive_<peripheral>_*(...);   // one family per peripheral that needs
                                                    // the test to "act as the outside world"
                                                    // (feed a serial byte, deliver an ADC
                                                    // result, seed EEPROM, …)
```

### 5.1 How interrupts dispatch on the host

You usually do **not** call `<family>_sim_set_irq_callback` yourself. The
harness registers the family's shared dispatch function as the sim
callback during `epic_harness_init`, mirroring the real target's
vector(s). When `sim_step` produces an overflow or completion, it calls
that dispatcher, which fans out to every peripheral handler; the one whose
flag is set runs and calls your callback. That is the only thing that
makes interrupt-driven examples work on the host.

### 5.2 Writing a sim-only test

Sim-only smoke tests talk to the simulator directly rather than through
the harness:

```c
<family>_sim_reset();
<family>_sim_set_irq_callback(SOME_PERIPHERAL_IRQHandler);  // route the flag → driver → callback
/* … configure + start the peripheral … */
for (uint32_t i = 0; i < SIM_BUDGET; i++) {
    <family>_sim_step(1);
    if (some_condition) break;
}
printf("OK: ...\n");
```

These tests are pure host programs, they `printf` and `return` an exit
code, and they never build for a real target (the XC8 Makefile only links
the firmware-capable examples). They have no `#ifdef` because they have
no target path.

---

## 6. Interrupts, the shared model

Every family centralises interrupt dispatch: a shared function fans out
to every peripheral `*IRQHandler`, and each of those is a no-op unless its
own flag is set, so the order is irrelevant. Applications never touch a
vector themselves.

What's shared across every family:

```c
uint8_t EPIC_IRQ_Disable(void);                 // clears the global enable, returns previous state
void    EPIC_IRQ_Restore(uint8_t prev_state);   // restore the global enable to prev_state
void    EPIC_IRQ_Enable(<FAMILY>_IRQn irq);     // set the source's enable bit
void    EPIC_IRQ_DisableSrc(<FAMILY>_IRQn irq);
void    EPIC_IRQ_ClearFlag(<FAMILY>_IRQn irq);
uint8_t EPIC_IRQ_GetFlag(<FAMILY>_IRQn irq);
```

`EPIC_IRQ_Disable`/`EPIC_IRQ_Restore` form a critical-section pair: disable
around a sensitive sequence, restore the previous state afterwards.

**Important gotcha, true on every family:** `EPIC_IRQ_Enable` arms the
*source* only, it does **not** set the chip's global interrupt enable. To
actually take interrupts you must also enable interrupts globally
(`EPIC_IRQ_Restore(1)` is the idiomatic way, since it both sets the global
enable and reads naturally as "interrupts on").

**Priority is shared vocabulary, family-specific effect.** The priority
enum lives in `epic-common/include/core/epic_irq.h`:

```c
typedef enum {
    EPIC_IRQ_PRIORITY_LOW  = 0,
    EPIC_IRQ_PRIORITY_HIGH = 1
} EPIC_IRQ_Priority;
```

`EPIC_IRQ_SetPriority(irq, prio)` is declared per-family (its `irq`
parameter is that family's own `*_IRQn` type) because the set of
interrupt sources differs per family, but the two-value vocabulary is
shared so every family spells "high" vs "low" the same way. On a
single-vector family this is a no-op; on a family with separate high/low
vectors it routes the source to the matching one. See your family's
manual for its `*_IRQn` enum and whatever it actually does with priority.

**No `HAL_PPP_MspInit`.** Unlike STM32Cube, ISR-vector wiring is never a
per-peripheral weak callback in any family here, the shared dispatcher(s)
handle it, and the extension point is always the handle callback (see
[§3.4](#34-interrupts-and-callbacks)), never the vector or the dispatcher
itself.

---

## 7. Writing an ISR-driven peripheral

The same recipe, on every family:

1. Fill a handle with an `OverflowCallback` / `ConvCpltCallback` / etc.
2. `HAL_PPP_Init(&h)`, this enables the source's interrupt bit.
3. `HAL_PPP_Start(&h)`.
4. `EPIC_IRQ_Restore(1)` (or your own global-enable call) to actually take
   interrupts.
5. In the callback (which runs in interrupt context): do the work. The
   HAL's handler has already cleared the flag for you, **do not clear it
   again** unless you know why, re-enabling with the flag still set can
   cause infinite re-entry (check your family's datasheet citation on the
   interrupt chapter for the specific warning).

The default weak handler clears the flag *and* calls the callback, so a
typical callback just does the application work:

```c
static void on_t0_overflow(void) {
    g_ticks++;
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}
```

---

## 8. The examples convention

Every family's examples live under `tests/`. By convention, the first one
or two build for **both** host and target (via the harness, see
[§4](#4-the-harness)), the canonical proof that "the HAL drives a real
application" on real silicon as well as the simulator; the rest are
host-only smoke tests that talk to the simulator directly
([§5.2](#52-writing-a-sim-only-test)), one per peripheral, exercising the
sim's `drive_*` helpers. Run all of a family's host examples with:

```sh
cmake --build build
for t in build/example_*; do "$t"; done   # exit code 0 = pass
```

See your family's manual for the actual example list and what each one
demonstrates.

---

## 9. Retargeting and porting

- **To another XC8 version.** Toolchain intermediate format, link flags,
  and device-family-pack requirements have changed across XC8 major
  versions, check your family's Makefile notes for the pinned version and
  what changes for others.
- **To another host compiler.** The host build is plain C99. The only
  non-portable pieces are `__attribute__((weak))` (in each family's
  `include/host/<family>_platform.h`) and the `static inline` report in
  the harness. Any C99 compiler should do.
- **To another chip family entirely.** The SFR map, the platform header,
  and the peripheral drivers need rework against that family's datasheet,
  but the *structure*, the handle pattern, the harness, the link-time
  host/target split, the shared-dispatcher interrupt model, is reusable
  as-is. See `docs/adding-a-device.md` for the concrete, verification-gated
  checklist (Path B: new family), and write that family's own `MANUAL.md`
  against the shape of this repo's existing ones as part of it.
