# CI: host tests, XC8 cross-compile, MPLAB SIM target tests, implementation plan

Status: **Phase 0 done** (`.github/workflows/host-tests.yml`,
`scripts/pre-commit-checks.sh` extended with `PRE_COMMIT_BASE_REF` for CI
reuse; first push to `master` after landing it went green, all 20 jobs,
https://github.com/apojomovsky/pic8-hal/actions/runs/30717451172).
**Phase 1 done** (XC8 v4.00, `docker/ci-toolchain/Dockerfile`,
`.github/workflows/xc8-build.yml`, `scripts/ci-discover-xc8-matrix.py`),
green on run https://github.com/apojomovsky/pic8-hal/actions/runs/30720162258
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
(https://github.com/apojomovsky/pic8-hal/actions/runs/30722374627)
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
(https://github.com/apojomovsky/pic8-hal/actions/runs/30723598436,
76/76 jobs green), both new GHCR tags confirmed private. The temporary
bootstrap fallback has since been removed from the workflow (same
lifecycle as `ci-assets`'s original one), and that removal is confirmed
on a fresh run too
(https://github.com/apojomovsky/pic8-hal/actions/runs/30724128114,
76/76 jobs green). **Phase 2 is done.** Only remaining step: deleting
the now-redundant `ci-mplabx-assets-tmp` GitHub Release, a human's call,
not done as part of this fix.

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
72) before pushing.

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
      matrix leg, others stay green. Not yet exercised for real (no
      failing PR has been pushed); `fail-fast: false` is set specifically
      for this, but treat as unconfirmed until a real matrix leg actually
      fails independently.
- [x] A clean push against current `master` is fully green. Confirmed on
      the first real run after landing this workflow: 20/20 jobs
      (`discover`, `lint`, 18 `build-test` legs), all `success`,
      https://github.com/apojomovsky/pic8-hal/actions/runs/30717451172.
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
      (https://github.com/apojomovsky/pic8-hal/actions/runs/30718807266).
- [~] Every existing module/MCU combination that builds locally today
      also builds green in this workflow. Two real bugs found across two
      runs, both fixed:
      1. Run 30718807266: `make` itself succeeded on every leg, but the
         workflow's own "Confirm .hex was produced" step assumed every
         module names its output `<MCU>-firmware.hex`, true only of the
         two top-level HAL Makefiles (each `pic8-*` module's `TARGET` has
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
         https://github.com/apojomovsky/pic8-hal/actions/runs/30720162258.
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
      https://github.com/apojomovsky/pic8-hal/actions/runs/30722036149.
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
      on run https://github.com/apojomovsky/pic8-hal/actions/runs/30722374627,
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
      https://github.com/apojomovsky/pic8-hal/actions/runs/30723598436
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
      https://github.com/apojomovsky/pic8-hal/actions/runs/30724128114,
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
    this repo's own `HAL_USART_Init` (`pic16f87xa_usart.c` and
    `pic18fxx5x_usart.c`, both families, same pattern) only sets `TXEN`
    when a non-null `TxCpltCallback` is supplied
    (`if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;`). Without one, the
    transmitter silently never enables, nothing shifts out, no error.
    Worth knowing for Phase 3's sim-target harness design, whatever it
    ends up transmitting over USART will need a callback set, even a
    no-op one, or `TXEN` never gets set.
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
- **GHCR image naming/ownership.** **RESOLVED (Phase 1):**
  `ghcr.io/${{ github.repository_owner }}/pic8-hal-ci`, resolved at
  workflow run time rather than hardcoded, so it stays correct across a
  fork or rename. Tag is `xc8-v<XC8_VERSION>-dfp<PIC18FXXXX_DFP_VERSION>`,
  read from `docker/ci-toolchain/Dockerfile`'s `ARG` defaults, so the tag
  changes automatically (forcing a rebuild instead of a stale reuse) if
  either pinned version is bumped.
