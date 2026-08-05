# pic8_/pic8- to epic_/epic- rename

Status: **done. Tooling built, dry-run reviewed, validated in an
isolated worktree AND re-validated in the real working tree; applied to
`master` and committed. Not pushed (pending explicit human approval,
per the user's standing rule).**

## Why

The project is branded Epicurus. The per-family HAL contract tier
(`HAL_` -> `EPIC_`, uppercase) was already renamed in an earlier pass
(`docs/hal-epic-rename-plan.md`, applied to `master`: `EPIC_GPIO_Init`
60 vs `HAL_GPIO_Init` 1, the residual `HAL_` being the deliberately
preserved STM32Cube-comparison tokens). This pass renames the
family-blind common/module layer, the other naming tier: lowercase
`pic8_`-prefixed functions/macros/files/CMake targets (`pic8_harness_*`,
`pic8_tick_*`, `pic8_modbus_*`, ...) and the 17 `pic8-<name>` module
directories. It is the mechanical complement of the `HAL_` pass, not a
re-litigation of whether to rebrand.

The user has also renamed the GitHub repo from `apojomovsky/pic8-hal` to
`apojomovsky/epicurus`. That surfaces a sub-decision (see "Scope
decisions" below) about the `pic8-hal`-derived strings, which the
original prompt assumed would all stay `pic8-hal`.

## Scope, explicitly

In scope (renamed):

- Lowercase `pic8_` -> `epic_` everywhere it is this project's own
  function/macro/file/CMake-target naming (case-sensitive: uppercase
  `PIC8_` is *not* matched). ~2618 occurrences.
- The 17 module directory names `pic8-<name>` -> `epic-<name>`, as
  complete tokens: `pic8-common`, `pic8-tick`, `pic8-math`,
  `pic8-taskmgr`, `pic8-serial`, `pic8-bus`, `pic8-modbus`,
  `pic8-console`, `pic8-usb`, `pic8-settings`, `pic8-sdcard`,
  `pic8-adcfilter`, `pic8-debounce`, `pic8-pid`, `pic8-encoder`,
  `pic8-lcd`, `pic8-fsm`. ~1106 hyphenated occurrences total (including
  the preserved `pic8-hal*` below).
- The glob/set reference `pic8-*` -> `epic-*` (e.g. "every `pic8-*`
  module", "`pic8-*/mcu/...`"). As a glob it would match nothing after
  the dir renames, so it must move with them.
- Files whose basename contains `pic8_` (41 files, e.g.
  `pic8_tick.c`, `pic8_harness.h`, `pic8_family.cmake`,
  `pic8_family.mk`, `pic8_hal.h`) -> `epic_`-named equivalent, via
  `git mv`.
- The 6 existing `docs/pic8-<module>-plan.md` plan docs (encoder, fsm,
  math, modbus, sdcard, usb) -> `docs/epic-<module>-plan.md`, via
  `git mv`, because the content pass turns prose/markdown references to
  them into `epic-<module>-plan.md`; renaming the files keeps those
  links valid.
- The GitHub repo URLs `github.com/apojomovsky/pic8-hal` ->
  `github.com/apojomovsky/epicurus` (19 occurrences across `README.md`
  badges and `docs/*.md` run links). Scoped to the `github.com/` host
  so it cannot touch `ghcr.io/apojomovsky/pic8-hal-ci`.

Out of scope (deliberately preserved, see "Scope decisions"):

- Uppercase `PIC8_` macros (`PIC8_BIT*`, `PIC8_REG`, `PIC8_BANK`,
  `PIC8_WEAK`, `PIC8_SFR_PTR`, `PIC8_HARNESS_RESULT`, `PIC8_PIE_*`,
  `PIC8_COMMON_DIR`, `PIC8_EPIC_LIB`, the `PIC8_EPIC_H` include guard,
  ...). ~2098 occurrences. Case-sensitive matching leaves them
  untouched; renaming them to `EPIC_*` would merge tier 2 with tier 1's
  `EPIC_` namespace and destroy the distinction the tiers exist to
  preserve. Per the prompt's recommendation, left unchanged.
- The repo's own identity and Docker image names: `pic8-hal`,
  `pic8-hal-ci`, `pic8-hal-ci-assets`, `pic8-hal-toolchain`,
  `pic8-hal-toolchain-home`. None is one of the 17 module names, so the
  whitelist hyphen replace never matches them; the only `pic8-hal`
  strings that move are the GitHub *URLs* (above), not the image names.
- The three family HAL directories `pic16f87xa-hal/`, `pic18fxx5x-hal/`,
  `pic16f193x-hal/` (different naming scheme, not `pic8-<name>`). Their
  *content* is touched only for lowercase `pic8_` cross-references to
  the common layer (e.g. `pic8_harness_report`, `#include "pic8_hal.h"`,
  `../pic8-common` paths, `pic8_add_*` CMake function calls) and the
  `pic8_hal.h` file rename (see Scope decisions); the per-family
  `EPIC_` contract is not re-touched.
- `pic8-vga`, `pic8-ramp`, `pic8-family` (hyphenated, not module dirs:
  `pic8-vga` is a planned-but-nonexistent module in
  `docs/pic8-vga-plan.md`; `pic8-ramp` is a hypothetical future module
  named in `pic8-pid/docs/ARCHITECTURE.md`; `pic8-family` is a prose
  typo for the underscore-named `pic8_family.mk` in
  `docs/pic8-math-plan.md`). Not in the 17-name whitelist, so preserved.
- `docs/pic8-vga-plan.md` (its `pic8-vga` references stay, since vga is
  not a renamed module; the file is not moved).
- `docs/pic8-debounce-plan.md` / `docs/pic8-pid-plan.md` references in
  `pic8-pid/README.md`, `pic8-pid/docs/*`, `docs/pic8-encoder-plan.md`,
  `docs/pic8-vga-plan.md`: these plan files do not exist (pre-existing
  stale links). The content pass updates the module-name portion
  (`pic8-pid` -> `epic-pid`), so the references become
  `epic-pid-plan.md` / `epic-debounce-plan.md`; still stale (no file),
  but no regression.
- The already-completed `HAL_` -> `EPIC_` per-family contract.
- Anything under `build*/`, `docker/ci-toolchain/vendor/`, `.git/`
  (gitignored; `git ls-files` excludes them).

## Scope decisions (judgment calls, flagged not silent)

1. **`pic8_hal.h` -> `epic_hal.h` (rename).** This is the
   family-neutral top-level entry header, one copy per family HAL dir,
   `#include`d by examples and module code. Its name uses the lowercase
   `pic8_` common-layer convention (its include guard is already
   `PIC8_EPIC_H`, half-rebranded by the `HAL_` pass), but it lives in
   the dirs the prompt said "do not touch." Decision: rename it
   (treating it as tier-2 naming), with user sign-off, because a full
   Epicurus rebrand that leaves the consumer-facing neutral entry name
   `pic8_` would be inconsistent. All 3 files `git mv`'d; every
   `#include "pic8_hal.h"` and prose reference updated by the content
   pass.
2. **`pic8-hal` strings, post repo-rename.** With the GitHub repo now
   `apojomovsky/epicurus`: (a) GitHub repo URLs -> `epicurus` (fixes
   stale README badges + doc run links); (b) the GHCR package
   `pic8-hal-ci` and `pic8-hal-ci-assets`, and the local image tag
   `pic8-hal-toolchain` + cache dir `pic8-hal-toolchain-home`, are kept
   as-is (a GHCR package rename is a separate manual republish; leaving
   them keeps CI working). User sign-off.
3. **`pic8-*` glob -> `epic-*` (rename).** These refer to the set of
   modules being renamed; as literal globs they would match nothing
   afterward. Decided (not silent): rename, for correctness.
4. **`PIC8_` uppercase macros (leave).** Per the prompt's
   recommendation; agreed, no reason found to do otherwise.
5. **`pic8-vga` / `pic8-ramp` / `pic8-family` (leave).** Not module
   dirs; the whitelist inherently preserves them.
6. **Precedent tooling + plan doc (`scripts/rename-hal-epic.{sh,awk}`,
   `scripts/hal-epic-exceptions.txt`, `docs/hal-epic-rename-plan.md`):
   content-updated, not excluded.** Their lowercase `pic8_`/`pic8-`
   references move to `epic_`/`epic-` (keeps them consistent with the
   new state, e.g. the exceptions file's `pic8-common/...` path becomes
   `epic-common/...` which is where the dir now lives). Their `HAL_`
   tokens are foreign/historical and left untouched by the
   case-sensitive lowercase gsub. The precedent plan doc's stale
   "Status: not yet applied to master" line is out of scope to fix
   (pre-existing doc rot, not caused by this change).
7. **This rename's own tooling + plan doc are excluded from the
   content pass** (`scripts/rename-pic8-epic.{sh,awk}`,
   `scripts/pic8-epic-modules.txt`, `docs/pic8-epic-rename-plan.md`):
   they deliberately encode old names as data/logic; self-editing would
   corrupt them.

## Survey (done, fresh)

Driven off `git ls-files` (so `build*/`, `docker/ci-toolchain/vendor/`,
`.git/` are excluded by `.gitignore`):

- Lowercase `pic8_`: **2618 occurrences**.
- Hyphenated `pic8-`: **1106 occurrences**.
- Files with `pic8_` in the basename: **41** (incl. the 3
  `pic8_hal.h` in the family HAL dirs, and `pic8-common`'s
  `pic8_family.cmake`, `pic8_family.mk`, `pic8_harness.h`,
  `pic8_irq.h`, `pic8_harness_target.c`).
- Top-level dirs matching `pic8-*`: **17** (the module list above).
- Uppercase `PIC8_`: **2098 occurrences** (left untouched).
- `github.com/apojomovsky/pic8-hal` (URL): **19 occurrences** (-> epicurus).
- Pre-existing `epic_`/`epic-` project tokens: **0** (collision check
  clean; the only `epic-` hits are inside the precedent's
  `hal-epic-rename*` filenames, a different string).

Distinct `pic8-<token>` counts (top): `pic8-common` 225, `pic8-tick`
175, `pic8-usb` 98, `pic8-math` 87, `pic8-serial` 67, `pic8-taskmgr`
62, `pic8-hal` 47 (the exception), `pic8-sdcard` 45, `pic8-fsm` 41,
`pic8-encoder` 41, `pic8-debounce` 41, `pic8-pid` 36, `pic8-modbus` 31,
`pic8-bus` 24, `pic8-settings` 18, `pic8-adcfilter` 18, `pic8-` 18
(the `pic8-*` glob), `pic8-console` 17, `pic8-lcd` 11, `pic8-vga` 2
(planned, preserved), `pic8-ramp` 1 (hypothetical, preserved),
`pic8-family` 1 (prose typo, preserved).

CMake: 16 of the 17 module dirs have `project(pic8_<name> C)` (underscore
project name, hyphenated dir); `pic8-common` has no `CMakeLists.txt`
(it is a shared resource dir, not a CMake project) -> that is the 16-vs-17
puzzle, not a missing file. The 3 family HAL `CMakeLists.txt` reference
`pic8-common` by relative path (`set(PIC8_COMMON_DIR
${CMAKE_CURRENT_SOURCE_DIR}/../pic8-common)`, `include(${PIC8_COMMON_DIR}/cmake/pic8_family.cmake)`)
and call the `pic8_add_*` CMake functions defined in `pic8_family.cmake`;
all updated by the content pass + the `pic8_family.cmake` file rename +
the `pic8-common` -> `epic-common` dir rename.

Safety checks for the hyphen whitelist (literal string replace per
module name): no module name is a substring of a longer alphanumeric
token (`pic8-<module>[a-z0-9]+` matches: empty); module-suffix forms
that do exist (`pic8-usb-specific` in `pic8-usb/src/usb_descriptors.c`,
`pic8-<module>-plan.md` references) are *correctly* renamed
(`epic-usb-specific`, `epic-<module>-plan.md`). No module name is a
prefix of another, so replacement order is irrelevant. The lowercase
`pic8_` gsub is case-sensitive, so `PIC8_` is never matched; the survey
found zero foreign lowercase `pic8_` (no `[a-z0-9]pic8_` substring hits,
no `pic8_EPIC`/`pic8_HAL` mixed-case remnants except the intended
`pic8_hal` header name).

CI / scripts cross-references found:
- `.github/workflows/sim-tests.yml`: matrix `dir: pic8-tick/mcu/...`
  (PIC16F87XA + PIC18Fxx5X entries) -> `epic-tick/mcu/...`; the
  PIC16F193X entry's `dir: pic16f193x-hal/mcu/...` is a family dir,
  unchanged. `host-tests.yml` and `xc8-build.yml` discover modules
  dynamically (`git ls-files -- '*/CMakeLists.txt'`,
  `scripts/ci-discover-xc8-matrix.py`), so they adapt automatically;
  `xc8-build.yml`'s `pic8-hal-ci` image ref is preserved.
- `scripts/ci-discover-xc8-matrix.py`: hardcoded
  `pic8-<module>/mcu/...` paths and `pic8-common/` prefix checks ->
  updated by the content pass.
- `scripts/ci-discover-affected-modules.py`: `pic8-common/` prefix
  string checks, `pic8-tick/CMakeLists.txt` example references, and
  module-dir-prefix matching -> updated.
- `scripts/sim-mdb-run.sh`, `scripts/sim-test-local.sh`: prose path
  examples (`pic8-tick/mcu/...`) and the `pic8-hal-ci` image ref
  (preserved) -> examples updated, image ref preserved.
- Root `Makefile`: `ALL_MODULES`/`TEST_MODULES` discovered dynamically
  (adapts); `pic8-hal-toolchain`/`pic8-hal-ci`/`pic8-hal-toolchain-home`
  preserved; a comment "see pic8-tick and friends" and the mdb-test
  usage example `MODULE=pic8-tick/mcu/...` -> updated.

## Tooling design

Three files, mirroring the precedent's auditable shape but simpler
(this rename needs *no exceptions file*: the hyphen replace is a
*whitelist* of the 17 module names + the `pic8-*` glob, so `pic8-hal*`,
`pic8-vga`, `pic8-ramp`, `pic8-family` are inherently preserved; and
the lowercase `pic8_` gsub is case-sensitive, so `PIC8_` is inherently
preserved).

- `scripts/pic8-epic-modules.txt`: data file, one module name per line
  (the 17), `#` comments. The whitelist source.
- `scripts/rename-pic8-epic.awk`: per-line content substitution, three
  rules, in order:
  1. `gsub(/pic8_/, "epic_")` (case-sensitive lowercase underscore).
  2. For each module `m` from the data file: `gsub("pic8-" m, "epic-" m)`
     (literal string replace; safe per the survey), then
     `gsub(/pic8-\*/, "epic-*")` for the glob.
  3. `gsub(/github\.com\/apojomovsky\/pic8-hal/, "github.com/apojomovsky/epicurus")`
     (scoped to `github.com/` so `ghcr.io/.../pic8-hal-ci` is never
     matched).
  Reads the module list via `-v MODS=<path>`. No rule touches `PIC8_`,
  `pic8-hal*`, `pic8-vga`, `pic8-ramp`, or `pic8-family`.
- `scripts/rename-pic8-epic.sh`: driver, `--dry-run` (default) or
  `--apply`. Excludes the 4 self-files from the content pass. Phases:
  - **A (content)**: `git ls-files -z`, skip excluded, pre-filter on
    lowercase `pic8` (catches `pic8_`, `pic8-`, and the GitHub URL;
    skips `PIC8_`-only files), run awk, diff (dry-run) or write in place
    via temp + `mv` preserving mode (and undoing awk's trailing-newline
    add, like the precedent).
  - **B (file `git mv`)**: rename every tracked file whose basename
    contains `pic8_` (41) to the `epic_`-basename equivalent, plus the
    6 `docs/pic8-<module>-plan.md` -> `docs/epic-<module>-plan.md`.
    Basename-only substitution; dir path unchanged at this phase.
  - **C (dir `git mv`)**: rename the 17 `pic8-<name>/` dirs to
    `epic-<name>/`, each only if present.
  Order in `--apply`: A (content) -> B (file moves) -> C (dir moves).
  Files-first then dirs so the pre-computed file list (old paths) stays
  valid. `--dry-run` prints per-file content diffs plus the planned
  `git mv` commands; writes nothing.

## Validation performed

Run twice, once in an isolated git worktree (detached at `master`
`a0be1ca`, the 4 tooling/plan files copied in, vendor installers
hardlinked so the Docker image rebuild cache-hits) and once in the real
working tree after `--apply` there. Both green, identical results.

1. Leftover check (`git ls-files -z | xargs -0 grep -nE 'pic8_|pic8-'`,
   minus the deliberately-preserved `pic8-hal*`, `pic8-vga`,
   `pic8-ramp`, `pic8-family`, and the 4 excluded self-files): **zero
   unintended `pic8_`**, **zero unintended `pic8-`**. `grep
   'github.com/apojomovsky/pic8-hal'`: **0 stale URLs** (19 moved to
   `epicurus`). `PIC8_` uppercase count unchanged at 2098.
2. Host-sim build + ctest, every module (native `cmake -B build &&
   cmake --build build && ctest`, the CLAUDE.md-sanctioned fast-inner-
   loop equivalent of `make test`): **19/19 PASS** in both the worktree
   and the real tree (16 `epic-*` modules + the 3 family HALs;
   `epic-common` has no `CMakeLists.txt`, correctly absent). Note: the
   real tree initially flunked because of 49 pre-existing gitignored
   `build/` dirs with stale `pic8_`-named CMake cache from prior dev
   work; cleaning them first (`rm -rf <module>/build`) made every
   module pass, confirming the rename itself is correct (a clean
   `epic-tick` build reached 100%).
3. `make xc8-build MODULE=pic16f87xa-hal MCU=16F877A`,
   `MODULE=pic18fxx5x-hal MCU=18F4550`,
   `MODULE=pic16f193x-hal MCU=16F1937`: **3/3 PASS** (the `xc8-build`
   target runs `make clean` first, so stale XC8 `mcu/.../build` dirs
   did not interfere).
4. `make mdb-test` for the three existing gates:
   - `MODULE=epic-tick/mcu/pic16f87xa-tick-mplabx MCU=16F877A
      DEVICE=PIC16F877A DFP=Microchip.PIC16Fxxx_DFP WAIT_MS=5000`
   - `MODULE=epic-tick/mcu/pic18fxx5x-tick-mplabx MCU=18F4550
      DEVICE=PIC18F4550 DFP=Microchip.PIC18Fxxxx_DFP WAIT_MS=5000`
   - `MODULE=pic16f193x-hal/mcu/pic16f193x-mplabx MCU=16F1937
      DEVICE=PIC16F1937 DFP=Microchip.PIC12-16F1xxx_DFP WAIT_MS=60000
      MODE=gpio` (the 60000/gpio values are load-bearing, not arbitrary:
      `sim-tests.yml` documents that PIC16F193X's `example_timer1.c`
      loops 2M C-level iterations before `report()`, needing >=30000ms
      under MPLAB SIM; 60000 chosen for margin).
   **3/3 PASS** in both the worktree and the real tree.

The two `epic-tick` mdb MODULE paths above are the renamed `pic8-tick`
dirs; the PIC16F193X path is the family dir `pic16f193x-hal` (not
renamed). All gates produced the same PASS marker as pre-rename.

## Not done, deliberately

- Committing or pushing. Commit happens only after the real-tree
  re-validation passes; push needs separate explicit human approval
  (per the user's standing rule).
- The optional second, fresh-context review pass (recommended, matching
  what the `HAL_` rename did): sweep for awkward post-substitution
  prose, missed cross-references, the deliberate-leave items above, then
  re-run the validation suite. Tracked as a follow-up, not part of the
  mechanical apply.
