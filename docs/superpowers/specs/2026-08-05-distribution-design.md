# Distribution and consumption design

Status: **phases 1 to 3 implemented**. Design agreed 2026-08-05,
implementation plans written 2026-08-05. Phases 1 and 2 (the manifest and
build driver, CI cutover) landed via
`docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md`. Phase 3
(the bundle generator) landed via
`docs/superpowers/plans/2026-08-06-bundle-generator.md`: its exit
criterion, every bundle's demo building from a scratch copy outside the
repo, is met (`bundle-gate` green). Phases 4 and 5 (MPLAB X projects,
release) are next. Supersedes nothing; complements
`docs/mplabx-link-gaps-plan.md` (see "Relationship to the link-gaps plan"
below).

Implementation plans, in order:

1. `docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md`
   (phases 1 and 2)
2. `docs/superpowers/plans/2026-08-06-bundle-generator.md` (phase 3)
3. `docs/superpowers/plans/2026-08-07-mplabx-projects-and-release.md`
   (phases 4 and 5)

## Problem

Epicurus has no consumption story. There are no tags, no releases, and
every `mcu/*-mplabx/Makefile` hardcodes sibling-relative paths
(`EPIC_DIR ?= ../../../pic16f87xa-hal`). The only way to use any of this
is to clone the whole repo and work inside its layout. There is no path
to "take `epic-serial` into my project."

Two consumer groups need serving:

1. **Bare XC8 users**, who do not want MPLAB X and drive `xc8-cc`
   themselves, typically from a Makefile.
2. **MPLAB X users** (the IDE, or the MPLAB extension for VS Code), who
   expect a project to open and a properties dialog to fill in. Today
   exactly one real MPLAB X project exists in the tree
   (`pic16f87xa-hal/mcu/pic16f87xa-mplabx/nbproject`); the other 28
   `*-mplabx` directories are bare Makefiles despite the name.

## Decisions

| Question | Decision |
|---|---|
| Distribution format | Source, never prebuilt binaries |
| Unit of consumption | One bundle per family |
| Bare-XC8 interface | Generated `epicurus.mk` fragment |
| MPLAB X story | One reference `.X` per family plus generated instructions |
| Publication | CI builds bundles on tag, attaches to a GitHub Release |
| Unsupported combos | Machine-readable matrix, build-time error |
| Internal build | Manifest-driven Python script, the 29 Makefiles are deleted |
| First release | All three families, `v0.1.0` |

### Why source and not binaries

XC8 `.a`/`.lpp` archives are tied to compiler version, target device,
and optimization level, and the free-tier versus PRO split means a
maintainer-built binary is useless to a large share of users. Source
distribution is effectively forced, not chosen.

### Why one bundle per family

A consumer picks their family and receives `epic-common` plus that
family's HAL plus every family-agnostic module, as one tree. They
compile only what they need. Per-module packages would duplicate the HAL
across artifacts and push dependency resolution onto the user; a
whole-repo snapshot would push all pruning and path-wiring onto them.

The PIC16F193X bundle is HAL-only for v0.1.0, because the higher-level
modules are currently wired to PIC16F87XA and PIC18Fxx5x only. This is a
legitimate bundle, just a thinner one, and it is a useful stress test of
the generator precisely because it is the degenerate case.

## Architecture

### The manifest is the single source of truth

The knowledge "which sources does module X need, and on which parts does
it work" is currently restated in two places and would be restated a
third time by any bundle:

- copy-pasted `EPIC_SOURCES` lists across 29 `mcu/*-mplabx/Makefile`s
  (26 per-module, 3 per-HAL)
- a `KNOWN_BROKEN` Python literal in
  `scripts/ci-discover-xc8-matrix.py`, which is the support matrix
  expressed as its inverse

The design replaces both with one declarative manifest,
`epic-common/manifest/modules.toml`, holding per module: its own
sources, its module dependencies, the HAL peripheral set it requires,
its supported parts, and a one-line reason for every exclusion.

```toml
[epic-serial]
sources    = ["src/epic_serial.c"]
depends_on = []
supported  = { PIC16F87XA = ["16F876A", "16F877A"],
               PIC18Fxx5x = ["18F4455", "18F4550"] }
excluded   = { "16F873A" = "RAM: 32B rx buffer will not fit",
               "16F874A" = "RAM: 32B rx buffer will not fit" }
```

Example programs, including their configuration words, are manifest data
too. Config words are the one thing today's Makefiles hold that is not
otherwise derivable: each has a bespoke recipe that `printf`s `#pragma
config` directives into a generated translation unit.

```toml
[examples.epic-serial]
name    = "example_echo"
sources = ["examples/example_echo.c"]
config  = { PIC16F87XA = { FOSC = "HS", WDTE = "ON", PWRTE = "ON",
                           BOREN = "ON", LVP = "OFF", WRT = "OFF" } }
```

Exactly three things read the manifest, and nothing else encodes the
knowledge:

1. **`scripts/epic-build.py`**, the internal real-target build driver.
2. **`scripts/ci-discover-xc8-matrix.py`**, which loses its
   `KNOWN_BROKEN` literal and derives the CI matrix from `excluded`.
3. **`scripts/make-bundle.sh`**, which copies exactly the files the
   manifest names and generates the consumer-facing artifacts from the
   same table.

### Module dependency resolution

The build driver and `epicurus.mk` both resolve `depends_on`
transitively. Requesting `modbus` pulls in `serial` and `tick`.
Requiring users to hand-list transitive dependencies is a bug factory
and is not done anywhere in this design.

### HAL peripheral set

Each bundle compiles its family's full HAL peripheral set, matching what
the two HAL Makefiles that link successfully today already do. Trimming
per module would require redesigning `pic16_irq_dispatch.c`'s
strong-reference contract, which is an open design question in
`docs/mplabx-link-gaps-plan.md` root cause 1 and a separate project. The
support matrix absorbs the consequence: a module that does not fit a
small part is recorded as excluded, with the reason.

## Internal build

`scripts/epic-build.py` becomes the only way a real-target build
happens. It:

1. reads the manifest and resolves `depends_on` transitively
2. validates `(module, MCU)` against `supported`, failing early and
   readably on an unsupported pair
3. generates the config-word translation unit from manifest data
4. emits a self-contained POSIX `sh` script of `xc8-cc` invocations,
   one `.p1` per source plus one link to `.hex`
5. runs that script (`--run`) or leaves it for another runner to execute
6. parses the XC8 memory summary from the build log and reports flash
   and RAM usage

### Why the driver emits a script instead of calling xc8-cc directly

The toolchain image (`docker/ci-toolchain/Dockerfile`) is
`debian:12-slim` with `ca-certificates curl unzip make tar`, the GTK
runtime libraries, and `cmake build-essential`. It has **no python3**,
deliberately, which is also why `ci-discover-xc8-matrix.py` packs its
matrix into a bash-parseable flat string today. A Python driver therefore
cannot run in the container where `xc8-cc` lives.

Adding python3 to the image would work but makes every build depend on a
human-gated `make ci-image-push`, since CI never builds the image itself.
Emitting a script avoids that entirely: the resolution phase runs
wherever python3 exists (a developer's host, or the GitHub runner, which
already runs `ci-discover-xc8-matrix.py`), and the execution phase needs
only `sh` and `xc8-cc`. No image change, and the container keeps the
no-python3 property it was given on purpose.

The emitted script is also a debugging artifact: it records the exact
`xc8-cc` command line for every translation unit, which suits a codebase
whose convention is to inspect generated output rather than assume it.

Step 5 matters beyond ergonomics: it turns "this module is RAM-marginal"
from folklore into a number CI can print and track, which is the
evidence `docs/mplabx-link-gaps-plan.md` root cause 2 needs to decide
"never fit" versus "avoidably wasteful" per module.

```
$ ./scripts/epic-build.py --module epic-serial --mcu 16F877A
resolved: epic-serial -> [] (no deps)
sources:  22 files (HAL 19 + module 1 + example 2)
config:   FOSC=HS WDTE=ON PWRTE=ON BOREN=ON LVP=OFF WRT=OFF
OK  build/16F877A-echo.hex  (4128 bytes flash, 91 RAM)
```

### Migration gate: byte-identical `.hex`

For every `(module, MCU)` pair that builds green today, the migration
builds it both the old way and the new way and **diffs the `.hex`**.
Identical sources, flags, and compiler produce byte-identical output;
any diff means the manifest disagrees with the Makefile it replaces, and
that is caught mechanically rather than by inspection.

A family's Makefiles are deleted only once every green pair for that
family matches. This is why the migration is sequenced family by family
rather than done in one pass.

### What changes in CI

- `xc8-build.yml`: the `discover` job (which already runs python3 on a
  bare runner) resolves builds and emits scripts; the containerised
  `build` job runs `sh` over them instead of `make -C`.
- `ci-discover-xc8-matrix.py`: `KNOWN_BROKEN` deleted, matrix derived
  from the manifest's `excluded` entries.
- `sim-tests.yml`: `.hex` output paths retargeted.
- `host-tests.yml`: **untouched**. The host build stays on CMake/ctest.
  That split is deliberate: the host side genuinely benefits from ctest
  and has none of `xc8-cc`'s two-phase `.p1` weirdness.

### What survives

- The root `Makefile` stays, retargeted. It is a Docker command wrapper,
  not a build system: `make test`, `make shell`, and `make xc8-build`
  are memorable names for `docker run` lines. Its `xc8-build` target
  becomes a one-line call into `epic-build.py`. Converting it would mean
  inventing a new command vocabulary for no gain.
- `pic16f87xa-hal/mcu/pic16f87xa-mplabx/nbproject`, the one real MPLAB X
  project, is promoted rather than deleted: it seeds that family's
  reference `.X` project.

## Consumer artifacts

### Bundle layout

```
epicurus-pic16f87xa-v0.1.0/
  epic-common/  pic16f87xa-hal/  epic-serial/  epic-tick/  ...
  epicurus.mk               generated: module -> sources, includes, guards
  epicurus-sources.json     the same resolved data, for non-make consumers
  SUPPORT.md                generated: per-module/per-part table + reasons
  MPLABX.md                 generated: exact folders and include paths
  QUICKSTART.md             copy-pasteable first build
  examples/epicurus-demo.X/ open in MPLAB X, hit Build
  VERSION  LICENSE
```

No path in a bundle escapes the bundle.

### `epicurus.mk`

Generated, so nobody hand-maintains it. Pure data plus a guard: no build
rules, and no assumption about the consumer's Makefile beyond its
ability to `include`. It sets `EPICURUS_SRCS`, `EPICURUS_INCLUDES`, and
`EPICURUS_CFLAGS`, and hard-errors on an unsupported `(module, MCU)`
with the exclusion reason carried through from the manifest.

```make
EPICURUS_DIR := third_party/epicurus
EPICURUS_MCU := 16F877A
EPICURUS_MODULES := serial tick taskmgr
include $(EPICURUS_DIR)/epicurus.mk

SRCS := main.c $(EPICURUS_SRCS)
CFLAGS += $(EPICURUS_CFLAGS)
```

A bad combination fails immediately and readably, instead of as a wall
of XC8 linker errors:

```
epicurus.mk:41: *** epic-modbus is not supported on 16F873A
  (RAM: does not fit, see SUPPORT.md). Supported on PIC16F87XA: none.
```

Make is kept as the consumer interface despite the internal move away
from it, because it is the lingua franca of bare-XC8 development and
MPLAB X can import a Makefile project. The repo's internal build system
and the artifact handed to consumers do not have to be the same thing.

### `epicurus-sources.json`

The same resolved data as `epicurus.mk`, in a form the other personas
can read. MPLAB X users cannot `include` a `.mk`, and neither can
someone driving XC8 from CMake, a shell script, or the VS Code
extension. It is also what `MPLABX.md` is generated from, so the "add
these folders, set these include paths" instructions cannot rot the
moment a source file moves.

### MPLAB X

One reference `.X` project per family, each a working demo with the
bundle already wired in. A user opens it, confirms it builds, then
copies its settings into their own project, with `MPLABX.md` covering
the "add to an existing project" path.

Three committed `nbproject` trees is sustainable; the 29 that mirroring
the current `mcu/` layout would require is not, because `nbproject` XML
is generated, pins DFP versions, and rots quickly.

## Release pipeline

Tagging `v0.1.0` triggers `release-bundles.yml`, which for each family
runs `make-bundle.sh`, gates the result, and attaches the tarballs plus
`SHA256SUMS` to a GitHub Release. Nothing generated is committed to the
repo. One version number spans all three families.

### The gate builds the bundle in isolation

CI copies each bundle to a scratch directory **outside the repo** and
builds its demo there with `xc8-cc`. Building in place would let a
missing file quietly resolve back through the repo's sibling layout,
which is exactly the failure mode packaging introduces, so the gate is
designed specifically to catch it. A demo that builds from an isolated
copy proves the bundle is self-contained.

### Open verification item

Whether the reference `.X` projects can be built headlessly in CI is
**not assumed**. The toolchain image ships `mdb`, which implies an MPLAB
X installation, which should mean `make -f nbproject/Makefile-default.mk`
works in-container. If it does, the `.X` projects get the same gate as
everything else. If it does not, they are hand-verified at release time.
Per this repo's convention, that is a throwaway probe before it is a
plan, and it runs before three `nbproject` trees are committed.

### Versioning

`v0.1.0`, all three families at once. The `0.x` prefix signals that the
`epicurus.mk` interface may still move, which it will; release notes say
so and tell adopters to pin the tag.

## Sequencing

This is more than one implementation plan's worth of work. It decomposes
into five phases, each independently verifiable and each leaving the repo
green:

1. **Manifest and build driver.** Write
   `epic-common/manifest/modules.toml` for PIC16F87XA only, plus
   `scripts/epic-build.py`. Nothing is deleted and no CI changes. Exit
   criterion: every green PIC16F87XA pair produces a `.hex` byte-identical
   to its Makefile's. **Done**, along with phase 2's rollout in the same
   plan; see
   `docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md`.
2. **Family rollout and CI cutover.** Extend the manifest to PIC18Fxx5x
   and PIC16F193X under the same hex-diff gate, delete each family's
   Makefiles once its pairs match, then retarget `xc8-build.yml` and
   `sim-tests.yml` and derive `ci-discover-xc8-matrix.py`'s matrix from
   `excluded`. Exit criterion: no `mcu/*/Makefile` remains and CI is
   green. **Done.**
3. **Bundle generator.** `scripts/make-bundle.sh` (implemented as
   `scripts/make_bundle.py`, see that plan's naming note) plus the
   generated `epicurus.mk`, `epicurus-sources.json`, `SUPPORT.md`,
   `MPLABX.md`, and `QUICKSTART.md`. Exit criterion: each bundle's demo
   builds from a scratch copy outside the repo. **Done**, `bundle-gate`
   green (PIC16F87XA and PIC18Fxx5x built and linked in isolation,
   PIC16F193X correctly reported HAL-only); see
   `docs/superpowers/plans/2026-08-06-bundle-generator.md`.
4. **MPLAB X projects.** Run the headless-build probe first, then commit
   three reference `.X` projects seeded from the surviving
   `pic16f87xa-mplabx/nbproject`. Exit criterion: each opens and builds,
   gated in CI if the probe says that is possible.
5. **Release.** `release-bundles.yml`, then tag `v0.1.0`.

Phase 4's probe has no dependency on phases 1 through 3 and can be run at
any point; doing it early de-risks phase 4 before three `nbproject` trees
are committed.

## Relationship to the link-gaps plan

`docs/mplabx-link-gaps-plan.md` documents 40 of 112 `(module, MCU)` legs
failing to link, including `epic-modbus` having zero surviving PIC16
variants. This design does not fix those and is not blocked by them.

It changes where they are tracked. `KNOWN_BROKEN` moves into the
manifest as `excluded` entries with reasons, so the link-gaps plan's
exit criterion becomes "every `excluded` entry is gone", the same goal
recorded in one place instead of two. The manifest also makes root cause
1 impossible to reintroduce by hand: hand-maintained `EPIC_SOURCES`
lists that omit a peripheral stop existing once the driver computes them.

Auditing all 29 Makefiles to build the manifest is the bulk of the work
in this project. It is work that would otherwise be done piecemeal while
paying down the link-gaps debt.

## Out of scope

- Fixing any `KNOWN_BROKEN` combination. Recorded, not repaired.
- Redesigning the `pic16_irq_dispatch.c` strong-reference contract.
- Per-module HAL peripheral trimming, which depends on the above.
- Wiring the higher-level modules to PIC16F193X.
- A starter template project. The `.mk` fragment plus the reference `.X`
  are the entry points for v0.1.0.
- Any change to the host CMake/ctest build.

## Prerequisite, already done

The git remote pointed at `apojomovsky/pic8-hal` while README badges
pointed at `apojomovsky/epicurus`. The project was renamed upstream; the
remote was corrected to `git@github.com:apojomovsky/epicurus.git` and
verified against upstream `master` on 2026-08-05.
