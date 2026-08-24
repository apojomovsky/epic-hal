# HAL-3a: epic-common under epic-cc and the per-module recipe

**Ticket:** `epic-hal#84`<br>
**Parent:** `epic-hal#59` (HAL-3)<br>
**Blocks:** every other HAL-3 cluster (#85-#92)

## Goal

Make `epic-common` (harness, status codes, shared fragments) build under
epic-cc on `16F877A` and `16F887`, and replace the hardcoded 887 keep-list
in `epic_build.py` with a manifest-driven selection so every later cluster
adds a manifest entry instead of touching shared tooling. Record the 877A
RAM/flash baseline in the issue.

## Baseline (measured 2026-08-24)

- XC8 877A blink: 1598 flash words, 82 RAM bytes (Memory Summary).
- XC8 887 blink: 2044 flash words, 94 RAM bytes.
- epic-cc 887 blink (current keep-list slice): builds; hex = 16402 data
  bytes = 8201 program words.
- epic-cc full 87XA HAL on 877A: `alloc: GPR demand exceeds 0x1EF` panic.
  The full peripheral set does not fit the 877A's 352 GPR bytes; the
  ticket's RAM-headroom warning is real, not hypothetical.
- 87XA files that fail under epic-cc today: `pic16_irq.c` (const flash
  GEP, epic-cc#114), `pic16f87xa_timer0.c` (ps_ratio flash GEP +
  callback indirect call, epic-cc#73), `pic16_irq_dispatch.c` (strong
  refs to every peripheral handler, undefined when sliced).
- 87XA files that already compile under epic-cc: `pic16f87xa_wdt_sleep.c`
  (shared BOR/POR helpers), `pic16f87xa_wdt_sleep_epiccc.c`,
  `pic16f87xa_gpio_epiccc.c`, `pic16_isr_vector.c` (epiccc).

## Design

### 1. Manifest: per-family `epiccc_sources` replaces the keep-list

Add an optional `epiccc_sources` list to each family table. It is the
family HAL's epic-cc conformant slice: exactly the files that build under
epic-cc and fit the part's RAM. The 887 keep-list's content is really
"the family's conformant slice", so it moves from `epic_build.py` into
the manifest, per family, and the hardcoded `if module == ... and mcu ==
...` block is deleted.

```toml
[families.PIC16F87XA]
# ... existing keys ...
epiccc_sources = [
  "pic16f87xa-hal/src/epiccc/pic16f87xa_gpio_epiccc.c",
  "pic16f87xa-hal/src/epiccc/pic16f87xa_timer0_epiccc.c",
  "pic16f87xa-hal/src/epiccc/pic16_irq_epiccc.c",
  "pic16f87xa-hal/src/core/pic16f87xa_wdt_sleep.c",
  "pic16f87xa-hal/src/epiccc/pic16f87xa_wdt_sleep_epiccc.c",
  "pic16f87xa-hal/src/epiccc/pic16_isr_vector.c",
  "pic16f87xa-hal/src/epiccc/pic16_irq_dispatch_epiccc.c",
  "epic-common/src/core/epic_harness_target.c",
]
```

`PIC16F88X` gets the equivalent 8-file slice (the current keep-list
output, verbatim). `PIC18Fxx5x` and `PIC16F193X` omit the key until their
clusters land; the epic-cc path fails loudly for them ("family has no
epiccc_sources").

Granularity is per family, not per module: the slice is a property of the
family HAL (which files have epiccc variants, which fit RAM), and every
module using the HAL on the epic-cc path gets the same slice. Later
clusters grow the slice by adding entries (e.g. `usart_epiccc.c` when
epic-serial lands), which is the ticket's "add entries, not rework".

### 2. Resolver and driver changes

- `epicmanifest.py`: `Family` gains `epiccc_sources` (default `[]`);
  `sources_for(module, mcu, variant, toolchain="xc8")` uses
  `fam.epiccc_sources` instead of `fam.hal_sources` + conditionals when
  `toolchain="epic-cc"`, and raises `ManifestError` when the family has
  none. The XC8 path is untouched (default argument).
- `epic_build.py`: `emit_build_script` passes `toolchain` through to
  `sources_for`; the keep-list block is deleted. `_epiccc_include`
  (include/target -> include/epiccc) stays. `_epiccc_source` shrinks to
  the generic `src/target/` -> `src/epiccc/` + `_target.c` ->
  `_epiccc.c` mapping (the per-file 88X/87XA mappings become
  unreachable once the manifest lists the epiccc files directly, and are
  removed); the generic mapping stays as the documented convention for a
  future module with target-only files.
- No matrix or bundle changes: the CI matrix and release bundles stay
  XC8-only (epic-cc CI is epic-hal#80, blocked on epic-cc#118).

### 3. `[modules.epic-common]` and the harness example

```toml
[modules.epic-common]
dir        = "epic-common"
sources    = ["src/core/epic_harness_target.c"]
includes   = ["include"]
depends_on = []
needs_hal  = false

[modules.epic-common.supported]
PIC16F87XA = ["16F873A", "16F874A", "16F876A", "16F877A"]
PIC16F88X  = ["16F882", "16F883", "16F884", "16F886", "16F887"]

[modules.epic-common.example.PIC16F87XA]
name    = "harness"
sources = ["tests/example_harness.c"]
config  = { BOREN = "ON", FOSC = "HS", LVP = "OFF", PWRTE = "ON", WDTE = "OFF", WRT = "OFF" }

[modules.epic-common.example.PIC16F88X]
name    = "harness"
sources = ["tests/example_harness.c"]
config  = { BOREN = "ON", FOSC = "HS", LVP = "OFF", PWRTE = "ON", WDTE = "OFF", WRT = "OFF" }
```

`needs_hal = false` is correct: the library (the harness target
implementation) is family-blind, exactly like epic-math's library. The
example is a new `epic-common/tests/example_harness.c`: the smallest
program that exercises the four-function contract (init, tick, running,
report), no `epic_harness_log` call (a string literal hits the
const-flash GEP, epic-cc#114). On the target it runs forever, which is
the harness contract. WDTE=OFF because the example has no WDT refresh
(epic-common has no wdt header); the family blink examples keep WDTE=ON.

The supported lists cover every variant of the two PIC14 families: the
harness is family-blind and builds on any of them (877A and 887 are the
verified acceptance parts; the rest are the same code path). The module
also joins the XC8 CI matrix automatically (two cheap builds), which is
the manifest's "nothing else to do" contract.

### 4. New 87XA epiccc variant files

Three new files under `pic16f87xa-hal/src/epiccc/`, each mirroring the
proven 88X pattern:

- `pic16_irq_epiccc.c`: `irq_table` in RAM under `__EPIC_CC__` (the
  const table is the flash-GEP panic), direct PIR1/PIR2 accesses with
  memory barriers (the file's existing `#ifndef EPIC_AT` branches already
  spell the direct path; the epiccc copy makes it unconditional and adds
  the barriers the 88X version carries).
- `pic16f87xa_timer0_epiccc.c`: straight-line prescaler mapping (no
  `ps_ratio` const array), store only the callback pointer (not a full
  handle copy), ISR clears the flag and skips the callback under
  `EPIC_AT` (indirect-call gap, epic-cc#73).
- `pic16_irq_dispatch_epiccc.c`: minimal fan-out, Timer0 + RB only, all
  other PIR1/PIR2 flags cleared without calling handlers (the 88X
  dispatch's exact shape, with the 87XA's PIE1/PIE2 read macros).

`pic16f87xa-hal/tests/example_blink.c` gains the same `__EPIC_CC__`
guards the 88X example carries: static handle, NULL callback, poll
TMR0IF in the main loop, no log/report under epic-cc. XC8 does not
define `__EPIC_CC__`, so the XC8 path is byte-identical (verified by
diffing the hex before/after).

### 5. Gate

- `make epiccc-build MODULE=pic16f87xa-hal MCU=16F877A` and
  `MODULE=pic16f88x-hal MCU=16F887` succeed.
- `make epiccc-build MODULE=epic-common MCU=16F877A` and `MCU=16F887`
  succeed.
- Harness gate: `make mdb-epiccc` (PORTB bit 0 toggle, 12 samples x
  200k stepi, deterministic) on the 877A and 887 blink hexes, using the
  build-sim layout per the target's usage text. The epic-common harness
  hex gets `make mdb-hex` with a TMR0 register read: the program is
  alive and running, not faulted (the harness is all no-ops on target, so
  an alive-check is the honest gate; the strong behavioral gates are the
  blink toggles).
- XC8 path unchanged: `make xc8-build` for 877A and 887 produce
  byte-identical hexes to the pre-change builds; the manifest test suite
  passes.

### 6. RAM baseline method (recorded in the issue)

- Flash: epic-cc hex data bytes / 2 = program words (Intel HEX record
  lengths), next to XC8's Memory Summary words.
- RAM: epic-cc's overlay demand. The driver does not print the
  allocator's `total_bank0`, so the baseline is measured by
  `--emit asm` and scanning the highest GPR address operand
  (`total_bank0 ~= max_addr - 0x20 + 1`). First-class size/map reporting
  is epic-cc#74; the asm scan is the pragmatic method until then, and
  the method is documented in the issue so later clusters can reproduce
  it.

## Steps

1. `epicmanifest.py`: `epiccc_sources` parse + `sources_for` toolchain
   parameter; unit tests.
2. `epic_build.py`: pass toolchain, delete keep-list, shrink
   `_epiccc_source`.
3. Manifest: `epiccc_sources` for PIC16F87XA and PIC16F88X;
   `[modules.epic-common]` + example.
4. New 87XA epiccc files (irq, timer0, dispatch) + example_blink guards
   + `epic-common/tests/example_harness.c`.
5. Build + gate both parts; XC8 hex diff; manifest tests.
6. Manifest README: "The epic-cc path" recipe section.
7. Record 877A baseline in the issue; takeoff ritual; PR.

## Out of scope

- PIC18 (contingent on epic-cc#75), epic-cc CI (epic-hal#80), the isel
  gaps themselves (epic-cc#73/#114, filed not worked around).
