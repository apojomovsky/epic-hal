# Dev environment scripts

Covers the host-toolchain bootstrap and pre-commit hook below. For
real-target XC8 builds and the `mdb` (MPLAB SIM) verification gate
without installing XC8/MPLAB X yourself, see the root
[`Makefile`](../Makefile) and [DEVELOPMENT.md](../DEVELOPMENT.md)'s Docker
section instead, everything runs inside a Docker image built from
installers you
drop in `docker/ci-toolchain/vendor/`. `scripts/sim-mdb-run.sh` and
`scripts/sim-test-local.sh` (this directory) are the lower-level pieces
that flow reuses; their own header comments cover direct use if you
need it.

## Bootstrap

```sh
./scripts/bootstrap.sh              # install missing host packages + the git hook
./scripts/bootstrap.sh --check-only # report what's missing, install nothing,
                                     # exit nonzero if anything is missing
```

One-time (idempotent) setup for a fresh clone: installs the host toolchain
the CMake builds need (`cmake`, `build-essential`, `cppcheck`,
`clang-format`, via `apt-get`, so Debian/Ubuntu; other package managers
get a printed package list instead of an auto-install), then runs
`install-git-hooks.sh` below, and finally verifies the Docker toolchain
for real-target work: it checks Docker is installed and reachable,
tells you which two Microchip installer files to download and where to
put them (`docker/ci-toolchain/vendor/`), and builds the toolchain
image once they are in place. Real-target (XC8) builds and the `mdb`
gate run inside that image (root `Makefile`), so nothing MPLAB-ish is
installed on the host; host builds work with or without it.

## Pre-commit hook

Installed on its own, or as part of `bootstrap.sh` above:

```sh
./scripts/install-git-hooks.sh
```

This symlinks `.git/hooks/pre-commit` to `scripts/pre-commit-checks.sh`
(`.git/hooks/` isn't tracked by git, so every clone needs to run the
installer once). Uninstall by deleting `.git/hooks/pre-commit`, or skip it
for one commit with `git commit --no-verify`.

### What it checks

1. **Trailing newline / trailing whitespace.** Auto-fixes the working-tree
   file, then blocks the commit and asks you to `git diff`, review, and
   `git add` again. Never silently changes what gets committed without you
   seeing it. If you staged only part of a file with `git add -p`, this
   still edits the whole working-tree file, review before re-adding.
2. **No em dashes**, this repo's documented style rule (`AGENTS.md`:
   "not in docs, not in commit messages, not in code comments"). Scoped to
   lines your commit actually *adds*, not the whole staged file: this repo
   has some pre-existing em-dashes from before the rule was adopted, and a
   whole-file check would block any unrelated commit that merely touches
   one of those files. Not auto-fixed (picking a comma vs. a colon vs. a
   period needs a human), the commit is blocked with the offending
   `file:line`.
3. **`cppcheck`** on staged `.c` files (`--enable=warning,performance,
   portability`), a real static-analysis gate (uninitialized variables,
   null derefs, etc.), not a style check. `unusedFunction` and
   `missingInclude`/`missingIncludeSystem` are suppressed: cppcheck
   analyzes one file at a time here, so it cannot see that a module's
   public functions are called from its own tests/examples or from other
   modules, and would otherwise flag every public API function in every
   module as dead code. Skipped with a notice if `cppcheck` isn't
   installed.

### What it deliberately does not check yet: `clang-format`

A starter `.clang-format` is in the repo root, but it is **not** wired
into the hook. Tested against a real file (`epic-pid/src/pid.c`) before
deciding this: even a style hand-picked to match this codebase's actual
conventions (4-space indent, `Stroustrup` brace style: own-line brace for
functions, attached brace for `if`/`while`/`else`) still reformatted
things this codebase does deliberately, aligned `struct` field/assignment
comments into columns, single-line `if (x) { y; }` clamp idioms expanded
across multiple lines, `} else {` split onto two lines. That is real
diff noise on files nobody actually changed the meaning of, not a bug in
the config, clang-format does not have a "match this file's existing
hand-tuned alignment" mode.

If you want to use it anyway for a specific file, `git clang-format
--diff` shows what would change without applying it, and only considers
lines your commit touches, not the whole file. Tightening `.clang-format`
enough to stop fighting this codebase's style (or deciding to reformat
the codebase once and live with the new style going forward) is future
work, not done here.

### Manual run

```sh
git add <files>
bash scripts/pre-commit-checks.sh
```

Runs the exact same checks the hook runs, without committing.

### CI

`.github/workflows/ci.yml`'s `host` job runs this same script
against a fresh checkout, which has nothing staged (everything is already
committed). Setting `PRE_COMMIT_BASE_REF=<ref>` switches every check from
"staged index vs. `HEAD`" to "`<ref>` vs. `HEAD`", so a PR gets exactly the
same em-dash/whitespace/cppcheck rules applied to the lines it actually
changed, not the whole tree (which would also flag this repo's
pre-existing violations from before the rules were adopted). Local,
hook-driven runs are unaffected: the variable is unset there, so behavior
is identical to before.

## CI change-scoping (non-code skip, affected-module narrowing)

Two classifiers keep cheap PRs cheap, both fail-closed (anything they
cannot attribute means full CI, so a wrong call can only cause an extra
run, never let a real break merge unverified):

- `ci_noncode_check.py <base-ref>`: the single source of truth for "can
  this change affect the build?". Prints `true` only when every changed
  file is on its explicit non-code allowlist (markdown, any `docs/`
  directory, `LICENSE*`, repo config files, image assets, and the
  dev-only `scripts/bootstrap.sh` / `scripts/install-git-hooks.sh` /
  repo-root `Makefile`). Workflow files and everything else are code by
  default. `family-check.yml` calls it to skip all real-target steps on
  a non-code PR; `ci-discover-affected-modules.py` imports
  `is_non_code()` for the host job.
- `ci-discover-affected-modules.py [base-ref]`: used by `ci.yml`'s
  `host` job. Answers two questions: is the change non-code (imports
  `ci_noncode_check`), and which modules were actually touched,
  directly or through a sibling dependency (read straight from each
  module's own `CMakeLists.txt`, no separately-maintained dependency
  graph to drift). Conservative on both axes, falls back to the full,
  unfiltered module list the moment it sees a changed file it can't
  attribute to a known module or `epic-common/`. Only ever applied to
  `pull_request` runs; every push to `master` always gets the full
  matrix regardless of what changed.

## CI target-job scripts (`ci-target-*.sh`)

Committed shell scripts, not inline workflow YAML, so they can be
shellchecked and dry-run locally against the real toolchain rather than
only ever exercised inside a GitHub Actions run. `ci.yml`'s `target` job
runs each one via `docker run --rm -v "$PWD:/work" -w /work <image>
bash scripts/ci-target-*.sh`, bind-mounting the checkout instead of the
old per-job `container:` field (this job also needs `python3`, which the
toolchain image deliberately doesn't have, so no single `container:`
value could cover every step in the job). None of the three stop at the
first failure; each writes a `ci-summary-*.md` PASS/FAIL table the
calling workflow step cats into `$GITHUB_STEP_SUMMARY` afterward (a file
on the runner, not reliably reachable from inside the container the
scripts themselves run in).

- `ci-target-build.sh [matrix.txt] [summary.md]`: real XC8 build for
  every `family module mcu` triple in `matrix.txt` (plain text, one
  triple per line, emitted by `ci.yml`'s emit step; deliberately not
  JSON, since the container has no `jq` either).
- `ci-target-sim.sh [summary.md]`: the fixed 3-entry `mdb`/MPLAB SIM run
  list (one per family), via `sim-mdb-run.sh`.
- `ci-target-bundle.sh [bundles-dir] [summary.md]`: the isolated bundle
  build, proving each generated bundle is self-contained by building it
  from `/isolated` (no repo above it) rather than in place. Also builds
  each bundle's reference MPLAB X project headlessly.

## `epic_build.py`, the real-target build driver

Reads `epic-common/manifest/modules.toml`, resolves a module's
dependencies and sources, and emits a POSIX `sh` script of `xc8-cc`
invocations. Replaces the `mcu/*-mplabx/Makefile`s.

```sh
python3 scripts/epic_build.py build --module epic-serial --mcu 16F877A --run
python3 scripts/epic_build.py matrix          # CI matrix JSON
python3 scripts/epic_build.py report --log build/16F877A/build.log
```

Without `--run` it only writes `build/<MCU>/build.sh`, which is the mode
CI uses: resolution needs python3, execution does not, and the toolchain
container has no python3 in it (`docker/ci-toolchain/Dockerfile`).
Reading that script is also the fastest way to see the exact command
line for any translation unit.

An unsupported `(module, MCU)` pair fails immediately with the reason
recorded in the manifest, rather than as a wall of XC8 linker errors.
`--fosc-hz` overrides the family's default oscillator frequency; an
example with no `config` table in the manifest compiles with no config
translation unit at all, matching the Makefiles that never had one
either. `epic-common/manifest/README.md` documents the schema this
driver reads.

## `make_bundle.py`, the release bundle generator

Assembles a self-contained, per-family source tree from the manifest,
plus the generated consumer files (`epicurus.mk`,
`epicurus-sources.json`, `SUPPORT.md`, `QUICKSTART.md`, `MPLABX.md`).

```sh
python3 scripts/make_bundle.py --family PIC16F87XA --version v0.1.0
```

Output lands in `bundles/`, which is gitignored: bundles are build
output, attached to a GitHub Release, never committed.

`bundlegen.py` holds the generation logic and is where every emitted
file's format lives. `.github/workflows/ci.yml`'s `target` job (its
"Isolated bundle build" step, `scripts/ci-target-bundle.sh`) proves a
bundle is self-contained by building it from a scratch directory
outside the repo.
