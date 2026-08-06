# CI consolidation: 4 workflows/~14 jobs down to 1 workflow/2 jobs

Status: **implemented, pending a real CI run to confirm** (local dry
runs against the real XC8 v3.10 toolchain pass, see Validation below;
the `mdb`/MPLAB SIM step and the Docker-image-pull path can only be
proven by an actual GitHub Actions run, not locally).

## Problem

A one-line manifest change (`docs/superpowers/plans/2026-08-06-pic16f193x-fsm.md`,
PR #3) produced 26+ separate GitHub PR checks: `host-tests.yml`,
`xc8-build.yml`, `sim-tests.yml`, and `bundle-gate.yml` had grown to
~14 job definitions between them, several matrixed per family
(`build (PIC16F193X, ...)`, `build (PIC16F87XA, ...)`,
`build (PIC18Fxx5x, ...)`, same for `sim-test`) or per module
(`build-test (epic-adcfilter)`, `build-test (epic-bus)`, ... 14 of
these). User request: squash related checks together, "at most 3", the
per-family/per-module split specifically called out as unnecessary.

## Decision

Two jobs, one file (`.github/workflows/ci.yml`), replacing all four:

- **`host`** (bare runner, no Docker): every module/HAL's `cmake`+`ctest`
  in one job with an internal loop (same PASS/FAIL step-summary pattern
  the old `xc8-build.yml` already used for MCU variants within a
  family), plus the repo's pre-commit checks (lint) folded in as a
  step.
- **`target`** (one Docker pull): real XC8 builds across all 3 families,
  all 3 `mdb`/MPLAB SIM runs, and the isolated bundle-gate build, as
  sequential steps in one job instead of three separate
  workflows/jobs each pulling the toolchain image independently.

Two checks total on a normal PR (`host`, `target`), matching the "at
most 3" request with headroom.

### Why `target` needed `docker run`, not the job-level `container:` field

The old `xc8-build.yml`/`sim-tests.yml`/`bundle-gate.yml` jobs used
GitHub Actions' `container:` field, which runs every step of that job
inside the named image. `target` needs both python3-only steps
(reading `epic-common/manifest/modules.toml`, emitting build scripts,
generating bundles, none of which the toolchain image can do, see
`docker/ci-toolchain/Dockerfile`'s deliberate lack of python3) and
`xc8-cc`/`mdb.sh` steps, so no single `container:` value could cover
the whole job. Each toolchain-dependent step instead runs `docker run
--rm -v "$PWD:/work" -w /work <image> ...` directly. This also
eliminates the old `actions/upload-artifact`/`download-artifact` pair
between the resolution job and the build job: everything now runs on
one runner's filesystem, so a bind mount replaces the artifact
round-trip.

### Why the container payloads are committed scripts, not inline YAML

The first draft inlined the build/sim/bundle loops as `docker run ...
bash -c '<script>'` strings inside the workflow YAML. That is a
double-quoting problem waiting to happen: the inner script's own quotes
and heredocs have to survive being embedded inside the outer YAML
`run:` block's quoting, and the bundle-build loop's Makefile heredoc
(`cat > Makefile <<EOF`) actually failed this exactly once during
development, both because a heredoc terminator indented to match the
surrounding YAML block breaks bash's exact-match terminator rule, and
because the same indentation breaks Make's "recipe lines start with a
literal tab" rule. Extracting the three payloads to
`scripts/ci-target-build.sh`, `scripts/ci-target-sim.sh`,
`scripts/ci-target-bundle.sh` (real files, shellcheck-able, dry-runnable
locally) removed the problem entirely rather than fixing the quoting by
hand.

### Why `matrix.txt`, not `matrix.json`

Same reason as the script-vs-inline decision: the container has neither
python3 nor `jq` (confirmed against `docker/ci-toolchain/Dockerfile`),
so `ci-target-build.sh` can't parse JSON. The emit step (bare runner,
has both) reuses `scripts/epic_build.py matrix`'s own family/module/mcu
filter rather than re-deriving it, then flattens that JSON into
`matrix.txt`, one `family module mcu` triple per line, which the
container-side script reads with a plain `while read`.

## Tradeoff, accepted knowingly

The old per-family/per-module jobs ran concurrently (GitHub schedules
matrix jobs on separate runners). Collapsed into one sequential job
each, `host` and `target` both take longer wall-clock than the old
fastest-parallel-branch time, in exchange for a much shorter PR check
list. Not measured yet (pending a real run); expected to land somewhere
in the 5-10 minute range for `target` given the old per-family jobs
individually took 1-2 minutes each and there are 3 families' worth of
real builds plus 3 sim runs plus the bundle build now serialized into
one job.

## Validation

Local, against the real XC8 v3.10 toolchain installed at
`/opt/microchip/xc8/v3.10` (see `CLAUDE.md`'s Build & toolchain
section):

- Emission (the `epic_build.py matrix` + per-entry `epic_build.py
  build` calls): produces 72 `build.sh` scripts and a 72-line
  `matrix.txt`, matching `docs/ci-plan.md`'s own recorded "72 build
  legs" figure from when this matrix was first built.
- `scripts/ci-target-build.sh` against all 72: all PASS, 0 FAIL, real
  `.hex` output for every (family, module, MCU) triple.
- `scripts/make_bundle.py` for all 3 families + `scripts/ci-target-bundle.sh`
  (isolated-build path patched to a writable `/tmp` dir locally, since
  `/isolated` needs root, which the real toolchain container runs as
  but a local dev shell doesn't): all 3 bundles PASS.
- `scripts/ci-target-sim.sh` not locally runnable: needs `mdb.sh`,
  which only exists inside the toolchain image, never installed
  locally. Confirmed via syntax check (`bash -n`) and shellcheck only;
  real validation is the first actual `ci.yml` run.
- Every `run:` block in `ci.yml` (including the inner `docker run ...`
  command strings) round-tripped through `bash -n` after extracting
  each step's script with a small PyYAML-based checker, to catch
  quoting/heredoc breakage before pushing rather than after.

Not yet validated: the actual GitHub-hosted runner's Docker pull of the
private GHCR image, and the real `mdb`/MPLAB SIM run. First real push
of this branch is the check for both.

## What this plan deliberately does not do

- Change what any check actually verifies. Every build, test, and gate
  the old four workflows ran still runs; only the job/file boundaries
  changed.
- Touch `docs/superpowers/plans/2026-08-07-mplabx-projects-and-release.md`,
  a separate, still-unmerged branch's plan (`mplabx-projects-and-release`,
  one commit ahead of `master` as of this writing) whose Task 4 and
  Task 5 reference `bundle-gate.yml` by name. That file is gone on
  `master` after this lands; whoever next executes that plan's Task 4/5
  needs to adapt them to `ci.yml`'s `target` job instead. Flagged, not
  fixed here, since it's a different branch's plan.
- Re-tune the `mdb` `wait_ms` budgets, RAM-exclusion lists, or any other
  behavior `docs/mplabx-link-gaps-plan.md`/the per-module `excluded`
  tables already cover. Purely a job/file restructuring.
