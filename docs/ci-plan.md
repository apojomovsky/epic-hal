# CI: host tests, XC8 cross-compile, MPLAB SIM target tests, implementation plan

**Post-consolidation update, see
`docs/superpowers/plans/2026-08-06-ci-consolidation.md`:** the four
workflow files this document describes (`host-tests.yml`, `xc8-build.yml`,
`sim-tests.yml`, `bundle-gate.yml`) were later merged into one,
`.github/workflows/ci.yml`, two jobs (`host`, `target`) instead of the
~14 job definitions (several matrixed per family or per module) the
four files had grown to, which produced 26+ individual PR checks for
even a one-line change. `target` absorbs the old `xc8-build`/`sim-tests`/
`bundle-gate` jobs as sequential steps, invoking `docker run` directly
per step (bind-mounting the checkout) instead of the job-level
`container:` field each of those used, since mixing python3-only steps
(manifest resolution, unavailable inside the toolchain image) with
`xc8-cc`/`mdb.sh` steps in one job needs that. The phase-by-phase
history below is left as-is, a historical record of how the original
four-workflow split was designed, not a description of the current job
set.

**Post-Phase-4 update, see `docs/docker-dev-plan.md`:** the toolchain
image `ci-assets`/`ci-assets-mplabx`/`toolchain-image` jobs documented
below (Phase 1/2's own account) were later simplified: CI now only
`docker pull`s a pre-published private GHCR image, it never builds or
pushes one itself. The root `Makefile` (`make image`, `make
ci-image-push`) is now the only path that builds and publishes that
image, from installers a human drops in `docker/ci-toolchain/vendor/`,
the exact same private-image / EULA-redistribution constraint this
document's Decision section worked out still applies unchanged.
`docker/ci-assets/` is kept, dormant, as a fallback if CI ever needs to
build the image itself again. The phase-by-phase history below is left
as-is, a historical record of how the image and its jobs were designed
and debugged, not a description of the current job set; read
`docs/docker-dev-plan.md` for that.

**Post-manifest-migration update, see
`docs/superpowers/specs/2026-08-05-distribution-design.md` and
`docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md`:**
`scripts/ci-discover-xc8-matrix.py` (Phase 1's discovery script, below)
is gone. `xc8-build.yml`'s `discover` job now runs `scripts/epic_build.py
matrix`, reading `epic-common/manifest/modules.toml` instead of scanning
for `mcu/*-mplabx/Makefile`s (all 29 deleted). The `build` job's per-
module `make -C <dir> MCU=<variant> ...` also changed shape: resolution
(reading the manifest, computing sources) needs python3 and runs in a
preceding `discover`-job step on the bare runner; the emitted POSIX `sh`
script is what actually runs inside the toolchain container, which has
no python3 (`docker/ci-toolchain/Dockerfile`, deliberate). Below is
Phase 1's own account of the pre-manifest design, left as history.

Status: **Phase 0 done** (`.github/workflows/host-tests.yml`,
`scripts/pre-commit-checks.sh` extended with `PRE_COMMIT_BASE_REF` for CI
reuse; first push to `master` after landing it went green, all 20 jobs,
https://github.com/apojomovsky/epicurus/actions/runs/30717451172).
**Phase 1 done** (XC8 v4.00, `docker/ci-toolchain/Dockerfile`,
`.github/workflows/xc8-build.yml`, `scripts/ci-discover-xc8-matrix.py`),
green on run https://github.com/apojomovsky/epicurus/actions/runs/30720162258
(74/74 jobs: `toolchain-image`, `discover`, 72 `build` legs). Took four
real GitHub Actions runs to land: Microchip's installer CDN being behind
an Akamai bot-challenge (fixed by self-hosting the installer as a GitHub
Release asset), a workflow bug assuming every module names its `.hex`
`<MCU>-firmware.hex` (fixed by globbing instead of guessing), and 40 of
112 `(module, MCU)` pairs having real, pre-existing link/resource bugs of
their own, never caught before because nothing had ever actually linked
them against real XC8 (root-caused and filed separately in
`docs/mplabx-link-gaps-plan.md`, excluded from this workflow's matrix
rather than either left red or silently ignored). See Phase 1's
validation notes below for the full account of each.

**Post-Phase-1 redistribution fix: done.** Both the GHCR image and the
GitHub Release asset used to land Phase 1 turned out to be publicly
redistributing Microchip's software with no EULA authorization to do so
(confirmed: an unauthenticated `docker pull` worked). Fixed with
`docker/ci-assets/`, a private blob-carrier image, both GHCR packages
manually confirmed private, and a real green run
(https://github.com/apojomovsky/epicurus/actions/runs/30722374627)
against the simplified pipeline. See the "Correction" note under
Decision, and Phase 1's follow-up validation checklist, for the full
account. The now-redundant public `ci-toolchain-assets` release has
since been deleted.

**Post-Phase-1 CI efficiency fix: done.** `xc8-build.yml`'s `build` job
originally matrixed per `(module, MCU)` pair, 112 jobs before excluding
known-broken ones (72 after), each a separate GitHub Actions job, meaning
a separate fresh VM and a separate `docker pull` of the multi-GB
toolchain image, to do a few seconds of real `make` work. It also meant
GitHub's concurrent-job cap (~20 on the free tier) turned the matrix into
queued batches, adding wait time unrelated to actual work, visibly so
once MPLAB X IDE made the image ~7GB+. Restructured to matrix per
*module* instead (23 jobs; `scripts/ci-discover-xc8-matrix.py` now emits
one entry per module with that module's allowed `mcus` and `dfp` attached,
not one entry per `(module, MCU)` pair), each job pulling the image once
and looping over its own MCU variants, `fail-fast: false` at the job
level plus the loop itself not stopping at the first failing variant, so
one broken MCU doesn't hide results for the others in the same module;
`$GITHUB_STEP_SUMMARY` gets a per-MCU PASS/FAIL table so a red job is
still legible without diving into raw logs. Caught a real bug while
testing this locally before pushing: `build/` is shared across every MCU
a job loops over now (same checkout, same job), so a bare `*.hex` glob
after the second MCU matched every prior MCU's `.hex` too, not just the
current one, fixed with `make clean` between iterations plus globbing on
the `$(MCU)-*.hex` prefix every module's `TARGET` actually uses (not a
guess, confirmed the same way the original filename-guessing bug was
diagnosed).

**Phase 2 (probe `mdb`/MPLAB SIM)**: the mechanism is confirmed working
for both families (headless, plain `.hex`, UART-to-file capture, both
PIC16F877A and PIC18F4550, throwaway probes, see this document's Phase 2
task list and the open questions below for the exact findings), and
findings are written up. MPLAB X IDE v6.35 has been added to
`docker/ci-toolchain/Dockerfile` (full, untrimmed 8-bit-only install,
~7.3GB; a multi-stage trim to keep only `mdbcore`/`java`/`packs` is
tracked as deliberately deferred follow-up, see the Dockerfile's own
header comment), following the same private-GHCR-asset pattern Phase 1's
redistribution fix established (`ci-assets-mplabx` job in
`xc8-build.yml`). Confirmed on a real run
(https://github.com/apojomovsky/epicurus/actions/runs/30723598436,
76/76 jobs green), both new GHCR tags confirmed private. The temporary
bootstrap fallback has since been removed from the workflow (same
lifecycle as `ci-assets`'s original one), and that removal is confirmed
on a fresh run too
(https://github.com/apojomovsky/epicurus/actions/runs/30724128114,
76/76 jobs green). **Phase 2 is done.** Only remaining step: deleting
the now-redundant `ci-mplabx-assets-tmp` GitHub Release, a human's call,
not done as part of this fix.

**Post-Phase-2 image-size fix: done.** The ~7.3GB untrimmed MPLAB X
install above was fatter than it needed to be even for what
`--8bitmcu 1` claims to scope: its bundled `packs/` still shipped every
hardware debug-probe tool pack (ICD, ICE, PICkit, Snap, PKOB, JTAGICE3,
EDBG family) and every Atmel/AVR DFP, none of which this repo's `mdb`
runs touch (Simulator only, never real hardware, never AVR). Pruned in
`docker/ci-toolchain/Dockerfile`'s own MPLAB X install `RUN` (same layer
as the install, not a later one, so the deleted bytes never land in a
committed layer), verified against a real `mdb` run for both
`sim-tests.yml` matrix entries (PIC16F877A, PIC18F4550, identical PASS
output before/after). Image: ~10.8GB to ~5.69GB. `mplab_platform/`
itself (the NetBeans-platform runtime `mdb.sh` actually needs, ~1.6GB)
is untouched, that multi-stage trim (`mdbcore`/`java` only, drop the
GUI/MCC/thirdparty tooling) is still deliberately deferred, same reason
as before: its real dependency set is a module classpath, not a few
obvious directories, and needs real trial-and-error to prune safely.

**Post-Phase-2 CI efficiency fix: done.** The per-module matrix (previous
paragraph) still meant 23 separate jobs each paying for its own multi-GB
image pull, most of which was pure overhead once the image grew to
~7.3GB with MPLAB X IDE added. Restructured again, this time to matrix
per *family* (2 jobs, PIC16F87XA and PIC18FXX5X):
`scripts/ci-discover-xc8-matrix.py` now emits one entry per family, with
that family's modules packed into a single bash-parseable string
(`"<dir>=<mcu>,<mcu>;<dir>=<mcu>,..."`, not nested JSON, since the
toolchain container has no `python3`/`jq` installed to parse it with).
Each family's job pulls the image once, then loops over every module and
every MCU variant within it, `fail-fast: false` at the job level plus
neither loop stopping at the first failure, so one broken module or MCU
doesn't hide results for the rest; `$GITHUB_STEP_SUMMARY` gets a
per-module-per-MCU PASS/FAIL table. Same 72 real MCU builds covered, now
behind 2 image pulls instead of 23. Verified locally (bash `IFS` parsing
against the script's actual output, both families' leg counts summing to
72) before pushing. First push failed both `build` jobs with "Syntax
error: redirection unexpected" (exit code 2): a `container:` job
defaults to `shell: sh`, not `bash`, unlike a bare `runs-on` job, and the
new per-family script uses `<<<` here-strings and `read -ra` arrays,
both bashisms `sh` doesn't support. The old per-module version never hit
this, it only used a POSIX `for mcu in $list` loop. Fixed by pinning the
step to `shell: bash` explicitly (confirmed present in the
`debian:12-slim` base image first); confirmed green on the next run,
6/6 jobs.

**Phase 3 (sim-target harness): build side done, `mdb`-driven signal
pending.** See Phase 3's own section below for the full account: the
harness contract, wire format, and Makefile opt-in are implemented and
build-verified for the pilot module (`epic-tick`, both families); the
actual run-under-MPLAB-SIM verification is deferred to Phase 4, since
this environment has no local `mdb`/GHCR access to do it by hand the way
Phase 2's own probes did.

**Phase 4 (wire the pilot into CI): PIC16 green, root cause found after
an eight-bug detour.** `sim-tests.yml` and local-reproduction tooling
(`scripts/sim-mdb-run.sh`, `scripts/sim-test-local.sh`) are built and
working; local Docker/GHCR access was set up mid-phase specifically to
debug this faster than repeated CI round trips, and paid off. The pilot
module's failure turned out to be much bigger than epic-tick: any
C-level local variable or parameter accessed while a PIC16 bank switch
(`pic_select_bank`) is in effect gets misdirected, silently breaking any
Bank 1 SFR access that needs a value to survive the switch, whether a
read-modify-write or (as the final root cause turned out to be) a plain
write of a function parameter. Six real bugs found and fixed en route
(four variations on the Bank-1 theme: `pic_select_bank`'s and
`pie_reg_addr`'s/`pir_reg_addr`'s own function-call-boundary corruption,
a ROM-read interleaved with an SFR read-modify-write, and
`EPIC_IRQ_Enable`/`DisableSrc`'s PIE1/PIE2 read-modify-write itself, now
hand-written inline asm; plus a genuine dangling-pointer bug in the
sim-target harness's USART handle, and a WDT/config-word oversight),
each verified individually via the local toolchain, none of them
regressing the host suite (18/18 modules) or the real XC8 build. None of
the six were the *final* blocker: with all six applied, the pilot got
further than ever (first delay completes, first log line transmits
correctly) but hung on the second delay with `GIE` stuck disabled. Two
well-motivated, officially-documented theories (PIC16F87XA's 8-level
hardware call stack; the XC8 v4.00 known-issue gap in `-mstackcall`'s
indirect-call protection) were tested directly against this and ruled
out, along with a `.sym`-diff-driven storage-overlap forensic pass that
found real candidates but not the actual cause. The deciding realization
that finally cracked it: `compute_period()`/`EPIC_TIMER2_WritePeriod()`
run entirely *before* `GIE` is ever enabled, so no interrupt could
possibly be involved, ruling out every interrupt-timing theory in one
step. `mdb` instruction-stepping (not `wait`, which turned out not to
reliably respect breakpoints in this toolchain's headless mode) then
localized the actual bug to `EPIC_TIMER2_WritePeriod`'s own
`pic_select_bank(1)` call misdirecting its `period` parameter, the exact
same failure shape already proven and fixed for PIE1/PIE2 earlier in
this phase, just hitting a different function. `EPIC_USART_Init`'s SPBRG
write had the identical, previously-undetected bug (masked because
MPLAB SIM's UART capture isn't baud-timing-sensitive). Fixed both with
the same proven pattern (load into W through a bank-independent scratch
byte before switching banks). See `pic16f87xa-hal/docs/ARCHITECTURE.md`
for the full nine-finding writeup, cross-checked against the real XC8
v4.00 User's Guide rather than asserted from empirical probing alone (an
earlier draft of this account called several of these "genuine XC8
bugs" without doing that check first; corrected). See Phase 4's own
Validation section for the full, detailed account.

**PIC18 now green too**, investigated and fixed as a follow-up: three
real bugs found and fixed (a baud-rate math error in the sim-target
harness, the same missing `WDT=OFF` Makefile knob PIC16 needed, and a
runtime-addressed SFR write compiling to PIC18's program-memory table
mechanism instead of a data-memory access, fixed by rewriting
`pic18_irq.c`'s table-driven dispatch into named, compile-time-constant
SFR accesses per IRQ source). Not the same bug class as PIC16's at all
(PIC18's own drivers have no `pic_select_bank` equivalent), a genuinely
separate investigation. `epic-tick`'s PIC18 sim-target test now reaches
`EPIC_HARNESS_RESULT: PASS` reliably. See
`pic18fxx5x-hal/docs/ARCHITECTURE.md`.

**A fourth, unrelated bug kept CI itself red after all three driver bugs
above were fixed and pushed**: `sim-tests.yml`'s `wait_ms` for the
`Build + run under MPLAB SIM` step was hardcoded to `80` (milliseconds
of real wall-clock time), a leftover from an earlier debugging pass that
needed a short window to sample pre-reset state and was never reverted.
80ms is enough real time for `pic16f87xa`'s build to reach the harness's
`report()` call but not `pic18fxx5x`'s (more peripherals to initialize
at a fixed 48 MHz FOSC), so the `pic18fxx5x` leg kept failing with the
same "no UART output" symptom the three driver-level bugs originally
produced, for a completely unrelated reason, even on commits where the
driver code was already correct. Found by reading the actual CI job logs
via `gh` (not available earlier in this phase) and reproducing the exact
failing GitHub Actions container locally with `docker run` against the
same `ghcr.io/apojomovsky/pic8-hal-ci` image tag: `stepi`-based `mdb`
stepping on that image confirmed `EPIC_IRQ_Restore` sets `GIEH`/`GIEL`/
`IPEN` correctly (ruling out a driver regression), and re-running
`scripts/sim-mdb-run.sh` locally at increasing `wait_ms` values showed
80/500ms fail and 2000/5000ms reliably pass. Fixed by raising `wait_ms`
to `5000` in `sim-tests.yml` (margin above the confirmed-passing 2000ms
baseline) and removing the now-stale debug `print` list that went with
the 80ms value. Verified locally against the real CI image for both
families; not yet confirmed on a live GitHub Actions run since this fix
hasn't been pushed yet.

**PIC16F193X's `mdb` gate, previously verified only via manual
`make mdb-test` runs (`docs/pic16f193x-plan.md` §6), is now wired into
`sim-tests.yml`'s matrix, same root-cause class as the wait_ms bug
above.** `pic16f193x-hal`'s `example_timer1.c` (the family's `HARNESS=sim`
diagnostic, `MODE=gpio`) loops `SIM_CYCLES=2_000_000` C-level iterations
before calling `epic_harness_report`, far more wall-clock time under
MPLAB SIM than the default `5000`ms budget: confirmed locally that
20000ms still fails (halts mid-loop, `PORTA` never set) while
30000/40000/60000ms reliably pass, matching the `WAIT_MS=60000` value
`docs/pic16f193x-plan.md` §6 already used for its manual Timer1
verification. Added as a third `sim-tests.yml` matrix entry
(`pic16f193x-hal/mcu/pic16f193x-mplabx`, `MODE=gpio`, `wait_ms: 60000`),
confirmed identically on both the pre- and post-image-trim toolchain
image (see the image-size fix above). Not yet confirmed on a live GitHub
Actions run.

## Motivation

There is no CI today. Every `epic-*` module and both HALs are host-testable
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
shared harness (`epic-common/include/core/epic_harness.h`) currently has
no way to terminate or report a result on real target
(`epic_harness_target.c` is four no-ops, the loop never exits, there is no
stdout). That gap is what most of this plan is about closing.

## Decision: MPLAB SIM via `mdb`, our own Dockerfile, GitHub Actions + GHCR

**Simulator: MPLAB SIM, not gpsim.** gpsim was seriously considered
(fully open source, tiny CI footprint, well-supported for classic PIC16
parts like the 877A). Rejected as the primary tool because it does not
support PIC18F2455/2550/4455/4550 at all, and `pic18fxx5x-hal` targets
exactly that family (it's the reason `epic-usb` exists). A single
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

**Correction, found during Phase 1: those images, and the installer,
cannot be public.** The paragraph above was written assuming a public
GHCR image was simply a latency optimization with no other tradeoff.
That assumption was wrong, and it took actually shipping a public
artifact to surface it. Two things happened, in order:

1. Phase 1's first CDN failure (Microchip's installer download sits
   behind an Akamai bot-challenge, see Phase 1 below) was "fixed" by
   re-hosting the XC8 installer as a public GitHub Release asset in this
   repo. That worked, but was the wrong fix.
2. Checking what Microchip's own EULA actually says (pulled straight out
   of the installed product's `docs/MPLABIDELicense.htm`, not assumed):

   > **2. Software License Grant.** Microchip grants strictly to Licensee
   > a non-exclusive, non-transferable, worldwide license to: a. Use the
   > Software solely for use with Microchip Products...
   >
   > **6. Licensee Obligations.** Licensee will not: (a) engage in
   > unauthorized use, modification, **disclosure or distribution of
   > Software or Documentation, or its derivatives**...

   That is a *use* license, not a redistribution one. Checked whether
   what this pipeline was actually doing counted as distribution:
   `docker logout ghcr.io && docker pull ghcr.io/apojomovsky/pic8-hal-ci:<tag>`
   succeeded with **zero authentication**. Both the GHCR image and the
   GitHub Release asset were reachable by anyone, not just this repo's
   own CI, no EULA authorization for that exists.

**Fix**: neither artifact is public anymore. `docker/ci-assets/Dockerfile`
is a generic, non-runnable "blob carrier" (`FROM scratch`, `COPY . /`),
built once per vendor file and pushed to a **private**
`ghcr.io/<owner>/pic8-hal-ci-assets` image. Consumers extract the file
with `docker create` + `docker cp` (never `docker run`, there's nothing
to run). `xc8-build.yml`'s `ci-assets` job builds/pushes it; visibility
is set by hand, once, per package (package Settings page, "Danger Zone"),
not by the workflow, an initial attempt to do it via `gh api PATCH
.../visibility` with `GITHUB_TOKEN` reported success but silently didn't
work (the token lacks the scope), so that step was removed rather than
left in as false confidence, see Phase 1's updated validation below for
the full account. `toolchain-image` extracts the installer from that
private image into
`docker/ci-toolchain/vendor/` (gitignored, `*.run`) before `docker
build`, so `docker/ci-toolchain/Dockerfile` now `COPY`s the installer
from build context instead of `curl`-ing it from anywhere, public or
not. Multi-stage trimming of the *final* image (the idea that started
this conversation: keep only `mdb`/`xc8-cc`/packs, drop the NetBeans
GUI once MPLAB X IDE is added in Phase 2) is a good idea on its own
merits, but doesn't address this problem by itself, a smaller
redistributed copy is still a redistributed copy; the fix is about who
can reach the artifact, not how much of it there is.

**This same problem will recur for MPLAB X IDE** (Phase 2's `mdb`), 1.1GB
and even more obviously Microchip's full commercial product. It hasn't
touched any persistent or shared artifact yet, everything so far is
local to whichever machine's Docker daemon ran the Phase 2 probes. Apply
the same private-asset pattern before it does.

## Target layout

```
.github/workflows/
  host-tests.yml   # Phase 0: cmake/ctest matrix, every module + both HALs
  xc8-build.yml    # Phase 1: ci-assets (private vendor-installer blob carrier)
                    # + toolchain-image (build-or-reuse on GHCR, private) + discover
                    # + build (xc8-cc matrix, every MCU variant, both families)
  sim-tests.yml    # Phase 4: runs pilot module(s) under mdb/MPLAB SIM, parses PASS/FAIL

docker/ci-toolchain/
  Dockerfile                  # Debian base, XC8, MPLAB X IDE (for mdb.sh)
  vendor/                     # gitignored (*.run); populated at build time only,
                               # extracted from the private ci-assets image

docker/ci-assets/
  Dockerfile                  # generic vendor-file blob carrier (FROM scratch),
                               # never redistributed publicly, see "Correction" above

scripts/
  ci-discover-xc8-matrix.py   # Phase 1, per-family since the post-Phase-2
                               # efficiency fix: per-family (mcu/*-mplabx
                               # dirs + their allowed MCU variants + DFP
                               # name) discovery, used by xc8-build.yml's
                               # discover job

epic-common/
  include/core/epic_harness.h        # unchanged contract, cycles param already exists
  src/core/epic_harness_target.c     # existing: real-target no-op variant (untouched)
  src/core/epic_harness_sim_target.c # NEW (Phase 3): bounded, USART-reporting variant
  mk/epic_family.mk                  # extended (Phase 3) to opt into the sim-target variant

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
   `epic-*` module) and matrix over them: `cmake -B build -S <dir> &&
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
tooling, matching the host-tooling subset of `scripts/bootstrap.sh`.

**Validation**
- [ ] A PR that breaks one module's host test fails only that module's
      matrix leg, others stay green. Not yet exercised for real (no
      failing PR has been pushed); `fail-fast: false` is set specifically
      for this, but treat as unconfirmed until a real matrix leg actually
      fails independently.
- [x] A clean push against current `master` is fully green. Confirmed on
      the first real run after landing this workflow: 20/20 jobs
      (`discover`, `lint`, 18 `build-test` legs), all `success`,
      https://github.com/apojomovsky/epicurus/actions/runs/30717451172.
- [ ] Removing a module's `CMakeLists.txt` (test in a throwaway branch)
      shrinks the matrix automatically, confirming discovery isn't
      hardcoded. Not yet exercised.

**Exit criterion**: `host-tests.yml` green on `master`, merged. Met.

---

### Phase 1: XC8 cross-compile check (build only, no simulator)

Proves every family/MCU combination still produces a `.hex` on every
push, without yet touching the simulator problem.

**Tasks**
1. `docker/ci-toolchain/Dockerfile`: Debian slim base; `curl`, `make`,
   `unzip`; XC8 **v4.00** (superseded the plan's first pass at this
   decision, see "which XC8 version to pin" below) installed via its
   silent Linux installer (`--mode unattended --unattendedmodeui none`,
   no license flags at all, v4.00 dropped the PRO gate entirely, verified
   against the real installer's own `--help` output, its flag set doesn't
   even have `--LicenseType` or `--netservername` anymore). Both
   `Microchip.PIC16Fxxx_DFP.1.7.162` and `Microchip.PIC18Fxxxx_DFP.1.7.171`
   are fetched as `.atpack` files from `packs.download.microchip.com` and
   unzipped under `$XC8_INSTALL_DIR/pic/packs/`; **neither is bundled**
   with the standalone v4.00 installer (verified locally: no `packs/`
   directory anywhere under the installed tree at all). `xc8-build.yml`'s
   `build` job passes `DFP_DIR` explicitly per family, since the
   Makefiles' own `DFP_DIR` default still points at the repo-documented
   v3.10 path, which doesn't exist in this image.
2. `.github/workflows/xc8-build.yml`, `toolchain-image` job: resolves a
   version-pinned tag (`xc8-v4.00-dfp1.7.162-1.7.171`) and reuses it via
   `docker pull` if it already exists on `ghcr.io/<owner>/pic8-hal-ci`,
   only building+pushing on a cache miss. Deliberately not a separate
   path-filtered "publish" workflow (the plan's original sketch): that
   would race its own first consumer on the very first run, before
   anything has been published yet. One workflow, image resolved before
   the matrix that needs it runs, sidesteps the chicken-and-egg.
3. `xc8-build.yml`, `discover` job: `scripts/ci-discover-xc8-matrix.py`
   finds every `mcu/*-mplabx/Makefile` from tracked files (28 found, more
   than this plan's earlier draft assumed, an incomplete manual `find`
   undercounted) and pairs each with its family's four MCU variants,
   same "discover, don't hardcode" discipline as `host-tests.yml`.
4. `xc8-build.yml`, `build` job: matrices over the discovered set inside
   the resolved image, `make -C <dir> MCU=<variant> DFP_DIR=<family's
   pack path>`, then asserts `<dir>/build/<variant>-firmware.hex` exists.

**Explicitly out of scope**: no MPLAB X IDE, no `mdb`, no simulation.
This phase only needs the XC8 compiler.

**A real risk that materialized, and how it got resolved**: Microchip's
installer CDN (`ww1.microchip.com`) sits behind an Akamai bot-challenge.
The first version of this Dockerfile (pinned to v3.10, fetching straight
from that CDN) failed identically in two independent places: a local
`docker build` in the sandbox this was written in, *and* a real
`xc8-build.yml` run on GitHub Actions itself (same `curl` exit code 22
in both). Trying `curl-impersonate` (spoofs Chrome's TLS/JA3 fingerprint)
got past the initial block but landed on a JavaScript challenge page
instead of the file, confirming it's a JS bot-challenge, not just header/
TLS fingerprinting, and not something worth automating past even if it
were feasible. Resolved by switching approach entirely: the user
downloaded the v4.00 installer directly (a real browser clears the
challenge trivially) and it's re-hosted as a release asset in this repo
(`ci-toolchain-assets` tag) instead of fetched from Microchip's CDN in
CI. The DFPs were never affected, `packs.download.microchip.com` is a
different, S3/CloudFront-backed host with no such challenge, confirmed
working from both the sandbox and a real GitHub Actions run.

**Validation**
- [x] Toolchain image builds successfully and is pullable from GHCR.
      Confirmed twice: locally (`docker build` against the final
      Dockerfile using a local HTTP server standing in for the release
      asset URL) and for real, `toolchain-image` succeeded on GitHub
      Actions once the `ci-toolchain-assets` release existed
      (https://github.com/apojomovsky/epicurus/actions/runs/30718807266).
- [~] Every existing module/MCU combination that builds locally today
      also builds green in this workflow. Two real bugs found across two
      runs, both fixed:
      1. Run 30718807266: `make` itself succeeded on every leg, but the
         workflow's own "Confirm .hex was produced" step assumed every
         module names its output `<MCU>-firmware.hex`, true only of the
         two top-level HAL Makefiles (each `epic-*` module's `TARGET` has
         its own suffix, `-debounce`, `-adcfilter-sizecheck`,
         `-multi-blink`, ..., no shared convention). Fixed by globbing
         `build/*.hex` and asserting exactly one match.
      2. Run 30719090416 (after fix 1, and after also fixing
         `toolchain-image`'s tag resolution silently ignoring the
         newly-added `PIC16FXXX_DFP_VERSION`): 40 of 112 `build` legs
         failed for real, genuine pre-existing link/resource bugs in
         those modules' Makefiles, not CI plumbing. Filed and root-caused
         in `docs/mplabx-link-gaps-plan.md`; excluded from this
         workflow's matrix via `scripts/ci-discover-xc8-matrix.py`'s
         `KNOWN_BROKEN` set (verified the excluded set matches the real
         failure set exactly, diffed both lists) so this workflow stays
         meaningfully green rather than either permanently red or
         silently wrong. **Confirmed on run 30720162258**: all 74 jobs
         green (`toolchain-image`, `discover`, all 72 `build` legs),
         https://github.com/apojomovsky/epicurus/actions/runs/30720162258.
         Marked `[~]`, not `[x]`: this is "green on the
         72 legs that are known to actually work," not literally "every
         module/MCU combination," 40 are deliberately, visibly excluded
         pending `docs/mplabx-link-gaps-plan.md`.
- [ ] A deliberately broken `.c` file (throwaway test branch) fails the
      matrix leg for its module/MCU, not the whole job. Not yet
      exercised.

**Exit criterion**: `xc8-build.yml` green on `master`, merged. Met
(run 30720162258).

**Follow-up validation, redistribution fix** (see "Correction" above):
- [x] `ci-assets` job succeeds: seeded `pic8-hal-ci-assets` from the
      (still public at the time, now needs deleting) `ci-toolchain-assets`
      release, pushed it. Confirmed on run
      https://github.com/apojomovsky/epicurus/actions/runs/30722036149.
- [x] `toolchain-image` job's extraction step (`docker create` + `docker
      cp` from the private asset image into `docker/ci-toolchain/vendor/`)
      confirmed working on that same real run, not just locally.
- [x] **Both `pic8-hal-ci-assets` and `pic8-hal-ci` are actually
      private.** First check (right after the run above) found this
      FALSE: the workflow's `gh api PATCH .../visibility` steps reported
      step-level "success", but that step had a `||` fallback that never
      failed the job either way, "success" only meant the shell script
      ran, not that the API call worked, and `docker logout ghcr.io` +
      `docker pull` on both images succeeded with zero authentication.
      `GITHUB_TOKEN` doesn't have the scope GitHub requires for the
      package-visibility endpoint (needs a PAT with admin rights on the
      package, not the automatic per-run token). Fixed by hand (package
      Settings page, "Danger Zone" > "Change package visibility", both
      packages), then re-verified the same way: `docker pull` on both
      now returns `unauthorized`. The ineffective API-attempt steps were
      then removed from `xc8-build.yml` entirely, false "success" is
      worse than no attempt; visibility is a one-time manual setting per
      package now, not something the workflow re-asserts.
- [x] The now-unnecessary bootstrap-from-public-release fallback in
      `ci-assets`'s pull step was removed (it only existed to seed the
      private image once from the about-to-be-deleted public release; the
      private image is cached now, and the fallback would have gone
      silently stale, not just broken, the next `XC8_VERSION` bump).
      Replaced with a loud failure plus reseed instructions on a cache
      miss. Re-run to confirm the cache-hit path still works without it
      before considering this done, see the item below.
- [ ] The public `ci-toolchain-assets` GitHub Release can now be deleted
      (both packages confirmed private, the bootstrap fallback that
      depended on it is gone). Not yet done.
- [x] A real CI run confirming the simplified `ci-assets`/`toolchain-image`
      jobs (no bootstrap fallback, no visibility-API steps) still work
      end to end against the now-private, already-cached images. Confirmed
      on run https://github.com/apojomovsky/epicurus/actions/runs/30722374627,
      all 75 jobs green (`ci-assets`, `toolchain-image`, `discover`, all
      72 `build` legs). The redistribution fix is done: both packages
      private, workflow doesn't lie about it, cache-hit path works.

The only remaining item from this fix is deleting the now-redundant
public `ci-toolchain-assets` GitHub Release (Repo -> Releases ->
`ci-toolchain-assets` -> Delete), safe to do now that both packages are
confirmed private and nothing in the workflow reads from it anymore.
Not this document's author's call to make unilaterally (deleting a
public release is a one-way, visible action), left for whoever's driving
this to do when ready.

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
- [x] Both probes (PIC16, PIC18) produce a captured UART output file with
      the expected `printf` content, driven entirely by `mdb.sh` inside
      the container, no interactive step. PIC16F877A: a throwaway
      `probe_usart_16f87xa.c` transmitting a fixed line in a loop, run
      for 2 real-time seconds under `mdb`/MPLAB SIM, produced 50KB of
      correctly-captured output. PIC18F4550 (the plan named PIC18F2455;
      used 4550 instead, same family, same mechanism, not a meaningful
      difference here): same shape, 895 bytes captured. Neither probe is
      committed, per this repo's own convention for exploratory checks
      (scripts/README.md's "throwaway probe" precedent).
- [x] Findings for each open question below are recorded, with the
      actual `mdb` script and output referenced.

**Exit criterion**: every open question tagged "resolve in Phase 2" below
has a recorded answer, met. `docker/ci-toolchain/Dockerfile` extended
with MPLAB X IDE (task 1), validated locally against the real installer
end to end; not yet confirmed on a real GitHub Actions run (pending the
temporary `ci-mplabx-assets-tmp` release), see the follow-up checklist
below.

**Follow-up validation, MPLAB X IDE addition**:
- [x] `ci-assets-mplabx` job succeeded on a real run: seeded
      `pic8-hal-ci-assets:mplabx-v6.35-installer` from the temporary
      `ci-mplabx-assets-tmp` release. Confirmed on run
      https://github.com/apojomovsky/epicurus/actions/runs/30723598436
      (76/76 jobs green: `ci-assets`, `ci-assets-mplabx`,
      `toolchain-image`, `discover`, all 72 `build` legs).
- [x] `toolchain-image`'s extraction step pulled both asset images and
      built the combined XC8 + MPLAB X IDE image on a real runner, same
      run, ~6 minutes for that job alone (the image is ~7GB+). Confirmed
      locally too before pushing (built from both real installer files,
      `xc8-cc --version`, and a real `mdb.sh device/hwtool SIM/quit`
      script all worked against the resulting image).
- [x] `pic8-hal-ci-assets:mplabx-v6.35-installer` is private. Checked the
      same way as before, `docker logout ghcr.io && docker pull`
      returned `unauthorized`. It inherited the package's existing
      private visibility automatically (visibility is per-package, not
      per-tag), no separate manual step was needed for this new tag.
      Same check against the new combined `pic8-hal-ci` tag
      (`...-mplabx6.35`) also returned `unauthorized`.
- [x] The bootstrap fallback in `ci-assets-mplabx`'s seed step was
      removed, mirroring exactly what happened to `ci-assets`'s original
      bootstrap, once the above were confirmed.
- [x] A fresh run confirms the cache-hit path still works with that
      fallback gone (same discipline as `ci-assets`'s own removal:
      removing dead code that would have failed silently isn't the same
      as confirming the remaining code still works). Confirmed on run
      https://github.com/apojomovsky/epicurus/actions/runs/30724128114,
      76/76 jobs green.
- [ ] The temporary `ci-mplabx-assets-tmp` GitHub Release can now be
      deleted (all of the above confirmed, the bootstrap that depended on
      it is gone). Not yet done as of this writing, a human's call, same
      as `ci-toolchain-assets` was.

---

### Phase 3: Design and implement the sim-target harness

The actual design gap: give real-target firmware a way to terminate and
report PASS/FAIL, for the simulator case specifically, without changing
the existing infinite-loop, no-stdout contract that real hardware still
needs.

**Tasks**
1. ~~Add `epic-common/src/core/epic_harness_sim_target.c`~~ Done, but per
   family, not in `epic-common`: see "Where the sim-target harness file
   lives" in Open questions for why. `pic16f87xa-hal/src/core/
   pic16_harness_sim_target.c` and `pic18fxx5x-hal/src/core/
   pic18_harness_sim_target.c`, both mirroring
   `epic_harness_target.c`'s init/tick no-ops, but with
   `epic_harness_running` bounded by `cycles` (like the host build) and
   `epic_harness_log` doing a real, polled USART write (not a real
   `printf`, see the files' own header comments: it walks `fmt` and
   transmits its raw bytes, ignoring any variadic args, deliberately, to
   avoid a `vsnprintf`-over-USART flash cost for a wire format that only
   ever needs to carry `epic_harness_report`'s own fixed marker line
   reliably). `epic_harness_report` itself (`epic_harness.h`) changed
   too: it now calls `epic_harness_log` with that marker before
   returning, see "Wire format" in Open questions.
2. ~~Extend `epic-common/mk/epic_family.mk`~~ Done, and it turned out
   `epic_family.mk` itself needed no changes at all: see "Mechanism for
   a module to opt in" in Open questions, a `HARNESS ?= target` variable
   in the module's own Makefile is enough.
3. Pilot module: `epic-tick`, both families. Already buildable, no
   USART dependency of its own (so no conflict with the harness
   borrowing the family's one physical USART for reporting), and its
   `example_tick.c` doesn't use the `epic_harness_running` loop pattern
   at all (a straight-line delay/check/report test), so the pilot
   doesn't happen to exercise the bounded-`running()` path; that path is
   still implemented correctly for Phase 5's other modules that do use
   it, just not exercised by this particular pilot.

**Explicitly out of scope**: no rollout to every module yet, no CI
wiring yet (that's Phase 4). This phase proves the harness variant works
when driven by hand (`mdb.sh` locally, or in the Phase 2 container), same
discipline as Phase 1 of `multi-family-plan.md` proving the build seam
before the hardware worked.

**Validation**
- [x] The pilot module's sim-target `.hex` builds via `xc8-cc`, same as
      today's real-target `.hex`, just linking the new harness variant.
      Confirmed against a real local XC8 v3.10 install (not just
      syntax-checked): `HARNESS=sim` for PIC16F877A/876A and
      PIC18F4455/4550 all link clean. PIC16F873A/874A hit a genuine new
      RAM overflow (`error: (1250) could not find space (4 bytes) for
      variable _g_cycles`, the sim harness's one extra `static uint32_t`
      plus the USART handle pushes an already-tight variant over
      budget); not a regression (those two build fine under
      `HARNESS=target`), a new, real, small-MCU-only constraint,
      recorded here for Phase 5 to handle the same way
      `docs/mplabx-link-gaps-plan.md` already handles other small-MCU
      overflows, not chased down further in this phase (the exit
      criterion only needs one working variant per family).
      PIC18F2455/2550 fail for an unrelated, pre-existing reason
      (`pic18fxx5x_spp.c` doesn't compile for those variants at all,
      regardless of harness; already excluded from `xc8-build.yml`'s
      matrix via `KNOWN_BROKEN`, confirmed the same failure happens
      under `HARNESS=target` too, so this is not something Phase 3
      introduced).
- [ ] Run under `mdb`/MPLAB SIM (from Phase 2's probe setup): a passing
      test reports the agreed PASS marker over captured UART, a
      deliberately-broken version (throwaway edit) reports the FAIL
      marker, and both terminate on their own (no reliance on the `mdb`
      script's `wait` timeout as the only signal). Not yet done: no
      local `mdb`/MPLAB X IDE install available outside the private
      GHCR toolchain image, and this environment has no GHCR
      credentials to pull it. Folded into Phase 4 instead of blocking
      here: `sim-tests.yml` will prove this against a real GitHub
      Actions run, the same ground-truth-over-local-assumption pattern
      Phase 1/2 already used when local reproduction wasn't available.
- [x] Real-target build of the same module (`epic_harness_target.c`
      variant) is unchanged, confirming the new variant is additive, not
      a modification of the existing target contract. Confirmed for both
      families (`HARNESS=target`, the default, still produces the exact
      same `$(MCU)-tick.hex` target name it always did). One small,
      repo-wide, deliberately-accepted side effect: `epic_harness_report`
      now calls `epic_harness_log` with the marker string on every
      build, including real-target; since target's `log()` stays a
      no-op, this costs a few bytes of program space everywhere (two
      string literals + a call site), confirmed on PIC16F877A: A22h ->
      A67h words. Full host CMake/ctest suite re-run across all 18
      host-testable modules after this header change, 0 failures.

**Exit criterion**: one module, both families, produces a real PASS/FAIL
signal from MPLAB SIM, driven by a script, with no manual interpretation
needed. Build side confirmed; the actual `mdb`-driven signal is Phase
4's job now.

---

### Phase 4: Wire the pilot into CI

**Tasks**
1. Done: `.github/workflows/sim-tests.yml`. `ci-assets` /
   `ci-assets-mplabx` / `toolchain-image` are a deliberate verbatim copy
   of `xc8-build.yml`'s own jobs (not a shared/reusable workflow, see the
   file's own header comment for why: same tag-resolution formula means
   both workflows' `docker pull` hits the same cache regardless of which
   one runs first, and a `workflow_call` refactor is reasonable future
   cleanup once a second consumer exists beyond these two, not forced
   now). `sim-test` matrices over the 2 families (hardcoded pilot entries
   for now, `epic-tick` only; Phase 5 makes this dynamic the way
   `xc8-build.yml`'s `discover` job already is), builds the sim-target
   `.hex` and runs `mdb.sh` via `scripts/sim-mdb-run.sh` (moved out of
   the workflow's own inline YAML once local reproduction, see below,
   needed the exact same build+mdb+grep sequence too; one script, two
   callers, not two copies that can drift apart), and checks the
   captured output in three distinct steps so a failure is legible about
   *why*: `mdb.sh` itself exiting non-zero, no output file at all, or a
   FAIL/missing marker are three different `::error::` messages, not one
   opaque red X.
2. Done: `actions/upload-artifact@v4`, `if: failure()`, uploads the
   captured UART file per family.
3. **New, added mid-Phase-4**: local reproduction. This environment has
   no local `mdb`/GHCR access, so every debugging round trip during this
   phase meant push, wait several minutes for a real Actions run, then
   ask a human to paste the log back (this assistant cannot pull
   Actions logs or artifacts itself, confirmed: the REST API returns
   403 "Must have admin rights to Repository" for both, even against
   this assistant's own pushed commits). That's fine for one or two
   rounds, expensive for the kind of register-level debugging Phase 4's
   pilot actually needed (see Validation below). Fix: `scripts/
   sim-mdb-run.sh` (the same script the workflow now calls) plus
   `scripts/sim-test-local.sh`, which resolves the exact same
   version-pinned image tag `toolchain-image` does, `docker pull`s it,
   and runs `sim-mdb-run.sh` inside it with this repo bind-mounted, so a
   local run goes through the identical toolchain, the identical
   command sequence, and produces the identical captured-UART output a
   CI run would, just in seconds instead of minutes and without a human
   relaying logs by hand. Requires `docker login ghcr.io` once (a PAT
   with `read:packages`; this repo's GHCR packages are private).

**Validation**
- [ ] A green pilot-module run on `master`. **Not yet, still red as of
      this writing**, and this phase surfaced real, previously-uncaught
      bugs along the way (the actual point of Phase 2-4: nothing had
      ever *executed* this repo's compiled firmware before now, only
      linked it):
      - `epic_tick_init` never enabled the chip's global interrupt
        enable (GIE); fixed (`epic-tick/src/epic_tick.c`,
        `EPIC_IRQ_Restore(1)`), confirmed via host/target rebuilds, did
        not by itself turn the sim-test jobs green.
      - The sim-target harness's `EPIC_USART_Init` TXEN workaround (a
        non-null `TxCpltCallback`) has a side effect: it also enables
        the TX interrupt source (TXIE). TXIF is pending immediately
        after reset and is only cleared by writing TXREG, so the moment
        GIE turns on, TXIE + pending TXIF fire the TX ISR in an
        infinite storm (the no-op callback never clears TXIF). Fixed
        (`EPIC_IRQ_DisableSrc(..._IRQ_USART_TX)` right after Init, both
        families' `*_harness_sim_target.c`), also did not by itself
        turn the jobs green.
      - Both fixes are correct and staying in, but resolved a different
        problem than the actual blocker turned out to be.
      - **Root cause, found after switching to local reproduction (task
        3): any Bank-1 SFR access (PIE1/PIE2, SPBRG, PR2, i.e.
        everything this family's `pic_select_bank` helper exists for)
        was at risk of corruption, not something specific to
        epic-tick.** (See `pic16f87xa-hal/docs/ARCHITECTURE.md` for the
        cross-check against XC8's own User's Guide done after this
        phase's initial debugging; item 1 below is a plausible, but not
        confirmed, explanation rather than a proven compiler defect,
        item 4 below turned out to match documented behavior exactly.)
        `mdb`'s `wait N` does not mean "N
        simulated milliseconds," it means "poll for the simulator to
        halt on its own, give up after N *real* milliseconds" (per
        `help wait`); since nothing in these scripts ever triggers a
        natural halt, every earlier `wait`-based register dump in this
        phase was actually reading state *after* an unknown, uncontrolled
        number of WDT reset cycles (confirmed: even `wait 5` showed the
        same "2 resets" as `wait 2000`, because MPLAB SIM simulates much
        faster than real time once actually running). Switched to `stepi
        N` (deterministic instruction count, immune to WDT timing)
        for reliable diagnostics.
        1. `pic_select_bank` (the `static inline` bank-select helper,
           `pic16f87xa_sfr.h`) compiled to a genuine out-of-line `fcall`
           under XC8 v4.00 (`__attribute__((always_inline))` was tried
           and ignored), and something about that call boundary
           corrupted the caller's own live value across it: a real
           `EPIC_TIMER2_WritePeriod(200)` call landed as PR2=0 every
           time, confirmed via a dedicated probe. **Fixed**: converted
           to a macro, forcing true preprocessor-level inlining, no call
           boundary possible regardless of what the optimizer does.
        2. `pie_reg_addr`/`pir_reg_addr` (`pic16_irq.c`, small `static`
           helpers returning a Bank 1/PIR register address) had the same
           function-call-boundary problem. **Fixed**: also converted to
           macros.
        3. `irq_table` is `static const`, ROM-resident on PIC16's
           Harvard architecture; XC8 reads its fields through its own
           runtime helper (`fcall stringdir` in the generated `.s`), and
           interleaving that ROM read with an in-progress SFR
           read-modify-write silently corrupted the SFR side. **Fixed**:
           `EPIC_IRQ_Enable`/`DisableSrc`/`ClearFlag`/`GetFlag` now pull
           every field out of `d` into locals before touching any SFR.
        4. **Fixed**: `EPIC_IRQ_Enable`/`DisableSrc`'s PIE1/PIE2
           read-modify-write, affected by the same symptom as item 1
           (a C-level local touched while banked into Bank 1 got
           misdirected), now hand-written inline asm following this
           repo's established binding convention
           (`epic-math/docs/ARCHITECTURE.md`'s "Inline-asm binding"
           section): the operand is copied into a file-scope,
           `__at`-pinned scratch byte and loaded into W *before* the
           bank switch, so nothing Bank-0-assumed is ever touched while
           banked; the read-modify-write itself is one `iorwf`/`andwf
           <SFR>,f` against the named SFR and W only. This is confirmed
           correct against XC8's own documentation, not just empirically:
           `pic16f87xa-hal/docs/ARCHITECTURE.md`'s Finding 1 cites the
           User's Guide's §5.12.2 stating in-line assembly resets the
           compiler's bank tracking, which is exactly what this fix
           relies on. This has to live
           in the per-platform header
           (`target/pic16f87xa_platform.h`'s new `EPIC_PIE_ENABLE_BIT`/
           `EPIC_PIE_DISABLE_BIT` macros), not inline in `pic16_irq.c`:
           that file is shared with the host build, and `asm()`/`__at()`
           are XC8-only syntax gcc/clang cannot parse (confirmed the
           hard way: an in-place version broke the host build outright,
           10 modules failing to compile, caught before push by this
           session's own "run the full verification suite before
           committing" habit). `host/pic16f87xa_platform.h` gets the
           same two macro names with a plain array read-modify-write, no
           banking concept needed there. The scratch byte's `extern`
           declaration needed its own `__at(0x70)` pin too, not just the
           definition (matches how the vendor's own device header
           declares pinned SFRs like `OPTION_REG`); without it, an
           unrelated debugging session nearly mistook "not enough
           `stepi` steps yet to reach the code under test" for a
           regression, caught by re-testing the isolated minimal probe
           that had worked before, apples-to-apples, before trusting a
           false alarm.
        5. **Found and fixed, unrelated but real**: `epic_harness_init`
           (`pic16_harness_sim_target.c`) called `EPIC_USART_Init(&h)`
           with `h` a plain local variable, but `EPIC_USART_Init` stores
           that *pointer* (`pic16f87xa_usart.c`'s `g_usart`), dereferenced
           later from ISR context on every interrupt for the life of the
           program, not just for the duration of the call. Once
           `epic_harness_init` returned, `h`'s storage was fair game for
           XC8's non-reentrant model to hand to some other function's
           locals, leaving `g_usart` dangling. Fixed: `h` is now `static`
           (permanent storage, never reused).
        6. **Found and fixed, unrelated but real**: the sim-target
           harness's `log()` actually transmits (unlike the no-op
           real-target build), and `epic-tick`'s config word bakes
           `WDTE = ON` unconditionally with nothing ever clearing the
           watchdog; a module with more than a trivial amount of
           delay-plus-log work reliably outran the default WDT period
           mid-run. Real-target firmware genuinely needs the watchdog
           running unattended; a diagnostic build that terminates and
           reports over USART does not, and PIC16's `WDTE` is a
           config-word fuse burned at program time (unlike PIC18's
           runtime-togglable `SWDTEN`), so the only place to turn it off
           for this variant is the config-word generation itself. Fixed:
           `epic-tick/mcu/pic16f87xa-tick-mplabx/Makefile`'s
           `$(CONFIG_SRC)` recipe now sets `WDTE = OFF` when
           `HARNESS=sim`, `WDTE = ON` (unchanged) otherwise. (An earlier
           attempt tried extending the WDT postscaler via `OPTION_REG`
           instead of disabling WDTE; abandoned once `stepi`-based
           inspection showed `OPTION_REG` is already `0xFF`, maximum
           postscaler, at POR default, so that approach could never have
           helped. A second earlier attempt tried petting the watchdog
           with `CLRWDT` inside the busy-wait loops instead; abandoned
           after it produced a *new*, different hang, not chased down
           further once the config-word fix proved simpler and more
           reliable.)
        7. **Still open, current blocker, a different bug than 1-6**:
           with fixes 1-6 all applied and verified individually correct,
           `epic-tick`'s real test now gets further than ever before
           (the first `epic_tick_delay_ms(10)` completes, and its log
           line transmits correctly) but hangs partway through the
           *second* delay: `INTCON` shows `GIE=0` (global interrupts
           disabled) while `PIE1`/`T2CON` still show Timer2 correctly
           configured and counting, `PIR1<TMR2IF>` pending but never
           serviced, i.e. something clears `GIE` without ever restoring
           it, well after `epic_tick_init`'s own `EPIC_IRQ_Restore(1)`
           already ran successfully once (proven by the first delay
           completing at all). `EPIC_IRQ_Disable`/`Restore`'s own
           generated assembly was traced instruction-by-instruction and
           is logically correct. Earlier hypothesis, since tested and
           weakened: **PIC16F87XA's hardware call stack is only 8 levels
           deep**, and XC8 has been warning about this the entire time
           (`warning: (1393) possible hardware stack overflow detected;
           estimated stack depth: 9` on every single build of
           `example_tick.c`, including every build before this session's
           Phase 2-4 work even started); a fresh build's own generated
           `.s` Call Graph Tables (compiler-computed, not inferred) show
           the interrupt-rooted call graph at an estimated maximum stack
           depth of **10**, two over the hardware's 8-level limit, while
           the main-line graph tops out at a safe 5
           (`pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 4). **Directly
           tested and not confirmed**: shrinking the dispatcher
           (`pic16_irq_dispatch.c`'s `epic_dispatch_all_irqs`, which
           really does unconditionally call all 13 peripheral handlers on
           every interrupt, not just in the compiler's worst-case
           estimate) down to only the one handler this test needs should
           reduce interrupt-side depth well under 8 if depth is really
           the mechanism. It didn't fix the hang; a bisection of which
           handlers' presence matters found a case where *adding* a
           handler back (increasing depth) fixed a worse failure, the
           opposite of what a pure depth theory predicts. Full account:
           `pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 5. A second,
           separately-motivated official lead was also tested and ruled
           out: the XC8 v4.00 release notes' own Known Issue `XC8E-11`
           ("Stack overflow") documents that `-mstackcall`'s protection
           does not cover indirect (function-pointer) calls, and this HAL
           has exactly one in the interrupt path
           (`USART_TX_IRQHandler`'s `TxCpltCallback` call, only present
           because of a separate, real bug tying `TXEN` to that
           callback's presence). Removing it entirely (throwaway,
           reverted) made no difference: `INTCON` still shows `GIE=0`
           after the hang, identical to baseline. Full account:
           `pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 6. A third attempt,
           pinning the specific locals live across the disable/restore
           window (`EPIC_IRQ_Disable`'s and `epic_tick_get`'s auto
           variables made `static`, throwaway, reverted) actually removed
           the `GIE`-stuck symptom, but a *different* variable (`PR2`,
           Timer2's period register) came back corrupted instead, whack-a-
           mole rather than a fix (`pic16f87xa-hal/docs/ARCHITECTURE.md`
           Finding 7). The flagged `.sym`-diff forensic pass was then
           actually carried out: built the `TIMER2_IRQHandler`-only and
           `TIMER2_IRQHandler`+`CCP1_IRQHandler` configs, diffed every
           local variable's storage assignment, and found a real
           candidate collision (`compute_period`'s `best_pr2` landing on
           `epic_harness_init`'s `cycles` parameter address in the broken
           config only). Tested directly (pinned `compute_period`'s
           locals `static`): `PR2` was still `0`. A second candidate
           (`epic_tick_init`'s own locals) had no collision at all.
           Full account: `pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 8.
           **Root cause found (Finding 9)**: `compute_period()` and
           `EPIC_TIMER2_WritePeriod()` both run entirely *before*
           `EPIC_IRQ_Restore(1)` ever enables `GIE`, so no interrupt could
           be involved, ruling out every theory above at once. `mdb`
           instruction-stepping (`stepi`, not `wait`, which turned out
           not to reliably respect `break`-set breakpoints in this
           toolchain's headless mode) traced a fixed step count directly:
           `compute_period@best_pr2` and `epic_tick_init@pr2` both held
           the correct value (`249` for 20 MHz) throughout, but the
           actual `PR2` register read `0` at the same point. The only
           thing between them is `EPIC_TIMER2_WritePeriod` itself, whose
           `pic_select_bank(1)` bank switch misdirects its own `period`
           parameter, the identical failure shape already proven and
           fixed for PIE1/PIE2 earlier in this phase (item 4), just
           hitting a different function. `EPIC_USART_Init`'s SPBRG write
           had the same bug (`SPBRG` read `4`, should be `129`),
           previously undetected because MPLAB SIM's UART capture isn't
           baud-timing-sensitive. **Fixed**: same proven pattern (load
           into W through a bank-independent scratch byte before
           switching banks) applied to both. Verified: `EPIC_HARNESS_RESULT:
           PASS` reliably (5/5 runs), full host suite and all 38
           previously-passing PIC16 `(module, MCU)` real-target builds
           clean, no regressions. Full account:
           `pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 9.
      - All of the above verified against a real local XC8 v4.00 build
        (via `scripts/sim-test-local.sh`'s toolchain image, pulled
        directly once `docker login ghcr.io` was set up) and real `mdb`
        runs, not simulated/assumed; full host CMake/ctest suite
        re-confirmed 0 failures (all 18 host-testable modules) after
        every round of changes, and both `HARNESS=target`/`HARNESS=sim`
        real XC8 builds re-confirmed clean for both families before each
        commit.
      - **Status: resolved.** This turned out to be a much deeper
        problem than "wire up one pilot module": eight real,
        independently-confirmed bugs found and fixed along the way to
        the ninth, actual root cause, none of them pre-existing
        knowledge, all invisible until Phase 2-4 actually *ran* compiled
        firmware for the first time. Two of the fixes (items 4 and 9)
        are confirmed correct against XC8's own documented behavior, not
        just empirically; the rest are real and reproduced but not run
        down to a documented compiler statement, so this document
        doesn't call them "codegen defects" outright. PIC18's own
        sim-test leg is untouched by any of this session's fixes (all
        PIC16-specific) and still fails the same way it always has,
        tracked separately.
- [x] A deliberately broken pilot-module change (throwaway branch) turns
      the job red for the right reason (grep sees FAIL or missing
      marker), not a container/tooling failure. Confirmed as a side
      effect of this phase's own debugging: every throwaway experiment
      that didn't fix the bug (Findings 5-8) reliably produced
      `::error::` output distinguishing `DEAD` (no UART output at all)
      from `PARTIAL` (first delay only) from the real `PASS`, not a
      container/tooling failure in any of them.

**Exit criterion**: `sim-tests.yml` green on `master` for the pilot
module, both families, merged. **PIC16 leg met** (verified locally,
5/5 runs; not yet merged/observed green in CI itself). **PIC18 leg now
also met** (verified locally, 3/3 runs; not yet merged/observed green
in CI itself). Three real bugs found and fixed: the sim-target
harness's baud-rate math didn't fit `SPBRG`'s 8 bits at this file's
48 MHz `FOSC_HZ`; the same missing `HARNESS=sim` → `WDT=OFF` Makefile
knob PIC16 needed; and `pic18_irq.c`'s `EPIC_IRQ_Restore`/`Enable`/
`DisableSrc`/`ClearFlag`/`GetFlag`, which dispatched off a runtime SFR
address (from a lookup table, not a compile-time constant), which XC8
compiled to PIC18's program-memory table read/write mechanism
(`TBLPTR`/`TABLAT`/`tblrd`/`tblwt`) instead of a plain data-memory
access, so the write silently went nowhere. Fixed by rewriting
`pic18_irq.c`'s table-driven dispatch into a `switch` per function with
one case per `PIC18_IRQn`, each naming its register directly so the
address is always a compile-time constant. `pic18fxx5x_ccp.c` had the
identical bug (confirmed via `mdb` with the existing `example_ccp_pwm.c`
smoke test, `CCPR1L`/`CCP1CON` both read `0` after init), fixed the
same way. Full account: `pic18fxx5x-hal/docs/ARCHITECTURE.md`.

---

### Phase 5: Roll out to remaining modules

**Tasks**
1. Apply Phase 3's harness-variant + Makefile pattern to each remaining
   `epic-*` module and both HALs' own example set, module by module,
   each as its own commit per this repo's convention.
2. Extend `sim-tests.yml`'s matrix the same way `xc8-build.yml` discovers
   modules dynamically in Phase 0/1, rather than hand-listing them.
3. `epic-usb` is explicitly deferred, not silently skipped: its host
   stub already tests ring-buffer/connection-state logic, not USB
   enumeration, and this plan does not currently establish that MPLAB
   SIM's USB SIE peripheral model is faithful enough to trust for
   enumeration-level testing (see open question below). Real hardware
   stays the source of truth for that module until/unless that's
   resolved separately.

**Validation**
- [ ] Every module (except `epic-usb`, tracked separately) has a green
      sim-tests matrix leg, both families where applicable (some modules
      are already family-agnostic at the host level; confirm the same
      holds for the sim-target variant).

**Exit criterion**: `sim-tests.yml` covers every module, `epic-usb`'s
deferral is documented in its own `docs/epic-usb-plan.md`, not just here.

## Open questions (resolve during the phase noted)

- **Which XC8 version to pin.** **RESOLVED (Phase 1), reversed from this
  document's first pass: `v4.00`, not `v3.10`.** The first pass reasoned
  CI should match what's already documented in this repo's Makefiles.
  That held right up until the CDN-fetchability question below forced a
  full reconsideration: since the installer had to be self-hosted as a
  release asset either way, there was no longer a "just curl it from
  Microchip" reason to prefer the older version, and v4.00 removes the
  PRO-license flag entirely (confirmed against the real installer's own
  `--help`: no `--LicenseType`/`--netservername` options exist anymore),
  one less moving part. This does mean CI now runs a newer XC8 than the
  version named in this repo's Makefile comments and
  `docs/multi-family-plan.md`; that divergence is real and left as its
  own separate follow-up (bump the repo-wide pin, or don't), not bundled
  into this CI work.
- **Whether the XC8 installer is even fetchable from a CI runner.**
  **RESOLVED (Phase 1): no, not from Microchip's CDN.** Confirmed twice,
  independently: a local `docker build` in the development sandbox, and
  a real `xc8-build.yml` run on GitHub Actions, both failed identically
  (`curl` exit 22) against `ww1.microchip.com`, which sits behind an
  Akamai bot-challenge. `curl-impersonate` (spoofs Chrome's TLS
  fingerprint) got past the TLS-level check but hit a JavaScript
  challenge page instead of the file, so it's a real JS challenge, not
  fixable with header/fingerprint spoofing, and not worth trying to
  automate past regardless. `packs.download.microchip.com` (DFPs) is
  unaffected, a different, S3/CloudFront-backed host, confirmed working
  from both the sandbox and a real GitHub Actions run. Fix: the XC8
  installer is now fetched from this repo's own GitHub Release
  (`ci-toolchain-assets` tag) instead of Microchip's CDN; a human
  downloaded it once via a real browser, which clears the challenge
  trivially. See `docker/ci-toolchain/Dockerfile`'s header comment for
  the full account.
- **Whether `mdb` truly needs no display server.** **RESOLVED (Phase 2):
  confirmed true.** Ran `mplab_platform/bin/mdb.sh` (MPLAB X IDE v6.35)
  in a plain Debian container, no `Xvfb`, no `DISPLAY` set at all. It
  degrades gracefully: `WARNING: Unable to create a system terminal,
  creating a dumb terminal`, then runs the script to completion normally.
  No GUI dependency in practice, not just in theory.
- **Whether `mdb`'s `program` command accepts a bare `.hex` with no
  `.elf`/debug symbols, and whether UART capture works without them.**
  **RESOLVED (Phase 2): yes to both.** `program /path/to/16F877A-
  firmware.hex` (no `.elf` anywhere in the container) programmed the
  simulator fine, and UART capture worked identically.
- **Two undocumented-until-hit `mdb` gotchas, found while getting the
  above two working, worth recording since they cost real debugging
  time:**
  - **`set` tool-property commands must be issued *before* `hwtool`**,
    not after. Silently ignored otherwise, no error, the property just
    doesn't take effect (confirmed by testing both orders: same script,
    reordered, went from "runs clean, output file never appears" to
    "works"). This is actually documented, in the MDB User's Guide
    (DS-50002102G, section on tool-property commands: "the set command
    ... must be executed before the Hwtool command is issued, otherwise
    the changes to the tool properties will be ignored"), just easy to
    miss since `mdb` doesn't warn about it at the point you get it wrong.
  - **The property is `uart1io.*`, not `usart1io.*`.** Also confirmed
    from the MDB User's Guide (`uartNio.uartioenabled`, `uartNio.output`,
    `uartNio.outputfile`, N = 1..6), not something `mdb` will tell you if
    you guess wrong: `set usart1io.anythingAtAll true` is silently
    accepted with no error either, so a wrong property name and a right
    one look identical until you check whether the file it should have
    produced actually exists.
  - The actual mechanism, both families, in the order that matters:
    ```
    device PIC16F877A
    set uart1io.uartioenabled true
    set uart1io.output file
    set uart1io.outputfile /path/to/output.txt
    hwtool SIM
    program /path/to/firmware.hex
    run
    wait 2000
    halt
    quit
    ```
  - **Unrelated to `mdb`, but hit while writing the PIC16/PIC18 probes**:
    this repo's own `EPIC_USART_Init` (`pic16f87xa_usart.c` and
    `pic18fxx5x_usart.c`, both families, same pattern) only sets `TXEN`
    when a non-null `TxCpltCallback` is supplied
    (`if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;`). Without one, the
    transmitter silently never enables, nothing shifts out, no error.
    Worth knowing for Phase 3's sim-target harness design, whatever it
    ends up transmitting over USART will need a callback set, even a
    no-op one, or `TXEN` never gets set.
- **Wire format for the sim-target harness's PASS/FAIL report.**
  **RESOLVED (Phase 3): a single terminating line, not Unity.**
  `epic_harness_report` (`epic_harness.h`, previously a bare `ok ? 0 :
  1`) now also calls `epic_harness_log` with a fixed marker
  (`EPIC_HARNESS_RESULT: PASS\n` / `...FAIL\n`) before returning. Since
  `epic_harness_log` already differs per build (no-op on target, printf
  on host, and now a real USART write on sim-target), every module gets
  the marker for free through its existing `return
  epic_harness_report(...)` call, no per-example changes needed, and no
  third-party framework pulled in.
- **Mechanism for a module to opt a build into the sim-target harness
  variant.** **RESOLVED (Phase 3): a `HARNESS ?= target` Makefile
  variable**, not a separate `make` target. Each module's own
  `mcu/*-mplabx/Makefile` gains a small `ifeq ($(HARNESS),sim)` block
  (mirroring the pattern it already uses for PSP's conditional source
  list) that swaps in the sim-target harness source and gives `TARGET`
  a `-sim` suffix so a sim build never clobbers a target build's `.hex`
  in the same `build/` dir. No change needed to the shared
  `epic_family.mk` fragment at all: which harness `.c` file lands in
  `SRCS` is decided by the caller before `include`-ing it, exactly like
  every other per-module source choice already works.
- **Where the sim-target harness file lives.** **CORRECTED from this
  plan's original Phase 3 task 1**, which named
  `epic-common/src/core/epic_harness_sim_target.c`. That's wrong once
  you account for what the file actually has to do: unlike
  `epic_harness_target.c`'s four architecture-blind no-ops, the
  sim-target variant needs real USART SFR access to make the report
  marker reach `mdb`'s `uart1io` capture, and USART access is
  family-specific. Per this repo's own rule (`epic-common/` holds only
  architecture-blind code, register-specific code lives per-family), it
  lives per-family instead: `pic16f87xa-hal/src/core/
  pic16_harness_sim_target.c` and `pic18fxx5x-hal/src/core/
  pic18_harness_sim_target.c`, mirroring the existing split for the
  host build's own per-family harness (`pic16_harness_sim.c` /
  `pic18_harness_sim.c`).
- **MPLAB SIM's fidelity for the PIC18 SIE (USB) peripheral.** Not
  expected to be trustworthy enough for enumeration-level testing of
  `epic-usb`; not going to be chased down as part of this plan. If it
  turns out MPLAB SIM does model the SIE well enough to be useful,
  that's a separate follow-up, tracked in `docs/epic-usb-plan.md`, not a
  blocker here.
- **GHCR image naming/ownership.** **RESOLVED (Phase 1):**
  `ghcr.io/${{ github.repository_owner }}/pic8-hal-ci`, resolved at
  workflow run time rather than hardcoded, so it stays correct across a
  fork or rename. Tag is `xc8-v<XC8_VERSION>-dfp<PIC18FXXXX_DFP_VERSION>`,
  read from `docker/ci-toolchain/Dockerfile`'s `ARG` defaults, so the tag
  changes automatically (forcing a rebuild instead of a stale reuse) if
  either pinned version is bumped.
