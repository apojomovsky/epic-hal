# Design note: PIC16F5x family groundwork, exemplar 16F54 (epic-hal#151)

Status: pending approval (design gate). Ephemeral plan, deleted before merge.

## Family boundary decision: ONE family, PIC16F5x / pic16f5x-hal

### Path B litmus (docs/adding-a-device.md section 5)

The 5x group is Path B: there is no existing family it can join, this is
the 12-bit baseline core, architecturally separate from every
14-bit classic-midrange family and the 14-bit-enhanced 193X. It shares
none of the classic core's addressing: no RP0/RP1 via STATUS, no
INTCON/PIR/PIE, no `pic14-midrange-core` at all. It is a new tree with
its own SFR map, platform headers, no IRQ layer and its own MANUAL.

### One family vs five (the ticket's open questions)

One family. Evidence, all verified from the pinned toolchain's DFP EDC
(`edc/PIC16Fnnn.PIC` for all five parts, cross-checked against the
`xc8/pic/include/proc/pic16fnnn.h` headers):

- The five 5x parts (16F505/506/54/57/59) all report
  `edc:arch="16c5x"`, `instructionSetId="pic12c5xx"`, one 12-bit core.
- `edc:ArchDef` `hwstackdepth="2"`: 2-level hardware stack on every one.
- `edc:MemTraits` in every part: 12-bit program words, 8-bit data.
- Common SFR block on every part (DFP-verified addresses): INDF 0x00,
  TMR0 0x01, PCL 0x02, STATUS 0x03, FSR 0x04. From there the parts
  differ only by which ports and control-space registers exist:
  - 16F54: PORTA 0x05, PORTB 0x06 (the minimal exemplar, 512 words,
    25 B GPR).
  - 16F57: + PORTC 0x07 (2048 words, 72 B GPR, 8-bit TMR0 with 8-bit
    prescaler).
  - 16F59: + PORTD 0x08, PORTE 0x09 (2048 words).
  - 16F505: no PORTA; OSCCAL 0x05, PORTB 0x06, PORTC 0x07, 20-pin,
    1024 words, 72 B GPR, 4 banks (FSR<6:5>).
  - 16F506: 16F505 + CM1CON0/CM2CON0/VRCON/ADCON0/ADRES at 0x08-0x0C.
- Same addressing model and interrupt architecture: `FSR` high bits
  select the data bank, no interrupt vector exists on any part
  (the EDC has no `interrupt_vectors` and no INTCON anywhere), TRIS and
  OPTION are dedicated instructions, not file registers.

The family is defined by the architecture, exactly as the ticket says
"12-bit baseline core, GPIO and Timer0, no interrupt vector, 2-level
stack". Per-part deltas are handled with the existing per-part macro
precedent (`PIC16F87XA_FAMILY_HAS_*`), not separate families.

### Per-part capability deltas (the shape matrix)

| fact | 16F54 | 16F57 | 16F59 | 16F505 | 16F506 |
|---|---|---|---|---|---|
| pins | 18 | 28 | 28 | 14 | 14 |
| flash (K words) | 0.5 | 2 | 2 | 1 | 1 |
| GPR (B) | 25 | 72 | 73 | 72 | 72 |
| FSR bank bits | 0 | 0 | 0 | 2 (`FSR<6:5>`) | 2 |
| data banks | 1 | 1 | 1 | 4 | 4 |
| PORTA | y (5-bit) | y | y | n | n |
| PORTB | y (8-bit) | y | y | y (8-bit) | y |
| PORTC | n | y | y | y | y |
| PORTD/E | n | n | y | n | n |
| OSCCAL | n | n | n | y (0x05) | y |
| Comparator+ADC | n | n | n | n | y (CM1/CM2/VRCON/ADCON0/ADRES) |
| TMR0 prescaler | 8-bit | 8-bit | 8-bit | 8-bit | 8-bit |
| WDT | y | y | y | y | y |
| interrupt vector | none | none | none | none | none |

- No part has any interrupt source: no INTCON, no PIR, no PIE anywhere
  in the EDC/DFP (verified for all five). This is the family-defining
  architectural fact, and it drives the IRQ-stub design below.
- All configuration is one 12-bit CONFIG word (OSC, WDT, CP; the EDC
  `ConfigFuseSector` reports `nzwidth=0xC`). No PWRTE on the baseline
  core. The epic-cc p16f54.toml models `osc`, `wdt`, `cp` exactly.
- The `__control` registers (TRISA/TRISB/OPTION) are NOT in any data
  bank: the DFP proc headers declare them `extern volatile __control`
  at `__at(0x000/0x005/0x006)` and XC8 compiles a write to them as
  `movlw` + `tris <f>` / `option` instructions (probed: `TRISA = 0xFF`
  -> `movlw 255; tris 5`). The SFR header therefore maps them as
  control-space constants, distinct from the file-register PIC_REG_*
  set, matching how the DFP spells them.

## The interesting decisions

### 1. IRQ-model stub: no interrupt vector exists on this core

The shared-contract interrupt API (`EPIC_IRQ_Disable/Restore/Enable/
DisableSrc/ClearFlag/GetFlag`, `EPIC_IRQ_SetPriority`, the
`*_IRQn` enum, `epic_dispatch_all_irqs`) is defined in
`epic-common/include/core/epic_irq.h` and each family's `core/*_irq.h`,
and the harness declares `epic_dispatch_all_irqs`.

For a core with no interrupt vector at all, the backend is a stub:
every function is a no-op (or trivially returns the passed state for
the critical-section pair), the `*_IRQn` enum is empty, and
`epic_dispatch_all_irqs` is a no-op. This is the honest model: it keeps
the fixed contract compilable and linkable for consumers who call the
API unconditionally, and there is no register to touch on this core.
The mdb/sim gates then verify the no-op-ness trivially: the GPIO +
Timer0 polled blink below needs no IRQ at all.

Contract decision, documented in the family MANUAL: applications must
poll (there is no ISR path). The stub exists so family-agnostic code
(epic-common, the harness, a consumer that calls EPIC_IRQ_Restore(1)
defensively) still builds and links unchanged. This is the "IRQ-model
stub" the ticket names.

### 2. DFP pack pin for 16F5x

The pinned `Microchip.PIC16FXXX_DFP` v1.7.162 (already in the
Dockerfile and used by every classic-midrange family) already ships all
five 16F5x parts:

- `edc/PIC16F54.PIC` (and the other four EDC files)
- `xc8/pic/include/proc/pic16f54.h` (+ 505/506/57/59)
- `xc8/pic/dat/cfgdata/16f54.cfgdata`, `cfgmap/16f54.cfgmap`
- `xc8/docs/chips/16f54.html`

No DFP pack or version bump is needed. The manifest `dfp` field for the
new family is `Microchip.PIC16FXXX_DFP` with `dfp_version = "1.7.162"`,
same as the existing families (the 7x/83_84 blocks use 1.8.167; the 5x
family block will use the pinned 1.7.162 tag from the Dockerfile, the
version actually in the image).

### 3. Host-sim strategy for a core the harness never targeted

The host sim (`src/sim/pic16f5x_sim.c`, public API in
`pic16f5x_sim.h`) follows the exact family shape: a 256-byte RAM array
standing in for the register file, a reset that loads POR values, a
`step()` that executes the small 12-bit ISA subset the HAL uses, and
`drive_input`/`read_output` helpers. The peripheral surface is tiny
(GPIO + Timer0), so the behavioral model is small: TMR0 increments on
a cycle countdown and the sim's `step` advances it (matching the
PIC16F88X finding that `stepi` advances Timer0 on the 14-bit core; the
12-bit sim models the timer in `sim_step` so the host blink example is
a real timed toggle, not register plumbing).

The sim models:
- the SFR block 0x00-0x06 (INDF indirect through FSR, TMR0, PCL,
  STATUS, FSR, PORTA, PORTB) and the TRIS/OPTION control registers as
  sim-side shadow state, matching the real CPU's write-only-ness;
- the data-file banks (0x10-0x1F on 16F54; 4-bank 0x30-0x7F shapes on
  the 505/506/57/59) via `FSR`-selected direct + indirect addressing;
- Timer0 with the 8-bit prescaler from OPTION, so the host blink
  example exercises the real `EPIC_TIMER0_*` API and asserts pin
  toggles against hand-computed cycle counts.

mdb does recognize the part: `device PIC16F54; hwtool SIM` accepted
(no unknown-device error), so the section-4 mdb gate runs on MPLAB SIM
as it does for every other family. The sim-target harness
(`src/mdb/pic16f5x_harness_mdb.c`) drives the PASS/FAIL marker on
PORTA bit 0 (RA0, present on the 18-pin 16F54) with the exact
MODE=gpio magic-string dispatch of the 193X harness, since the family
has no UART.

### 4. The Timer0 interrupt question

The 12-bit baseline core has NO Timer0 interrupt (no INTCON at all, on
any of the five parts). So Timer0 on this family is a polled timer:
the driver provides `EPIC_TIMER0_Init/Start/Stop/ReadCounter...` with
no `OverflowCallback` (no IRQ to serve it). The blink example polls
TMR0 and drives GPIO directly. This is the correct, honest surface for
the die, and it is what the ticket's "GPIO and Timer0, no interrupt
vector" means.

The shared `pic14-timer0` driver cannot be reused: it is IRQ-driven
(INTCON TMR0IF/TMR0IE, `TIMER0_IRQHandler`), which does not exist on
this core. The family owns a small `pic16f5x_timer0.c/.h` with the same
EPIC_TIMER0_* names but a polled body, per the fixed-contract rule
(same names/signatures across families, different bodies).

### 5. GPIO TRIS/OPTION access: the asm layer

Port direction and the Timer0-prescaler/OPTION bits are control-space
on this core: XC8 compiles `TRISA = v` to `movlw` + `tris 5` (probed)
and `OPTION = v` to `movlw` + `option`. So the family platform header
provides the SFR access layer exactly like the existing families
(`EPIC_REG8`/`epic_sfr_read8/write8` for the file registers) plus
control-space helpers:

```c
#define EPIC_TRIS_WRITE(port_sel, val)  \
    do { uint8_t _v = (val); \
         __asm__("movwf _v; tris " #port_sel); } while (0)
```

The target platform header implements them with XC8 asm (file-scope
static volatile operand, the pattern already proven in this codebase's
inline asm); the host platform header implements them as sim-side
shadow-register writes. This is a new per-family spelling, but it is a
direct consequence of the architecture: there is no TRISA file register
on a 12-bit core (DS41236E Table 4-1).

## Shared-core contract changes (mandatory)

`epic-common/include/core/epic_irq.h` and `core/epic_harness.h` declare
`epic_dispatch_all_irqs` and `EPIC_IRQ_*`/`EPIC_IRQ_SetPriority` and
the `EPIC_IRQ_Priority` enum are already family-agnostic: nothing in
epic-common references a register or a vector. The new family provides
its own `core/pic16_irq.h` declaring the same names with an empty IRQn
enum and no-op bodies, its own `src/core/pic16_irq_dispatch.c`
(no-op `epic_dispatch_all_irqs`), and its own WDT/sleep pair
(`src/core/pic16f5x_wdt_sleep.c` + `src/target/pic16f5x_wdt_sleep_target.c` +
`src/epiccc/pic16f5x_wdt_sleep_epiccc.c`), exactly mirroring how each
classic family owns its `pic16_irq.h` + `pic16_irq_dispatch.c`.

### The mandatory no-op harness contract

`epic-common/src/core/epic_harness_target.c` (the family-blind no-op
harness) is reused verbatim. The family's `src/mdb/pic16f5x_harness_mdb.c`
and `src/sim/pic16_harness_sim.c` implement the host/sim side of the
four-function harness, as every family does.

## Tree and files (the Path B skeleton, from the 63x/7x template)

`pic16f5x-hal/`:
- `CMakeLists.txt` (host sim build, thin caller of
  `epic-common/cmake/epic_family.cmake`; EPIC_LIB pic16f5x_hal,
  EPIC_SOURCES: family gpio, timer0, wdt/sleep core+sim, irq stub,
  harness sim, sim backend)
- `README.md`, `MANUAL.md`
- `include/pic16f5x_hal.h` (umbrella, device-select + SFR + platform)
- `include/pic16f5x_sfr.h` (file-register map for the exemplar shape,
  hand-maintained bits + PIC_REG_* / PIC_*_BIT, generated-region
  markers as `scripts/gen-sfr.py` emits for a new CANONICAL entry)
- `include/pic16f5x_sim.h` (sim backend API)
- `include/host/pic16f5x_platform.h` (EPIC_REG8 -> RAM array,
  EPIC_WEAK real, EPIC_PLACE no-op, control-space helpers -> sim shadow)
- `include/target/pic16f5x_platform.h` (EPIC_REG8 -> volatile deref,
  EPIC_WEAK empty, EPIC_PLACE __at, TRIS/OPTION asm helpers)
- `include/core/pic16_irq.h` (empty IRQn enum + no-op EPIC_IRQ_* decls)
- `include/core/pic16f5x_wdt_sleep.h` (EPIC_WDT_Refresh / EPIC_Sleep_Enter)
- `include/peripherals/pic16f5x_gpio.h` (+ no-op `hal_gpio.h` pull-in)
- `include/peripherals/pic16f5x_timer0.h` (+ no-op `hal_timer0.h`)
- `src/core/pic16_irq_dispatch.c` (no-op epic_dispatch_all_irqs)
- `src/core/pic16f5x_wdt_sleep.c` (clrwdt/sleep asm + helpers for host)
- `src/target/pic16f5x_wdt_sleep_target.c` (real clrwdt/sleep)
- `src/epiccc/pic16f5x_wdt_sleep_epiccc.c` (epic-cc intrinsics variant,
  following the pic14_wdt_sleep_epiccc pattern)
- `src/sim/pic16_harness_sim.c` (host harness)
- `src/sim/pic16f5x_sim.c` (the RAM/ISA sim)
- `src/mdb/pic16f5x_harness_mdb.c` (GPIO marker harness)
- `src/peripherals/pic16f5x_gpio.c`, `src/peripherals/pic16f5x_timer0.c`
- `tests/example_blink.c` (host + real-target via harness, Timer0 polled
  blink on PORTB bit 0), `tests/example_timer0.c` (host-only register
  probe: set prescaler, count cycles, assert exact TMR0/TRIS states)

## Manifest

`[families.PIC16F5x]` in `epic-common/manifest/modules.toml`:
- hal_dir = "pic16f5x-hal"
- variants: ["16F54", "16F57", "16F59", "16F505", "16F506"] (exemplar
  16F54 canonical first? No: the 7x/83_84 precedent puts the canonical
  part LAST so family-check's scaffold gate builds variants[-1].
  Follow that: ["16F505", "16F506", "16F57", "16F59", "16F54"],
  canonical 16F54 last.)
- dfp = "Microchip.PIC16FXXX_DFP", dfp_version = "1.7.162",
  fosc_hz = 4000000 (16F54 XT, the datasheet-typical 4 MHz crystal and
  the 84A precedent; HS to 20 MHz exists but XT is the canonical demo
  config, matching the 4 MHz the 83_84 family uses)
- includes = target, include, epic-common/include (NO pic14-midrange-core)
- hal_sources: family gpio/timer0/irq-stub/wdt-sleep target+core+harness
- epiccc_sources: gpio + timer0 + wdt + harness + irq-stub (the blink
  minimum; rides the landed p16f54 epic-cc target, epic-cc#413)
- `[modules.pic16f5x-hal]` slot (needs_hal = true, supported =
  PIC16F5x all five, example blink + sim control probe), mirroring
  the pic16f83_84-hal module slot. The example config uses the
  baseline word's setting names, verified against the pinned
  cfgdata CSETTING rows: `OSC = "XT"`, `WDT = "OFF"`, `CP = "OFF"`
  (the baseline word has no PWRTE and names the watchdog setting
  `WDT`, not `WDTE`; the epic-cc table maps `wdt` -> `wdt`
  unchanged, and p16f54.toml fields osc/wdt/cp match 1:1).
- No consumer module supports PIC16F5x yet: the manifest loader fails
  loudly until every module classifies the family, so every epic-*
  module gets its `excluded` entry, exactly as the 83_84 addition did
  (the "loader fails until every module has classified it" rule).

## CI wiring (foundation time, adding-a-device section 5 step 9)

- `scripts/gen-sfr.py` CANONICAL entry, `scripts/sfr-map-audit.py`
  FAMILIES entry (the family header + the exemplar part tuple),
  `scripts/config-key-audit.py` and `scripts/hex-identity-audit.py`
  `--family` choice lists += "PIC16F5x", plus the hal_label maps.
- `scripts/statics-audit.py`: the family has no IRQ-shared statics
  (no interrupt-context functions exist), and its RAM is tiny, so it is
  not added to BANKED_FAMILIES; the audit's per-family loop already
  covers every manifest family, so the family appears once its module
  exists.
- `scripts/ci-local-emit.py` and `.github/workflows/family-check.yml`:
  the pilot list gets `("pic16f5x-hal", "16F54")`;
  `scripts/ci-target-sim.sh` gets `run_one pic16f5x 16F54 PIC16F54
  pic16f5x-hal ... gpio` (PORTA bit 0 marker, the 83_84/193X gate
  mode; the family has no UART).
- `ci.yml` + `nightly.yml`: `family-5x` job calling family-check with
  family: PIC16F5x. The epiccc-gate job additionally covers the
  `mdb-epiccc` leg: `make mdb-epiccc MODULE=pic16f5x-hal MCU=16F54
  DEVICE=PIC16F54` runs the same epic-cc-built hex under real MPLAB
  SIM with the toggle gate (stepi-advanced, PORTB watch), which is the
  only gate that exercises both the epic-cc toolchain and real
  peripheral timing on this core.
- The epiccc-gate job: `make epiccc-build MODULE=pic16f5x-hal MCU=16F54
  EPIC_CC_HOST=1` and a sim-runner gate for the 16F54 blink. Two
  verified facts drive the gate's shape:
  - sim-runner's `Sim::build` currently hard-errors on
    `Core::PicBaseline` ("no sim model"); epic-cc's crates/sim has a
    `PicBaseline` struct but it is not routed from the runner. This
    ticket adds a `Sim::Baseline` arm to `scripts/sim-runner/src/main.rs`
    (parse_hex already handles 12-bit words; `PicBaseline` is pub with
    `with_device`/`run`/`ram`/`tris`/`option` accessors). Contained to
    this repo's own runner, verified by the epiccc-gate leg.
  - The baseline sim model executes instructions only, it does NOT
    advance TMR0 (verified by reading crates/sim: `step()` dispatches
    on opcode and never touches the timer). A Timer0-polled blink
    would never toggle under the runner. The gate's firmware is
    therefore a software-loop toggle (init GPIO, loop: set PORTB bit,
    delay, clear, delay), which the model fully executes (TRIS via
    `tris` shadow, PORTB via the RAM array). The epiccc gate then
    watches `--watch PORTB:0 --samples 12 --steps 200000` with no
    `--irq-every` (there is no IRQ on the die and no timer to inject).
  - This is the correct epiccc-gate contract anyway: it proves the
    HAL's GPIO codegen works under epic-cc for a baseline target,
    which is the "did a HAL change break against a known-good
    compiler" direction, and the section-4 mdb gate (real MPLAB SIM,
    which DOES advance TMR0) carries the timer proof.
- `scripts/pre-commit-checks.sh` dir_whitelist += "pic16f5x-hal".
- `scripts/epic_build.py` CANONICAL + the `PIC16F5x`-aware bits: the
  config-key table already passes `osc`/`wdt`/`cp` through unchanged
  (verified: p16f54.toml field names match the cfgdata CSETTINGs), and
  the `bor`/`boren` flip family set does not include PIC16F5x because
  the baseline has no BOR bit (there is no `bor` in p16f54.toml or the
  cfgdata). The `xtal_hz` and `FOSC_HZ` paths apply unchanged.

## Litmus test (adding-a-device section 5 step 10)

Point a family-agnostic consumer at the new family and confirm zero
changes to the consumer. On this ticket, no consumer module is
supported on PIC16F5x yet (RAM and flash are too small for epic-serial/
epic-tick on the exemplar; the 505/506 offer no OSCCAL-adjacent
compat), so the litmus is: the family's own example (blink) and the
family-check scaffold gate build against the family with no
epic-common code change. If a shared-core change were needed to fit the
family, that would signal the shared contract was accidentally
family-specific; the no-op IRQ stub and the control-space platform
header are family-owned, exactly as every other family owns its
platform and IRQ files, so no epic-common edit is expected.

## Exemplar 16F54 tier (this ticket)

Onboarded and verified through section 4's full ladder:

- GPIO: family `pic16f5x_gpio.c`, TRIS via the control-space asm,
  PORTA/PORTB pin read/write via EPIC_REG8.
- Timer0: family `pic16f5x_timer0.c`, polled (no IRQ on the die),
  prescaler + clock source via the OPTION control register, counters
  via TMR0 (0x01), readback asserted in the host example and the mdb
  gate.
- WDT/sleep: family `pic16f5x_wdt_sleep.c` (+ target/epiccc twins),
  `clrwdt`/`sleep` instructions, no PCON (no BOR/POR flags on the
  baseline core).
- Harness: `MODE=gpio`-style marker (RA0-equivalent PORTB/PORTA bit)
  in `src/mdb/pic16f5x_harness_mdb.c`, the exact
  `EPIC_HARNESS_RESULT` magic-string dispatch of the 193X harness,
  with the family's gpio driver; the mdb wait/stepi protocol follows
  section 4 exactly.
- Section-4 ladder for the exemplar: host ctest, XC8 cross-compile
  (real-target build for 16F54), real mdb register gate (TMR0 and
  PORTB readback vs hand-computed values).

## What the follow-up batch ticket inherits

The remaining four parts (16F505/506/57/59): the family tree, the
union SFR map, the capability macros (`PIC16F5X_FAMILY_HAS_PORTC`,
`HAS_PORTD`, `HAS_PORTE`, `HAS_OSCCAL`, `HAS_COMP_ADC`, `FSR_BANK_BITS`),
the CI slots and the manifest variants are all pre-staged. The batch
flips per-part macros, adds OSCCAL/ADC/comparator drivers where they
exist (16F506), regenerates the SFR header for the wider parts, and
onboards each part mechanically.

## Open questions for approval

1. Family key `PIC16F5x`, dir `pic16f5x-hal`, variants ordered so the
   canonical 16F54 is last (family-check scaffold gate builds
   variants[-1]).
2. IRQ-model stub shape: empty IRQn enum, no-op EPIC_IRQ_*/dispatch,
   family-owned (not in epic-common).
3. Timer0 is polled on this family (no TMR0 interrupt exists); the
   EPIC_TIMER0_* contract keeps its names, the family body is polled.
4. Control-space SFR layer: TRIS/OPTION accessed via a small asm
   helper in the family platform header (probe-verified XC8 codegen).
5. sim-runner gains a PicBaseline arm (a small edit to
   scripts/sim-runner/src/main.rs) so the epiccc-gate leg has a
   simulator; the mdb gate is the primary section-4 proof.
6. The manifest `dfp_version` for the family matches the pinned image
   (1.7.162) rather than the 1.8.167 the 7x/83_84 blocks use.
