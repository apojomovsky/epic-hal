# CI: host tests, XC8 cross-compile, MPLAB SIM target tests, implementation plan

Status: **Phase 0 implemented, pending its first real run on GitHub
Actions to confirm the validation checklist** (`.github/workflows/
host-tests.yml`, `scripts/pre-commit-checks.sh` extended with
`PRE_COMMIT_BASE_REF` for CI reuse). Phases 1+ not started; Phases 2+
depend on a probe (Phase 2) whose findings must be recorded in this
document before Phase 3 starts.

## Motivation

There is no CI today. Every `pic8-*` module and both HALs are host-testable
(`cmake -B build && cmake --build build && ctest`) and real-target
buildable (`mcu/*-mplabx/Makefile` via `xc8-cc`), but nothing runs either
path automatically on push or PR. The goal, in order of value delivered
per unit of risk taken on:

1. Run the existing host CMake/ctest suite on every push, catching the
   easy stuff (a module that doesn't compile, a test someone broke) for
   free, immediately.
2. Cross-compile every module for real silicon with XC8, both families,
   every MCU variant, so a change that's fine on the host's memory-backed
   sim but breaks on XC8 (bank placement, inline asm, a family-specific
   register quirk) gets caught before merge instead of at hardware
   bring-up.
3. Go one step further than "it links": actually run the resulting `.hex`
   on Microchip's own instruction-accurate simulator (MPLAB SIM, driven
   headlessly through `mdb`) and get an automated PASS/FAIL back, for
   both families, without needing physical hardware in the loop and
   without paying for anything.

Steps 1 and 2 are close to mechanical. Step 3 needs real design work: the
shared harness (`pic8-common/include/core/pic8_harness.h`) currently has
no way to terminate or report a result on real target
(`pic8_harness_target.c` is four no-ops, the loop never exits, there is no
stdout). That gap is what most of this plan is about closing.

## Decision: MPLAB SIM via `mdb`, our own Dockerfile, GitHub Actions + GHCR

**Simulator: MPLAB SIM, not gpsim.** gpsim was seriously considered
(fully open source, tiny CI footprint, well-supported for classic PIC16
parts like the 877A). Rejected as the primary tool because it does not
support PIC18F2455/2550/4455/4550 at all, and `pic18fxx5x-hal` targets
exactly that family (it's the reason `pic8-usb` exists). A single
simulator that covers both families uniformly is worth more than a
lighter but PIC16-only one. MPLAB SIM, reached through the `mdb`
command-line debugger (a separate headless binary from the MPLAB X IDE
GUI, not a GUI wrapped in `xvfb-run`), is the only option that runs both.
It is also, as of the current XC8/MPLAB X releases, fully free: XC8 v4.00+
dropped the PRO-optimization license gate entirely, no key or license
server needed for anything this plan does.

**Our own Dockerfile, not the vendor CI/CD Wizard's generated one.**
MPLAB X IDE ships a CI/CD Wizard (Tools > CI/CD Wizard) that generates a
Dockerfile, a Jenkinsfile, and an `mdb` script for a project. It was
useful as a reference for the *mechanism*, confirming that "program a
`.hex` into MPLAB SIM, pipe simulated UART output to a file, grep the
file for a pass/fail marker" is a real, Microchip-documented workflow
(they pair it with the Unity test framework and a "scan output for Unity
results" step). Its actual generated output was not used: it assumes an
MPLAB X `.X` project and a Jenkins pipeline, and this repo has neither
(hand-written Makefiles, GitHub Actions). Hand-writing the Dockerfile and
`mdb` scripts keeps them in the same style as everything else in this
repo (plain, explicit, no IDE-generated boilerplate) and keeps us free to
slim the image later without fighting a generated file.

**GitHub Actions, hosted runners, no self-hosting.** Standard
GitHub-hosted Linux runners are free and unmetered for public
repositories (fair-use only, no minute cap), which this repo is. Travis
CI's free open-source tier ended in 2020; there is no reason to look at
it or any other third-party CI host.

**Build the toolchain image once, host it on GHCR, don't reinstall per
run.** The MPLAB X IDE installer alone is roughly 1.2 GB, plus XC8, plus
DFPs. Reinstalling that on every push would work but would make every CI
run slow for no benefit. Instead: a Dockerfile lives in the repo
(`docker/ci-toolchain/Dockerfile`), a separate workflow builds and pushes
it to `ghcr.io/<owner>/pic8-hal-ci` only when that Dockerfile changes
(path-filtered, or manually dispatched), and the main workflows just
`docker pull` the prebuilt image. GHCR gives free, unlimited storage and
bandwidth for public images, so there is no cost concern either way; this
is purely a latency optimization for everyday CI runs.

## Target layout

```
.github/workflows/
  host-tests.yml            # Phase 0: cmake/ctest matrix, every module + both HALs
  xc8-build.yml              # Phase 1: xc8-cc build matrix, every MCU variant, both families
  build-toolchain-image.yml  # Phase 1 infra: builds+pushes docker/ci-toolchain, path-filtered
  sim-tests.yml               # Phase 4: runs pilot module(s) under mdb/MPLAB SIM, parses PASS/FAIL

docker/ci-toolchain/
  Dockerfile                 # Debian base, XC8, MPLAB X IDE (for mdb), both DFPs

pic8-common/
  include/core/pic8_harness.h        # unchanged contract, cycles param already exists
  src/core/pic8_harness_target.c     # existing: real-target no-op variant (untouched)
  src/core/pic8_harness_sim_target.c # NEW (Phase 3): bounded, USART-reporting variant
  mk/pic8_family.mk                  # extended (Phase 3) to opt into the sim-target variant

<family>-hal/mcu/*-mplabx/
  mdb-sim-script.txt         # NEW (Phase 4): per-module or per-family mdb script
```

## Litmus test for the whole effort

A change that alters PIC18F2455 register-level GPIO or timer behavior in
a way the host's memory-backed sim cannot catch (because the host sim
does not model real bank/BSR addressing or timing) gets caught by CI
before merge: the pilot module's target-sim build runs under MPLAB SIM,
reports FAIL over simulated USART, `mdb`'s captured output gets grepped,
and the GitHub Actions job goes red. All of this happens on a standard
public GitHub-hosted runner, with no physical hardware, no license
purchase, and no manual step.

## Phases

### Phase 0: Host CI (CMake/ctest matrix)

No new tooling, no new risk. Every module already builds and tests this
way locally.

**Tasks**
1. `.github/workflows/host-tests.yml`: on push and PR, discover every
   directory with a top-level `CMakeLists.txt` (both HALs, every
   `pic8-*` module) and matrix over them: `cmake -B build -S <dir> &&
   cmake --build build && ctest --test-dir build --output-on-failure`.
   Discover the module list at workflow run time (a `find`/`ls` step
   feeding a dynamic matrix) rather than hand-listing modules in the
   YAML, so a new module is picked up without a CI edit.
2. Re-run the existing pre-commit checks (trailing whitespace, no
   em-dash, `cppcheck`) as an explicit CI step, not just a local git hook
   (a hook can be bypassed locally with `--no-verify`; CI shouldn't be).
   `scripts/pre-commit-checks.sh` is written for a staged index (a
   pre-commit hook always has one); a CI checkout has none, everything is
   already committed. Extend it with a `PRE_COMMIT_BASE_REF` env var that,
   when set, diffs a ref range instead of the index, so the same script
   and the same rules run in both places, not a parallel reimplementation.
   `clang-format` is deliberately not part of this: `scripts/README.md`
   already documents, with a concrete example, that this codebase's
   hand-tuned alignment doesn't survive it, gating CI on it would just
   break on correct, intentional code.

**Explicitly out of scope**: no XC8, no MPLAB X, no Docker. Pure host
tooling, matching what `scripts/bootstrap.sh` already sets up.

**Validation**
- [ ] A PR that breaks one module's host test fails only that module's
      matrix leg, others stay green.
- [ ] A clean push against current `master` is fully green.
- [ ] Removing a module's `CMakeLists.txt` (test in a throwaway branch)
      shrinks the matrix automatically, confirming discovery isn't
      hardcoded.

**Exit criterion**: `host-tests.yml` green on `master`, merged.

---

### Phase 1: XC8 cross-compile check (build only, no simulator)

Proves every family/MCU combination still produces a `.hex` on every
push, without yet touching the simulator problem.

**Tasks**
1. `docker/ci-toolchain/Dockerfile`: Debian base; `curl`, `make`,
   `unzip`; XC8 (current stable, see open question below on which
   version to pin) installed via its silent Linux installer
   (`--mode unattended --unattendedmodeui none`, no license flags needed
   now that PRO optimizations are free); both DFPs
   (`Microchip.PIC16Fxxx_DFP`, `Microchip.PIC18Fxxxx_DFP`) fetched as
   `.atpack` files from `packs.download.microchip.com` and unzipped to
   the paths the existing Makefiles already reference
   (`DFP_DIR` in each family's `mcu/*-mplabx/Makefile`).
2. `.github/workflows/build-toolchain-image.yml`: builds and pushes
   `ghcr.io/<owner>/pic8-hal-ci:<tag>` on changes to
   `docker/ci-toolchain/**`, or via manual `workflow_dispatch`. Tag by
   the pinned XC8/DFP versions so a stale image is obvious from its tag.
3. `.github/workflows/xc8-build.yml`: pulls the image, matrices over
   every `(family, MCU variant)` pair (873A/874A/876A/877A;
   2455/2550/4455/4550) and every module that has an `mcu/*-mplabx/`
   tree, runs `make MCU=<variant>`, asserts a `.hex` exists and the
   build exits zero.

**Explicitly out of scope**: no MPLAB X IDE, no `mdb`, no simulation.
This phase only needs the XC8 compiler.

**Validation**
- [ ] Toolchain image builds successfully and is pullable from GHCR.
- [ ] Every existing module/MCU combination that builds locally today
      also builds green in this workflow (no regressions from the
      containerized environment vs. a developer's local XC8 install).
- [ ] A deliberately broken `.c` file (throwaway test branch) fails the
      matrix leg for its module/MCU, not the whole job.

**Exit criterion**: `xc8-build.yml` green on `master`, merged.

---

### Phase 2: Add `mdb`, probe headless MPLAB SIM behavior

Design validation before any new harness code gets written, per this
repo's own convention of probing uncertain toolchain behavior instead of
assuming it. **This phase's findings must be recorded in the "Open
questions" section below before Phase 3 starts.**

**Tasks**
1. Extend `docker/ci-toolchain/Dockerfile` with a silent MPLAB X IDE
   install (needed only for `mdb`, `mplab_platform/bin/mdb`, not the
   NetBeans GUI). Confirm it installs unattended the same way XC8 does.
2. Throwaway probe (not committed as permanent test infra, per repo
   convention for exploratory toolchain checks): hand-write one
   `mdb` script against an existing PIC16F87XA example (`example_usart`
   is the natural pick, it already talks over the peripheral `mdb` needs
   to capture) and confirm, end to end, in the container:
   - `mdb.sh script.txt` runs to completion with no display/X server
     available (confirm no `Xvfb` is needed; Microchip's own
     documentation frames `mdb` as the headless/scriptable counterpart
     to the GUI, but this repo verifies before trusting it).
   - `program <path>.hex` accepts the plain `.hex` this repo's Makefiles
     already produce, no `.elf`/debug symbols required.
   - `set usart0io.uartioenabled true` / `outputfile` captures simulated
     UART output to a file readable after `halt`/`quit`.
3. Repeat step 2 against one PIC18F2455 example, confirming the same
   mechanism works on the second family (this is the family gpsim
   couldn't reach at all, so it's the one that actually matters to
   verify).

**Explicitly out of scope**: no new harness code yet, no CI wiring yet.
This phase only answers "does the mechanism work," recorded as fact, not
assumption.

**Validation**
- [ ] Both probes (PIC16, PIC18) produce a captured UART output file with
      the expected `printf` content, driven entirely by `mdb.sh` inside
      the container, no interactive step.
- [ ] Findings for each open question below are recorded, with the
      actual `mdb` script and output pasted or referenced.

**Exit criterion**: every open question tagged "resolve in Phase 2" below
has a recorded answer.

---

### Phase 3: Design and implement the sim-target harness

The actual design gap: give real-target firmware a way to terminate and
report PASS/FAIL, for the simulator case specifically, without changing
the existing infinite-loop, no-stdout contract that real hardware still
needs.

**Tasks**
1. Add `pic8-common/src/core/pic8_harness_sim_target.c`: a third
   implementation of the four-function contract, compiled only into a
   new build variant. Bounded by the existing `cycles` parameter to
   `pic8_harness_init` (already plumbed through the API, currently
   ignored on real target); `pic8_harness_running` returns false once the
   bound is hit instead of always true; on completion it reports over
   USART instead of stdout. Exact wire format is an open question below
   (resolve before writing code): the simplest thing that a `grep` in CI
   can check reliably, most likely a single terminating line rather than
   adopting Unity wholesale (Unity is what Microchip's own docs pair
   with this workflow, but pulling in a third-party test framework is a
   bigger decision than this plan should make unilaterally, flagged
   below, not decided here).
2. Extend `pic8-common/mk/pic8_family.mk` (or add a sibling fragment) so
   a module's `mcu/*-mplabx/Makefile` can opt into linking the sim-target
   harness instead of the real-target one, without duplicating the whole
   fragment. Exact mechanism (a `make` target like `sim-hex`, vs. a
   `HARNESS=sim` variable) is an open question below.
3. Pick one pilot module per family to wire up first (suggest
   `pic8-debounce` or `pic8-tick`, both already have their own plan docs
   in flight and are simple enough to validate the mechanism without a
   large surface area).

**Explicitly out of scope**: no rollout to every module yet, no CI
wiring yet (that's Phase 4). This phase proves the harness variant works
when driven by hand (`mdb.sh` locally, or in the Phase 2 container), same
discipline as Phase 1 of `multi-family-plan.md` proving the build seam
before the hardware worked.

**Validation**
- [ ] The pilot module's sim-target `.hex` builds via `xc8-cc`, same as
      today's real-target `.hex`, just linking the new harness variant.
- [ ] Run under `mdb`/MPLAB SIM (from Phase 2's probe setup): a passing
      test reports the agreed PASS marker over captured UART, a
      deliberately-broken version (throwaway edit) reports the FAIL
      marker, and both terminate on their own (no reliance on the `mdb`
      script's `wait` timeout as the only signal).
- [ ] Real-target build of the same module (`pic8_harness_target.c`
      variant) is unchanged, confirming the new variant is additive, not
      a modification of the existing target contract.

**Exit criterion**: one module, both families, produces a real PASS/FAIL
signal from MPLAB SIM, driven by a script, with no manual interpretation
needed.

---

### Phase 4: Wire the pilot into CI

**Tasks**
1. `.github/workflows/sim-tests.yml`: pulls the toolchain image, builds
   the pilot module's sim-target `.hex` for both families, runs `mdb.sh`
   with the module's `mdb-sim-script.txt`, captures UART output to a
   file, greps for the PASS/FAIL marker agreed in Phase 3, fails the job
   if FAIL, if the marker is missing, or if `mdb` itself errors
   (a silently-hanging simulator should fail loud, not fail quiet).
2. Surface the captured UART output as a build artifact on failure, so a
   failing run is debuggable from the GitHub Actions UI without
   reproducing locally.

**Validation**
- [ ] A green pilot-module run on `master`.
- [ ] A deliberately broken pilot-module change (throwaway branch) turns
      the job red for the right reason (grep sees FAIL or missing
      marker), not a container/tooling failure.

**Exit criterion**: `sim-tests.yml` green on `master` for the pilot
module, both families, merged.

---

### Phase 5: Roll out to remaining modules

**Tasks**
1. Apply Phase 3's harness-variant + Makefile pattern to each remaining
   `pic8-*` module and both HALs' own example set, module by module,
   each as its own commit per this repo's convention.
2. Extend `sim-tests.yml`'s matrix the same way `xc8-build.yml` discovers
   modules dynamically in Phase 0/1, rather than hand-listing them.
3. `pic8-usb` is explicitly deferred, not silently skipped: its host
   stub already tests ring-buffer/connection-state logic, not USB
   enumeration, and this plan does not currently establish that MPLAB
   SIM's USB SIE peripheral model is faithful enough to trust for
   enumeration-level testing (see open question below). Real hardware
   stays the source of truth for that module until/unless that's
   resolved separately.

**Validation**
- [ ] Every module (except `pic8-usb`, tracked separately) has a green
      sim-tests matrix leg, both families where applicable (some modules
      are already family-agnostic at the host level; confirm the same
      holds for the sim-target variant).

**Exit criterion**: `sim-tests.yml` covers every module, `pic8-usb`'s
deferral is documented in its own `docs/pic8-usb-plan.md`, not just here.

## Open questions (resolve during the phase noted)

- **Which XC8 version to pin.** Currently `v3.10` throughout the repo's
  Makefiles (chosen when DFPs moved out of the compiler install).
  XC8 v4.00+ removes the PRO-optimization license gate entirely, which
  simplifies the silent-install command (no `--LicenseType` flag needed
  at all). Bumping isn't required for this plan to work at `v3.10`, but
  since Phase 1 stands up fresh install scripting anyway, decide whether
  to move the pin. Resolve in Phase 1, record the chosen version and
  install command here.
- **Whether `mdb` truly needs no display server.** Believed true (it's
  documented as the headless/scriptable counterpart to the MPLAB X IDE
  GUI, distinct binary, distinct purpose), not yet independently
  confirmed for this repo's exact MPLAB X version. Resolve in Phase 2,
  record the actual container run (with or without `Xvfb`) here.
- **Whether `mdb`'s `program` command accepts a bare `.hex` with no
  `.elf`/debug symbols, and whether UART capture works without them.**
  Believed yes (UART capture doesn't need symbol info, only symbolic
  breakpoints/memory-by-name would). Resolve in Phase 2.
- **Wire format for the sim-target harness's PASS/FAIL report.** Options:
  a single terminating line (simplest, easiest to grep, no third-party
  dependency), or adopting the Unity test framework (what Microchip's
  own CI/CD Wizard docs pair with this exact workflow, gets richer
  per-assertion output, but pulls in a third-party C framework this repo
  doesn't currently use anywhere). Resolve in Phase 3, before writing
  `pic8_harness_sim_target.c`.
- **Mechanism for a module to opt a build into the sim-target harness
  variant** (separate `make` target vs. a `HARNESS=sim` variable vs.
  something else). Resolve in Phase 3 task 2.
- **MPLAB SIM's fidelity for the PIC18 SIE (USB) peripheral.** Not
  expected to be trustworthy enough for enumeration-level testing of
  `pic8-usb`; not going to be chased down as part of this plan. If it
  turns out MPLAB SIM does model the SIE well enough to be useful,
  that's a separate follow-up, tracked in `docs/pic8-usb-plan.md`, not a
  blocker here.
- **GHCR image naming/ownership** (`ghcr.io/<owner>/...`): depends on
  which GitHub account/org ends up owning the repo's packages. Fill in
  the real path when `build-toolchain-image.yml` is written in Phase 1.
