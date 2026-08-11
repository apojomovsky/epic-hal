# HAL src/ environment split: target, sim, mdb

Status: **approved 2026-08-11, in progress**.

## Problem

Each HAL's `src/core/` mixes shared, host-sim, MPLAB-SIM, and
real-target files at the same level (`pic16_harness_sim.c`,
`pic16_harness_sim_target.c`, `pic16f87xa_wdt_sleep_sim.c`,
`pic16f87xa_wdt_sleep_target.c`, ...). All sanctioned consumption paths
(bundle, reference project, epic_build.py) are selection-driven and
correct, but nothing makes the split structural: a naive directory
import (MPLAB X "add existing folder", a `wildcard src/core/*.c` Makefile)
pulls host-only files (loud: missing `*_sim.h`) and, worse, the
`_sim_target` harness, which compiles for a real target but runs with
MPLAB-SIM semantics (silent wrong behavior). The `_sim_target` name
actively invites picking the wrong file for a real build.

## Goal

1. **Structural split** (safe by construction): each HAL's `src/`
   becomes `src/core/` (shared, target-safe), `src/target/` (target-only
   twins), `src/sim/` (host-sim only), `src/mdb/` (MPLAB-SIM gate).
   "Import core + target" is unambiguously the real-hardware build.
2. **Rename + warnings**: `*_harness_sim_target.c` -> `*_harness_mdb.c`
   (it is the mdb gate variant); each HAL README states the
   environment-split rule and that directory globbing is wrong.
3. **Executable contract**: the bundle generator asserts the bundle
   never contains `_sim`/`_mdb` files, so a manifest edit cannot
   silently leak sim files into a release.

## Decisions (user-approved: "do all 3")

1. **Layout** per HAL (mirror across pic16f87xa-hal, pic18fxx5x-hal,
   pic16f193x-hal), classified from the actual build lists:

   - `src/core/` (stays, shared): `pic16_irq.c`,
     `pic16_irq_dispatch.c`, `pic16f87xa_wdt_sleep.c` (compiled by the
     host build AND the target build), plus `src/peripherals/`
     unchanged.
   - `src/target/` (moved): `pic16_isr_vector.c`,
     `pic16f87xa_wdt_sleep_target.c` (target-build only).
   - `src/sim/` (moved): `pic16_harness_sim.c`,
     `pic16f87xa_wdt_sleep_sim.c` (host-build only; the register-file
     `pic16f87xa_sim.c` already lives here).
   - `src/mdb/` (moved + renamed): `pic16_harness_sim_target.c` ->
     `pic16_harness_mdb.c` (mdb-gate only, via the manifest's sim
     sections' `harness_src`).

   Filenames keep their `_sim`/`_target` suffixes (the directory is the
   guard, the suffix is belt-and-suspenders); only `_sim_target` ->
   `_mdb` is renamed, since that name is the trap.

2. **Consumers updated in lockstep**: the manifest (`harness_src` in
   every `.sim` section, plus the target sections' core file lists),
   the three HAL `CMakeLists.txt`, the three reference MPLAB X projects
   (`nbproject/Makefile-default.mk`, `configurations.xml`,
   `project.xml`), and the script-test fixtures
   (`test_epic_build.py`, `test_epicmanifest.py`) that embed paths.
   `make_bundle.py`/`bundlegen.py` are manifest-driven and flow through
   automatically.

3. **Dead leftover removed**: `pic16f87xa-hal/mcu/pic16f87xa-mplabx/`
   is a pre-manifest-era project that survived the `mcu/*-mplabx`
   deletion (the manifest era replaced those dirs); it references the
   old layout and is deleted.

4. **AGENTS.md** gains one convention line: the environment-split
   layout (`src/core` shared, `src/target`, `src/sim`, `src/mdb`; never
   glob a HAL src directory, select via the manifest).

5. **Bundle gate**: `make_bundle.py` asserts after assembling the file
   set that no path contains `_sim` or `_mdb` (or lies under
   `src/sim/`/`src/mdb/`), failing the build otherwise. Pinned by a
   `test_bundlegen.py` case.

## Phases

- **Task 1-3**: one HAL each: git mv per the map, rename the mdb
  harness, update `CMakeLists.txt` source list, add the README warning
  paragraph. Gate: the HAL's host cmake+ctest and the doxygen checker
  on its files.
- **Task 4**: manifest path updates (all three families: target core
  lists + every `.sim` section's `harness_src`). Gate: manifest
  validation + `epic_build.py` tests + a sim-variant emission check.
- **Task 5**: the three reference `.X` projects (nbproject source lists
  + include paths) and the `mcu/` leftover deletion. Gate: a
  path-consistency grep (every `src/` path referenced in the projects
  resolves) and the repo-side project files parse.
- **Task 6**: `make_bundle.py` bundle-gate assertion + the
  `test_bundlegen.py` case + script-test fixture updates (test_epic_build,
  test_epicmanifest) + any `statics-audit.py` path literals. Gate:
  full script test suite.
- **Task 7**: AGENTS.md convention line + final gates: full-tree
  doxygen checker, 22-module ctest sweep, pre-commit, and a bundle
  dry-run proving zero `_sim`/`_mdb` files.

## Verification

- 22-module host cmake+ctest sweep PASS (paths moved, semantics
  unchanged).
- Full-tree doxygen checker exit 0 (files moved with docstrings
  intact).
- `make_bundle.py` run for one family: asserts clean, bundle contains
  no `_sim`/`_mdb` paths.
- Path-consistency grep over the `.X` projects: every referenced
  `src/` path resolves.
- Pre-commit checks PASS; no em-dashes in added lines.

## Out of scope

- `epic-common/` (its `src/core/epic_harness_target.c` is already the
  single target harness, no environment mixing).
- Content changes: files move and the mdb harness is renamed; no code
  semantics change.
- The bundle-gate wiring into CI (the assertion fails the local
  `make_bundle.py`; CI already builds bundles, the assertion runs
  there too via the existing bundle step).
