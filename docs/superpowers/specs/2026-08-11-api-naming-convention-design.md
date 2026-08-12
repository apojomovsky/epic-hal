# Enforce the epic_* API naming convention

Status: **approved 2026-08-11, in progress**.

## Problem

The repo's implicit API naming convention is: a module `epic-X` exports
functions named `epic_x_*` (epic_tick_init, epic_serial_init,
epic_lcd_clear); the HALs' cross-family public contract is `EPIC_*`
uppercase (EPIC_GPIO_Init); family-internal helpers use the family
prefix (pic16f87xa_*, pic18_*, pic16f193x_*). The convention is
nowhere written, and eight modules violate it:

| Module | Current prefix | Convention |
|---|---|---|
| epic-math | `pic_math_*` (21 symbols) | `epic_math_*` |
| epic-debounce | `debounce_*` (3) | `epic_debounce_*` |
| epic-encoder | `encoder_*` (7) | `epic_encoder_*` |
| epic-fsm | `fsm_*` (3) | `epic_fsm_*` |
| epic-pid | `pid_*` (6) | `epic_pid_*` |
| epic-swuart | `swuart_*` (incl. test hooks) | `epic_swuart_*` |
| epic-console | `console_*` | `epic_console_*` |
| epic-taskmgr | `task_manager_*`, `task_spawn` (8) | `epic_taskmgr_*` |

## Goal

1. Rename every violating public symbol to its `epic_<module>_*` form,
   updating all callers (headers, sources, tests, examples, combos,
   the target selftest, the golden-vector generator, READMEs and
   per-module docs) so a repo-wide grep finds zero old-prefix
   references.
2. Write the naming convention into AGENTS.md so it is enforced, not
   assumed.
3. Pure symbol renames: no behavior change; golden vectors regenerate
   identically (the generator's own calls rename, outputs are the
   same values).

## Decisions (user-approved: "Fix 'em all and open PR once you're done")

1. **The convention** (added to AGENTS.md, under the module anatomy /
   conventions section):

   - Module `epic-X` exports `epic_x_*` (lowercase), e.g.
     `epic_serial_init`.
   - HALs export `EPIC_*` uppercase for the cross-family contract
     (EPIC_GPIO_Init); family-internal helpers use the family prefix
     (`pic16f87xa_*`, `pic18_*`, `pic16f193x_*`).
   - `epic-common` harness and shared glue use `epic_*`.
   - Third-party code keeps its own names.

2. **Rename map** (prefix substitution per module, all symbols):

   - `pic_math_` -> `epic_math_`
   - `debounce_` -> `epic_debounce_`
   - `encoder_` -> `epic_encoder_`
   - `fsm_` -> `epic_fsm_`
   - `pid_` -> `epic_pid_`
   - `swuart_` -> `epic_swuart_`
   - `console_` -> `epic_console_` (already-conforming
     `epic_console_init` stays)
   - `task_manager_` -> `epic_taskmgr_`; `task_spawn` ->
     `epic_taskmgr_spawn`

   Careful exclusions: `epic_` already-prefixed names stay; the
   `pic_math_` prefix must not touch the HALs' `pic16f87xa_`/`pic18_`/
   `pic16f193x_` internals; `console_` must not touch the C standard
   library; third-party untouched.

3. **Completeness gate** per rename and at the end:

   `grep -rnE '\b(pic_math|debounce_|encoder_|fsm_|pid_|swuart_|console_|task_manager|task_spawn)[a-z0-9_]*\(' . --include='*.c' --include='*.h' --include='*.md' | grep -v third_party` -> zero matches.

4. **No behavior change**: golden_vectors.h regenerates with identical
   values (the generator's calls rename); all host ctest and the mdb
   gates must stay green.

## Phases

- **Task 1**: AGENTS.md naming-convention rule (one bullet under the
  conventions section).
- **Task 2**: epic-math rename (21 symbols + every caller: epic-pid,
  tests, examples, target_selftest, gen_golden_vectors, MANUAL/API/
  ARCHITECTURE/README docs). Regenerate golden vectors. Gate: epic-math
  ctest + the module grep clean.
- **Task 3**: epic-taskmgr rename (8 symbols + all callers incl. the
  root README's task_manager examples and the combos). Gate:
  taskmgr ctest + grep clean.
- **Task 4**: epic-debounce + epic-encoder renames. Gate: both ctest +
  grep clean.
- **Task 5**: epic-fsm + epic-pid renames. Gate: both ctest + grep
  clean.
- **Task 6**: epic-swuart + epic-console renames. Gate: both ctest +
  grep clean.
- **Task 7**: final gates: the completeness grep zero repo-wide
  (excluding third_party), full 22-module ctest sweep, doxygen checker
  full+brief, pre-commit, bundle dry-run; ephemeral plan deletion.

## Verification

- Completeness grep zero matches; per-module ctest green; full ctest
  sweep green; checker green (function names rename, docstrings'
  @param names unchanged); pre-commit green; golden vectors unchanged
  in value.

## Out of scope

- The HAL internals (`pic16f87xa_*` etc. are family-internal by
  convention, not module APIs).
- Third-party code.
- Any behavior change beyond symbol names.
