# PIC8_ -> EPIC_ uppercase rename

Status: **done. Tooling built (gsub + two targeted fixups the gsub
cannot catch), validated in an isolated worktree AND re-validated in
the real working tree; applied to `master` and committed. Not pushed
(pending explicit human approval, per the user's standing rule).**

## Why

Follow-up to `docs/pic8-epic-rename-plan.md` (the lowercase `pic8_`/
`pic8-` pass). That pass deliberately left the uppercase `PIC8_` macros
and CMake variables untouched (its Scope decision 4), to keep tier 2's
namespace distinct from tier 1's `EPIC_` per-family HAL contract. The
user has now reversed that decision: rename `PIC8_` -> `EPIC_` too, and
does not care about the tier-namespace merge.

## Scope

- Uppercase `PIC8_` -> `EPIC_`, case-sensitive, everywhere it is this
  project's own naming: ~2123 occurrences across 222 files, ~70 distinct
  tokens. Covers C preprocessor macros (`PIC8_REG8`, `PIC8_BIT*`,
  `PIC8_WEAK`, `PIC8_SFR_PTR`, `PIC8_HARNESS_RESULT`, `PIC8_BANK*`,
  `PIC8_PIE_*`, the per-module `PIC8_*_MAX_*`/`PIC8_*_RING_SZ`/...), the
  `-DPIC8_*=...` compile-define flags that override them, CMake
  variables (`PIC8_COMMON_DIR`, `PIC8_FAMILY_*`, `PIC8_HOST_INCLUDE_DIR`,
  `PIC8_DEFAULT_DEVICE`, `PIC8_HARNESS_*`, ...), and include guards
  (`PIC8_USB_H`, `PIC8_TICK_H`, ...).
- **Collapse rule for `PIC8_EPIC_*`**: the `HAL_`->`EPIC_` pass already
  renamed the inner `HAL` in `PIC8_HAL_*` to `EPIC`, producing
  `PIC8_EPIC_LIB`/`PIC8_EPIC_SOURCES`/`PIC8_EPIC_COMPILE_DEFS`/
  `PIC8_EPIC_H`. A literal `PIC8_`->`EPIC_` gsub would give
  `EPIC_EPIC_*`. Per the user's preference, collapse `PIC8_EPIC_` ->
  `EPIC_` first, so those become `EPIC_LIB`/`EPIC_SOURCES`/
  `EPIC_COMPILE_DEFS`/`EPIC_H`. This is a *unification*, not a
  collision: the family-side `PIC8_EPIC_LIB` and the module-side
  `EPIC_LIB` (set by `epic-*/CMakeLists.txt` to the family HAL lib name)
  hold the same value when a module pulls in a family, so merging the
  name is semantically correct (the `PIC8_` prefix is the only reason
  the `HAL_` pass did not already merge them). `EPIC_SOURCES`/
  `EPIC_COMPILE_DEFS` have no standalone CMake counterpart (the Makefile
  `EPIC_SOURCES` is a separate build system). Verified by build, not
  assumed.
- Content-only: no filename contains uppercase `PIC8_` (filenames are
  lowercase), so no `git mv`, no directory moves. Simpler than the
  lowercase pass.

## Out of scope (excluded from the content pass)

Rename-meta files that deliberately encode old names as data/logic or
historical record, and must not self-edit or be rewritten:
- This pass: `docs/pic8-epic-uppercase-rename-plan.md`,
  `scripts/rename-pic8epic-uppercase.{sh,awk}`.
- The lowercase pass: `docs/pic8-epic-rename-plan.md`,
  `scripts/rename-pic8-epic.{sh,awk}`, `scripts/pic8-epic-modules.txt`.
- The `HAL_` precedent: `docs/hal-epic-rename-plan.md`,
  `scripts/rename-hal-epic.{sh,awk}`, `scripts/hal-epic-exceptions.txt`.

Everything else with `PIC8_` (live code, MANUALs, READMEs, API/
ARCHITECTURE docs, the PIC16F193X implementation plan docs under
`docs/superpowers/plans/`, `docs/adding-a-device.md`,
`docs/multi-family-plan.md`, ...) is renamed.

Note: this reverses the lowercase pass's Scope decision 4. That plan
doc's decision-4 text is left as-is (it accurately records what that
pass decided at the time); this doc is the reversal record.

## Tooling design

- `scripts/rename-pic8epic-uppercase.awk`: two rules, in order:
  1. `gsub(/PIC8_EPIC_/, "EPIC_")` (collapse the double prefix first).
  2. `gsub(/PIC8_/, "EPIC_")` (the rest; case-sensitive, so lowercase
     `pic8_` is untouched, and `PIC8_` inside `PIC8_EPIC_` is already
     gone after rule 1).
  No data file, no exceptions file: the survey found zero foreign
  `PIC8_` (the only `[A-Z0-9]PIC8_` substring hits are `-DPIC8_` compile
  flags, which the gsub renames correctly) and no vendored `PIC8_`
  (m-stack under `epic-usb/third_party/` has none).
- `scripts/rename-pic8epic-uppercase.sh`: content-only driver,
  `--dry-run` (default) / `--apply`, modeled on
  `scripts/rename-hal-epic.sh`. Enumerates `git ls-files`, skips the
  excluded rename-meta files, pre-filters on uppercase `PIC8_`, runs the
  awk, diffs (dry-run) or writes in place (apply, preserving mode +
  undoing awk's trailing-newline add).

## Validation performed

Run twice, once in a fresh isolated git worktree (detached at `master`
`884f73f`, this pass's tooling/plan files + the vendor installers
hardlinked in so the Docker image rebuild cache-hits) and once in the
real working tree after `--apply` there. Both green, identical results.

1. Leftover check (`git ls-files -z | xargs -0 grep -n 'PIC8_'`, minus
   the excluded rename-meta files): **zero leftover `PIC8_`**. `grep
   'EPIC_EPIC_'`: **0** (the collapse rule worked, no double prefix).
2. Host-sim build + ctest, every module (native `cmake -B build &&
   cmake --build build && ctest`): **19/19 PASS** in both the worktree
   and the real tree.
3. `make xc8-build MODULE=pic16f87xa-hal MCU=16F877A`,
   `MODULE=pic18fxx5x-hal MCU=18F4550`,
   `MODULE=pic16f193x-hal MCU=16F1937`: **3/3 PASS** in both.
4. `make mdb-test` for the three gates (the two `epic-tick` gates at
   `WAIT_MS=5000`, the PIC16F193X gate at `WAIT_MS=60000 MODE=gpio`):
   **3/3 PASS** in both.

### Two fixups the gsub cannot catch (Phase B), found by validation

The blind `PIC8_` -> `EPIC_` gsub is correct for contiguous tokens but
misses two split-token cases that the validation gates caught:

- **B1, include-guard collisions** (found by the host-sim build failing
  on 5 tick-dependent modules with `unknown type name 'EPIC_IRQ_Priority'`):
  the `HAL_` pass left the `hal_*` shim/family headers with guards
  `EPIC_IRQ_H` / `EPIC_LCD_H`; this pass renamed the common
  `epic_irq.h` / module `epic_lcd.h` guards (`PIC8_IRQ_H` / `PIC8_LCD_H`)
  to the same names, so `hal_irq.h` (included first) defined the guard
  and `epic_irq.h`'s `EPIC_IRQ_Priority` typedef got skipped. Fix:
  `hal_irq.h` (3 families) -> `EPIC_HAL_IRQ_H`, `hal_lcd.h` (PIC16F193X)
  -> `EPIC_HAL_LCD_H`; the common/module `epic_*` headers keep
  `EPIC_IRQ_H` / `EPIC_LCD_H`. (The other cross-family guard duplicates,
  `EPIC_GPIO_H`/`EPIC_TIMER0_H`/`EPIC_WDT_SLEEP_H`/`EPIC_H`, are
  pre-existing and benign: only one family is on the include path at a
  time.)
- **B2, magic-string dispatch** (found by the PIC16F193X mdb gate
  failing with `PORTA=0` despite byte-identical instruction code):
  `pic16f193x_harness_sim_target.c`'s `epic_harness_log` drives RA0 only
  when the format string matches `"PIC8_HARNESS_RESULT: PASS\n"` compared
  *char-by-char* (`fmt[0]=='P' && fmt[1]=='I' && fmt[2]=='C' && fmt[3]=='8'
  && ...`). The gsub renamed the string literal in `epic_harness_report`
  (`epic_harness.h`) to `"EPIC_HARNESS_RESULT..."` but could not rename
  this comparison, since `'P','I','C','8'` is split across char literals
  with no contiguous `PIC8_` token. So the sender emitted `"EPIC_..."`
  while the receiver still checked `"PIC8_..."`, the match never fired,
  RA0 stayed low, and the gate failed. Fix: chars 0-3 -> `'E','P','I','C'`
  in both the PASS and FAIL branches (chars 4+ unchanged; `"_HARNESS_
  RESULT: PASS\n"` is identical). This is a fragile pattern (a future
  rename could break it again); a `strcmp` against a shared constant
  would be more robust, but that is a refactor out of scope for this
  mechanical rename. The classic PIC16F87XA/PIC18Fxx5X gates use UART
  mode (the marker is grepped from captured UART output by
  `scripts/sim-mdb-run.sh`, which the gsub did update to grep for
  `EPIC_HARNESS_RESULT`), so they were unaffected.

Both fixups are codified in `scripts/rename-pic8epic-uppercase.sh`
Phase B and are idempotent. The worktree run that proved them used the
codified driver end-to-end (Phase A + B), not hand-edits.

## Not done, deliberately

- Committing or pushing. Commit only after the real-tree re-validation
  passes; push needs separate explicit human approval.
- The `EPIC_EPIC_*` form is never produced (collapse rule). If the build
  had broken on the `EPIC_LIB` unification, the fallback was to rename
  the offending CMake variable specifically (per the user's "rename
  cmake shit differently"), not to revert the macros to `EPIC_EPIC_*`.
  Not needed if the build passes.
