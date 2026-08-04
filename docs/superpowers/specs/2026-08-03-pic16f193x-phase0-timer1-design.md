# Phase 0 + Timer1 for pic16f193x-hal

Design document for the next unit of work on `pic16f193x-hal`. The
sister plan document, `docs/pic16f193x-plan.md`, frames the whole
family addition; this spec is the concrete, decomposed scope for the
first executable piece.

## Scope

Two parts, in dependency order:

1. **Phase 0: HARNESS=sim GPIO reporting for PIC16F193X.** Today the
   `make mdb-test` wrapper (`scripts/sim-mdb-run.sh`) reports PASS/FAIL
   over the **EUSART** and the existing `HARNESS=sim` builds (PIC16F87XA,
   PIC18Fxxxx) write a marker line over UART. PIC16F193X has no EUSART
   driver yet, so the convenience wrapper does not apply. Phase 0
   generalizes the wrapper to support a **GPIO register** reporting mode
   alongside the existing UART mode, and adds the matching
   `HARNESS=sim` target implementation for PIC16F193X that drives a
   single pin (RA0, bit 0) instead of UART, by reusing
   `pic8_harness_log()`'s format-string dispatch (see "Harness hook:
   magic-string in `log()`" below).

2. **Timer1 peripheral.** A `HAL_TIMER1_*` driver for the PIC16F193X,
   `HARNESS=target` real-target example + host-sim example, and the
   full §4 verification gate (`docs/adding-a-device.md`) run via the
   Phase-0 `MODE=gpio` wrapper.

Phase 0 is the prerequisite for Timer1's §4 gate. Splitting it as its
own phase means Phase 0 lands as one self-contained commit (the
shared-CI change, with PIC16F87XA + PIC18Fxxxx regressions to prove no
breakage), and Timer1 lands as a follow-up commit. The plan's §7
roadmap (Timer2/4/6, CCP/ECCP1, EUSART, MSSP, ADC, Comparator, EEPROM,
DAC, FVR, SR latch, capacitive sensing, LCD driver, CCP3/4/5) is
explicitly **not** in this spec; each becomes its own design+plan+impl
cycle.

## Why now

The plan (Status line, §6, §7) names Timer1 as the natural first
peripheral. The only blocker is the HARNESS=sim reporting channel ,
the plan's §4 flags it as an "open design decision for the next unit
of work," which is exactly what Phase 0 resolves. After this spec the
plan's §7 gates become one shot per peripheral instead of one wrapper
rewrite per peripheral.

## Non-goals

- No Timer2/4/6, no CCP/ECCP, no EUSART, no MSSP, no ADC, no other
  peripheral. Each is its own follow-up spec.
- No Timer1 gate (`T1GCON`, DS41364B §16.6). The Timer1 driver covers
  `T1CON` only; gate is the next spec's Timer1.1 work if/when asked.
- No migration of PIC16F87XA or PIC18Fxxxx off UART. Those families
  keep their `pic16_harness_sim_target.c`/`pic18_harness_sim_target.c`
  UART path. The hook mechanism (`log()` magic-string dispatch) is
  opt-in per file; nothing in `pic8-common/` or the other families'
  sim-target harnesses changes.
- No CI workflow changes. `.github/workflows/host-tests.yml`,
  `xc8-build.yml`, `sim-tests.yml` are not touched in this phase.
- No pushing to `ghcr.io`. Per the plan and the root `CLAUDE.md` git
  safety rules, no push without explicit fresh approval.

---

## Phase 0: HARNESS=sim GPIO reporting

### Files added

- `pic16f193x-hal/src/core/pic16f193x_harness_sim_target.c`: target
  implementation of `pic8_harness_*` for `HARNESS=sim` builds.
  Mirrors `pic16_harness_sim_target.c`'s shape but without USART
  init. `log()` is not a pure no-op: when `fmt` is the pass/fail
  marker string (`"PIC8_HARNESS_RESULT: PASS\n"` or
  `"PIC8_HARNESS_RESULT: FAIL\n"`), it drives RA0 (PORTA bit 0) and
  ignores variadic args; for any other `fmt`, it is a no-op. See
  "Harness hook: magic-string in `log()`" below for the rationale.
- **No changes to `pic8-common/`.** The family-blind no-op
  `pic8_harness_target.c` (and its sibling two PIC16F87XA and PIC18
  UART-mode sim-target harnesses) keep working unchanged. No new
  function in `pic8_harness.h`, no new file in `pic8-common/src/`.

### Files modified

- `scripts/sim-mdb-run.sh`: add a positional `mode` arg (`uart` or
  `gpio`, default `uart`), **appended after the existing args**
  (wait_ms stays at `${6}`, mode becomes `${7}`, `extra_mdb`
  shifts from `${7}` to `${8}`). All existing callers pass six args
  or fewer and use the default `uart` mode, so they don't need to
  change. In `gpio` mode, skip UART setup; after `halt`, inject
  `print PORTA` and check bit 0 (1 = PASS, 0 = FAIL).
- Root `Makefile` `mdb-test` target: add optional `MODE` variable,
  forward to the script as the new positional arg, default `uart`.
- `pic16f193x-hal/mcu/pic16f193x-mplabx/Makefile`: add `HARNESS=sim`
  conditional selecting
  `$(HAL_DIR)/src/core/pic16f193x_harness_sim_target.c` and setting
  `CONFIG_WDTE := OFF` (mirroring PIC16F87XA's sim-target Makefile
  pattern: real-target firmware needs the watchdog; a diagnostic
  build that terminates and reports does not).

### Harness hook: magic-string in `log()`

The existing `pic8_harness_report()` static inline (in
`pic8_harness.h`) emits its PASS/FAIL marker line by calling
`pic8_harness_log(ok ? "PIC8_HARNESS_RESULT: PASS\n" :
"PIC8_HARNESS_RESULT: FAIL\n")`. The UART-mode `log()` writes those
bytes to the USART. The new PIC16F193X sim-target `log()` instead
*inspects the format string*: when it is exactly the pass or fail
marker, drive RA0 (PORTA bit 0) from the message's meaning. Call
`HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, ok ? GPIO_PIN_SET :
GPIO_PIN_RESET)`, then ignore variadic args; for any other `fmt`, it
is a no-op.

This works because:

- `pic8_harness_report()` is the only call site in the codebase that
  passes one of these exact strings (verified by grep over
  `pic8-common/`, every family HAL, and every `pic8-*` module under
  `tests/` and `examples/`).
- `log()` on a real-target no-stdout build is already an
  implementation detail with no observable contract beyond the
  marker line; making it inspect that one string is a small step, not
  a category change.
- No new public API surface, no shared-header change, no edit to
  `pic8_harness.h`, no new file in `pic8-common/`. The hook lives
  entirely inside the one new file.

Trade-off considered and rejected: a separate weak
`pic8_harness_signal_result()` called from `report()`. That's cleaner
in concept but introduces a new function for one call site and
touches every family for no behavior change. The magic-string-inspect
path is strictly less work and strictly less code; it lands in one
file and stops.

### GPIO pin choice: RA0

All six PIC16F193X parts have PORTA. RA0 is `ANSELA<0>` = 1 by default
(analog), so the harness init must clear `ANSELA<0>` and set
`TRISA<0> = 0` to make it a digital output. LATA starts at 0 (POR
value). RB0 is already used by `example_blink` and is the IOC pin on
this core (DS41364B §7.0), so driving it from the harness could
spuriously trigger IOC interrupts if IOC were ever enabled. RC0 is
free but the established convention across all three PIC16F87XA
examples is RA0. Choosing RA0 keeps the pin portable and avoids the
IOC hazard.

### Mdb mechanics for `print PORTA`

`mdb` accepts `print <REGISTER>` for SFRs and prints the hex byte,
e.g. `0x01`. The wrapper parses one line, checks that it matches the
expected `0x0N`/`0x1N` form, masks bit 0, and reports PASS/FAIL
accordingly. If `print` returns empty or non-hex, the wrapper fails
the run with a clear error message , same shape as the existing
"no UART output" failure.

### Phase 0 verification (regression + new path)

Run inside the toolchain container (`make shell` or equivalent), all
three must PASS:

1. **Regression: PIC16F87XA `HARNESS=sim` UART path.**
   ```
   make mdb-test \
     MODULE=pic8-tick/mcu/pic16f87xa-tick-mplabx \
     MCU=16F877A DEVICE=PIC16F877A \
     DFP=Microchip.PIC16Fxxx_DFP
   ```
   Expect PASS, identical to pre-change behavior.

2. **Regression: PIC18Fxxxx `HARNESS=sim` UART path.**
   ```
   make mdb-test \
     MODULE=pic8-tick/mcu/pic18fxx5x-tick-mplabx \
     MCU=18F4550 DEVICE=PIC18F4550 \
     DFP=Microchip.PIC18Fxxxx_DFP
   ```
   Expect PASS.

3. **New: PIC16F193X `HARNESS=sim` GPIO path on a smoke.** Build the
   existing `example_blink` (Timer0 + GPIO) with `HARNESS=sim` for
   PIC16F1937, then:
   ```
   make mdb-test \
     MODULE=pic16f193x-hal/mcu/pic16f193x-mplabx \
     MCU=16F1937 DEVICE=PIC16F1937 \
     DFP=Microchip.PIC12-16F1xxx_DFP \
     MODE=gpio
   ```
   Expect PASS. The wrapper's captured output should show the GPIO
   pin toggled. The example's ISR drives LATB<0> on every Timer0
   overflow; the harness init leaves RA0 low; on `pic8_harness_log()`
   with the pass/fail marker, the harness writes LATA<0> from `ok`.
   The deterministic read is whatever LATA<0> was last driven to by
   the harness (RA0, bit 0). Both are observable via `print LATA`
   and `print LATB` if needed, but the wrapper only needs the PASS
   bit on PORTA.

### Phase 0 exit criteria

All three verification runs PASS. PIC16F193X `HARNESS=sim` produces a
.hex that runs end-to-end under `mdb` and is observable via
`print PORTA`. No change to PIC16F87XA or PIC18Fxxxx CI.

---

## Timer1 peripheral

### Files added

- `pic16f193x-hal/include/peripherals/pic16f193x_timer1.h`: public
  API. Types: `TIMER1_ClockSourceTypeDef` (INTERNAL = `TMR1CS=0`,
  EXTERNAL = `TMR1CS=1`), `TIMER1_PrescalerTypeDef` (1:1, 1:2, 1:4,
  1:8), `TIMER1_HandleTypeDef` (ClockSource, Prescaler, ReloadValue,
  OverflowCallback). API: `HAL_TIMER1_Init/DeInit/Start/Stop/
  ReadCounter/WriteCounter/PrescalerToRatio`. Weak `TIMER1_IRQHandler`.
  `TIMER1_HANDLE_DEFAULT` macro. **No T1OSC field** (T1GCON replaces
  it on this core and is the next spec's Timer1.1 scope).
- `pic16f193x-hal/include/peripherals/hal_timer1.h`: neutral shim.
  Per the existing `hal_timer0.h` convention: empty `#include
  "peripherals/pic16f193x_timer1.h"`.
- `pic16f193x-hal/src/peripherals/pic16f193x_timer1.c`: driver
  implementation. See "Timer1 driver shape" below.
- `pic16f193x-hal/tests/example_timer1.c`: host-testable example
  (also linked into the real-target XC8 build). Computes expected
  register image and toggle count, documented in the file header in
  the same style as `tests/example_blink.c`.

### Files modified

- `pic16f193x-hal/src/sim/pic16f193x_sim.c`: add `sim_step_timer1`
  mirroring `sim_step_timer0`'s shape. Wire into the per-step loop.
- `pic16f193x-hal/CMakeLists.txt`: add `pic16f193x_timer1.c` to
  `PIC8_HAL_SOURCES`; add
  `pic8_add_example(example_timer1 tests/example_timer1.c)`.
- `pic16f193x-hal/mcu/pic16f193x-mplabx/Makefile`: add the new
  source to `HAL_SOURCES` (both `HARNESS=target` and `HARNESS=sim`
  branches link it).
- `pic16f193x-hal/MANUAL.md`: add Timer1 register reference,
  citing DS41364B §16.0.
- `pic16f193x-hal/docs/ARCHITECTURE.md`: append any codegen finding
  surfaced by the §4 gate (e.g. high/low byte write order, BSR-bank
  surprises for PIE1 reads). One `## Finding N` per real finding,
  none if none surface.

### Timer1 driver shape

Mirrors `pic16f87xa_timer1.c` per peripheral:

- Atomic 16-bit counter read with the `hi1/lo/hi2` retry loop (DS41364B
  §16.4.1 explicitly warns about TMR1H:TMR1L consistency issues).
- Write-counter: high byte first (DS41364B §16.8, same idiom as
  PIC16F87XA, to be confirmed against the datasheet section while
  implementing, and recorded in `MANUAL.md`).
- `Init`: stop TMR1ON, configure `PIE1<TMR1IE>` based on callback
  presence (clear TMR1IF first), save handle pointer.
- `DeInit`: clear TMR1IE, clear TMR1IF, restore T1CON POR value,
  clear TMR1H/L.
- `Start`: write `ReloadValue`, program T1CON as one literal write
  with TMR1ON set last (T1CKPS, T1OSCEN, T1SYNC, TMR1CS, TMR1ON).
- `Stop`: clear TMR1ON only.
- `TIMER1_IRQHandler`: if TMR1IF is set, clear it, call the
  registered callback.

### PIC16F193X-specific notes

- All SFR accesses are literal `PIC_REG_*` tokens (T1CON = 0x18, TMR1L
  = 0x16, TMR1H = 0x17, PIR1 = 0x11, PIE1 = 0x91). XC8 auto-banks on
  this core (plan §6 codegen probe clean). No runtime dispatch.
- The BSR-dispatch pattern is not used here; the entire driver is
  compile-time-constant. This is the simplest case the §4 gate can
  validate and the reason Timer1 is the first peripheral.
- T1CON register bit layout (DS41364B Register 16-1) differs from
  PIC16F87XA's T1CON (DS39582B Register 6-1) in details: every bit
  mask must be transcribed from the 193X datasheet, not copied from
  the 87XA header. The header file's `#define` lines will cite
  DS41364B §16.0 explicitly.
- TMR1IF/PIE1 use `PIC_PIR1_TMR1IF`/`PIC_PIE1_TMR1IE` (already in
  `pic16f193x_sfr.h` from the foundation).

### Host sim Timer1 model

`sim_step_timer1()` in `pic16f193x_sim.c`:

- Read T1CON; bail if `TMR1ON = 0`.
- If `TMR1CS = 1` (external clock): not modeled, return. Document
  in a comment that this is the same limitation `pic16f87xa_sim.c`
  has.
- Compute prescaler ratio from T1CKPS<1:0> (1, 2, 4, 8).
- Increment an internal prescaler counter; when it hits `ratio`,
  reset to 0 and increment TMR1H:TMR1L as a single 16-bit value.
- On wrap from 0xFFFF to 0x0000: set `PIR1<TMR1IF>` and fire the sim
  IRQ callback (which runs `pic8_dispatch_all_irqs`, which routes
  TMR1IF to `TIMER1_IRQHandler` via the IRQ descriptor table).

### Example: `tests/example_timer1.c`

Shape mirrors `example_blink.c`:

- `pic8_harness_init(SIM_CYCLES)` with a `SIM_CYCLES` chosen so 1:8
  prescaler + FOSC/4 = 8 MHz at 32 MHz FOSC gives 3-9 Timer1 overflows
  in the run. (256 ticks prescale-counter isn't right for Timer1; use
  the same ratio-from-OPTION_REG derivation pattern, but the math
  here is straightforward: 16-bit counter + 1:8 prescaler = 524288
  cycles/overflow. So `SIM_CYCLES = 2_000_000` gives ~3-4 overflows.)
- Configure RB0 (or RA0, but RA0 is reserved for the harness) as
  digital output, drive low.
- Timer1 handle with `TIMER1_CLOCK_INTERNAL`, `TIMER1_PRESCALER_1_8`,
  reload 0, OverflowCallback that toggles RB0.
- HAL_TIMER1_Init + HAL_TIMER1_Start.
- HAL_IRQ_Restore(1) to arm the global interrupt.
- Main loop pumps sim/ticks.
- Pass condition: toggle count >= 3.

Expected register image (after init, before main loop runs):
T1CON = `(T1CKPS=11, 1:8) | (T1OSCEN=0) | (T1SYNC=0) | (TMR1CS=0) | (TMR1ON=0)`
= `0x30` (T1CKPS<5:4>=11, others 0, before `HAL_TIMER1_Start` writes the
`TMR1ON=1` final value). PIE1 = `0x01` (TMR1IE bit 0). INTCON = `0xC0`
(GIE=1, PEIE=1; TMR0IE=0 because `example_timer1.c` does not init
Timer0; only `example_blink.c` does). These specific values are
documented in the example's header comment.

### Timer1 §4 gate verification

Run inside the toolchain container, all four must PASS:

1. **Host sim.** `cmake -B build && cmake --build build && ./build/
   example_timer1`. Exit 0; expected toggle count. Run per-device
   loop via `pic8_add_example_per_device(example_timer1 ...)`.

2. **Real-target XC8 build for every part.** `make -C
   pic16f193x-hal/mcu/pic16f193x-mplabx clean MCU=16F1937 HARNESS=
   target`, then `MCU=16F1933`, `16F1934`, `16F1936`, `16F1938`,
   `16F1939`. All six build clean, produce a `.hex`.

3. **`mdb` register readback via the Phase-0 wrapper.** `make mdb-test
   MODULE=pic16f193x-hal/mcu/pic16f193x-mplabx MCU=16F1937
   DEVICE=PIC16F1937 DFP=Microchip.PIC12-16F1xxx_DFP MODE=gpio`.
   Expect PASS.

4. **Manual `mdb.sh` `stepi <N>` + `print <REG>` for every SFR
   Timer1 touched.** The exact register image from the example
   header, byte for byte, vs the values read back. Registers to
   read: T1CON, TMR1H, TMR1L, PIR1 (TMR1IF after overflow), PIE1
   (TMR1IE), INTCON (GIE/PEIE), LATB (RB0 driven by Timer1 ISR),
   LATA (RA0 driven by the harness's `log()`-marker path on
   `pic8_harness_report(ok)`).

The §4 control-register check: if any value above is wrong, first
check a known-good control register (an unrelated plain
compile-time-constant SFR access elsewhere in the same run, e.g.
TRISB after `HAL_GPIO_Init`); if that reads correctly and the
Timer1 value doesn't, that's a real bug, not a timing issue. The
established pattern from PIC16F87XA and PIC18 debug sessions.

### Timer1 exit criteria

All four steps above pass for the default part (`PIC16F1937`). If a
bug surfaces, fix in the driver / sim / harness layer as
appropriate, re-run the gate, document the finding in
`docs/ARCHITECTURE.md`.

---

## Cross-cutting

### Documentation updates

- `pic16f193x-hal/README.md`: update the HARNESS=sim line to say
  "now works via RA0 GPIO mode (MODE=gpio)" instead of "no EUSART
  driver yet."
- `pic16f193x-hal/MANUAL.md`: add Timer1 register reference.
- `pic16f193x-hal/docs/ARCHITECTURE.md`: append any codegen findings.
- `pic8-common/MANUAL.md`: note in the harness section that
  real-target `pic8_harness_log()` implementations may dispatch on
  the format string (the existing UART-mode ones write bytes; the
  PIC16F193X GPIO-mode one drives RA0 from the marker). No code or
  API contract changes; just documenting the convention so future
  sim-target harness writers know the magic-string-inspect hook is
  available if they need it.
- `docs/adding-a-device.md`: add a one-line note that
  `scripts/sim-mdb-run.sh` now supports `MODE=gpio` for families
  without an EUSART driver yet.

### Commit shape

Per `AGENTS.md`: commit whenever a piece of work is finished,
Conventional Commits, scope = the module or `phase0`. **Two commits
expected for this spec**:

1. `feat(pic16f193x,phase0): HARNESS=sim GPIO reporting via RA0 +
   MODE=gpio wrapper generalization`, the wrapper generalization,
   the new PIC16F193X sim-target harness, the Makefile change, and
   the regression tests for PIC16F87XA + PIC18Fxxxx. **Verify the
   PIC16F87XA + PIC18Fxxxx UART regressions pass before committing
   (this is the §4 control-register check for the wrapper itself).**
2. `feat(pic16f193x): Timer1 peripheral through §4 gate`, the
   Timer1 driver, sim, example, MANUAL entry, and `make mdb-test`
   verification on PIC16F1937. Depends on commit 1.

Push remains gated per `feedback_git_push_approval.md`: every push
needs fresh explicit approval, no standing "push when ready"
permission. No commit is held back beyond the work being finished
(§4 gate passes, host sim green, six-part real-target build green,
`make mdb-test` PASS); §4 verification output lives in the commit
itself (commit message + diff) and is reviewed at PR time.

---

## Open questions / future work

These are explicitly out of scope for this spec; recording them so
the next spec can pick them up:

- Timer2/4/6 (DS41364B §17.0), different prescaler/postscaler model,
  PR2/PR4/PR6 register interaction, separate TMR2IF/TMR4IF/TMR6IF
  sources in PIR1/PIR3.
- CCP/ECCP1 (DS41364B §19.0), 5 CCP modules, ECCP1 has enhanced
  PWM features, CCP4/CCP5 are standard.
- EUSART (DS41364B §20.0), baud-rate math is the §4 high-risk
  pattern from the appendix (overflow on SPBRG). Once this lands,
  PIC16F193X can switch from GPIO mode to UART mode in the wrapper.
- MSSP, ADC, Comparator, EEPROM, DAC, FVR, SR latch, capacitive
  sensing, LCD driver, CCP3/4/5.
- Timer1 gate (T1GCON, DS41364B §16.6).
- Timer1 external clock + T1OSC (DS41364B §16.5).
- Migration of PIC16F87XA and PIC18Fxxxx from UART to GPIO reporting
  in their HARNESS=sim builds (would save the USART init flash bytes
  but is otherwise a no-op; do it if a build size budget ever forces
  it).

## References

- `docs/pic16f193x-plan.md`, the family-addition plan.
- `docs/adding-a-device.md`, the operational procedure (Path B,
  §4 gate, §6 deliverables).
- `docs/multi-family-plan.md`, the fixed contract.
- `docs/docker-dev-plan.md`, the Docker image and `make mdb-test`
  flow.
- DS41364B §16.0 (Timer1), §16.4.1 (atomic read), §16.5 (T1OSC),
  §16.6 (gate), §16.8 (write order); §2.2 (BSR + SFR map); §4.0
  (interrupts); §6.0 (GPIO, ANSEL); §15.0 (Timer0, for the sim-step
  pattern).
- `pic16f87xa-hal/include/peripherals/pic16f87xa_timer1.h` and
  `pic16f87xa-hal/src/peripherals/pic16f87xa_timer1.c`, the
  PIC16F87XA Timer1 driver this spec mirrors.
- `pic16f87xa-hal/src/core/pic16_harness_sim_target.c`, the
  PIC16F87XA sim-target harness this spec's new file mirrors.
- `scripts/sim-mdb-run.sh`, the wrapper to be generalized.
- `pic16f87xa-hal/mcu/pic16f87xa-tick-mplabx/Makefile`, the
  PIC16F87XA sim-target Makefile with `HARNESS=sim` and
  `CONFIG_WDTE := OFF`, the pattern this spec mirrors.
