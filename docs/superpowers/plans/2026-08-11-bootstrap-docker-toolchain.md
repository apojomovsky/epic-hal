# Bootstrap Docker-first Real-target Setup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `scripts/bootstrap.sh` the Docker-first setup for real-target work: verify Docker, handle the two vendor-sourced Microchip installers self-instructively (what, from where, where to place it, what next), and build the toolchain image when the installers are present, deleting the manual-install prose.

**Architecture:** One new "Docker toolchain" section in `bootstrap.sh` replaces the `xc8-cc` PATH check. It orchestrates, it does not own facts: `make check-vendor` (root Makefile) is the source of truth for installer filenames, size thresholds, and download URLs, and `make image` is the build. A cached-image `docker image inspect` check makes re-runs skip the build. Five docs that describe the old bootstrap contract are updated in a sweep.

**Tech Stack:** bash (`set -euo pipefail`), GNU make, Docker CLI, git.

## Global Constraints

- No em-dash characters (—) in docs, commit messages, or code comments; the word "em-dash" is fine.
- Conventional Commits (`feat`/`docs`/`plan`/`fix`/`refactor`/`style`), scope usually the module.
- `bootstrap.sh` must not duplicate vendor filenames, size thresholds, or download URLs; those live in the Makefile's `check-vendor` target only.
- `--check-only` never installs, builds, creates directories, or writes anything; it reports and exits nonzero on any gap.
- Normal-mode bootstrap exits 0 when the toolchain is not ready; host-sim builds do not need it.
- Docker is verify-only: bootstrap never installs Docker, adds group membership, or enables daemons.
- Image tag literal `pic8-hal-toolchain:local` mirrors the Makefile's `LOCAL_IMAGE`, kept in sync by hand (stable literal, no make round-trip).
- `set -e` must not turn a missing toolchain into a fatal abort; only real failures (e.g. `docker build` itself failing) abort.

---

### Task 1: Rewrite bootstrap.sh's XC8 section into the Docker toolchain section

**Files:**
- Modify: `scripts/bootstrap.sh:12-16` (header paragraph)
- Modify: `scripts/bootstrap.sh:73-80` (XC8 section replaced by the docker section)

**Interfaces:**
- Consumes: `make -C <repo_root> check-vendor` and `make -C <repo_root> image` (root Makefile), `docker` / `docker info` / `docker image inspect` on PATH, existing `check_only` and `problems` variables.
- Produces (observable contract):
  - normal mode: exit 0 with the toolchain not ready (guidance + recap printed, `vendor/` created); exit 0 with "image present" when cached; runs `make image` when installers are present and the image is not built; aborts only on a real failure.
  - `--check-only`: exit 1 iff docker missing, daemon unreachable, make missing, vendor files missing, or image not built; never writes anything.

- [ ] **Step 1: Replace the header paragraph (lines 12-16)**

Old text:

```bash
# Real-target (XC8) builds need MPLAB X + MPLAB XC8 v3.x installed
# manually: proprietary, license-gated, an interactive installer, not
# something apt can hand you. This script only checks whether `xc8-cc`
# is on PATH and points at the README if it isn't; host-simulation builds
# (the ones this script actually prepares you for) work without it.
```

New text:

```bash
# Real-target (XC8) builds and the mdb (MPLAB SIM) gate run inside the
# docker/ci-toolchain/ Docker image built by the root Makefile (`make
# check-vendor` -> `make image`), so nothing MPLAB-ish is installed on
# the host. The image needs two Microchip installer files a human
# fetches once into docker/ci-toolchain/vendor/ (Microchip's CDN blocks
# scripted downloads); this script verifies Docker and those files and
# builds the image when it can, printing exactly what to download and
# where to put it when it cannot.
```

- [ ] **Step 2: Replace the XC8 section (lines 73-80)**

Old text:

```bash
# ---- XC8 / MPLAB X (real-target builds; not automatable, see header) ----
if command -v xc8-cc >/dev/null 2>&1; then
    echo "bootstrap: xc8-cc found on PATH ($(command -v xc8-cc)), real-target builds are ready."
else
    echo "bootstrap: xc8-cc not on PATH. Host-simulation builds (cmake, most of this repo's"
    echo "  tests) work without it. For real-target builds, install MPLAB X + XC8 v3.x"
    echo "  manually, see README.md #Requirements, then add xc8-cc's bin/ to PATH."
fi
```

New text:

```bash
# ---- Docker toolchain (real-target builds) ----
# Real-target (XC8) builds and the mdb (MPLAB SIM) gate run inside the
# docker/ci-toolchain/ image; nothing MPLAB-ish is installed on the host
# (docs/docker-dev-plan.md). The image is built from two Microchip
# installer files only a human can fetch once (their CDN sits behind a
# bot-challenge, see `make check-vendor` and docs/ci-plan.md):
#   docker/ci-toolchain/vendor/xc8-installer.run
#   docker/ci-toolchain/vendor/mplabx-installer.tar
# `make check-vendor` and `make image` own the filenames, thresholds,
# URLs, and the build; this section only orchestrates them.
toolchain_ok=1
if ! command -v docker >/dev/null 2>&1; then
    echo "bootstrap: docker not found. Real-target builds run in a Docker image, see"
    echo "  docs/docker-dev-plan.md; install Docker first."
    toolchain_ok=0
elif ! docker info >/dev/null 2>&1; then
    echo "bootstrap: docker found but the daemon is not reachable (is it running? are"
    echo "  you in the docker group?). Real-target builds need it."
    toolchain_ok=0
elif ! command -v make >/dev/null 2>&1; then
    echo "bootstrap: make not found, cannot run check-vendor/image. Install make, or"
    echo "  build the image yourself with:"
    echo "    docker build -t pic8-hal-toolchain:local docker/ci-toolchain"
    toolchain_ok=0
fi

if [ "$toolchain_ok" = 1 ]; then
    # vendor/ is the drop location the guidance below points at; create
    # it so "place it here" is a real path. check-only never writes.
    if [ "$check_only" = 0 ]; then
        mkdir -p "$repo_root/docker/ci-toolchain/vendor"
    fi
    if make -C "$repo_root" check-vendor; then
        # Same tag as the Makefile's LOCAL_IMAGE, kept in sync by hand
        # (a stable literal, not worth a make round-trip to derive).
        if docker image inspect pic8-hal-toolchain:local >/dev/null 2>&1; then
            echo "bootstrap: docker toolchain image present (pic8-hal-toolchain:local);"
            echo "  real-target builds are ready (make xc8-build / make mdb-test)."
        elif [ "$check_only" = 1 ]; then
            echo "bootstrap: docker toolchain image not built yet (run ./scripts/bootstrap.sh"
            echo "  or 'make image')."
            problems=1
        else
            make -C "$repo_root" image
        fi
    else
        echo "bootstrap: real-target toolchain not built yet. One-time manual step:"
        echo "  1. In a browser, download the two Microchip installers (links in the"
        echo "     make output above; the bot-challenge blocks scripted downloads):"
        echo "       docker/ci-toolchain/vendor/xc8-installer.run"
        echo "       docker/ci-toolchain/vendor/mplabx-installer.tar"
        echo "  2. Re-run ./scripts/bootstrap.sh (or run 'make image')."
        echo "  Host-simulation builds and tests work without this."
        [ "$check_only" = 1 ] && problems=1
    fi
elif [ "$check_only" = 1 ]; then
    problems=1
fi
```

- [ ] **Step 3: Syntax check**

Run: `bash -n scripts/bootstrap.sh`
Expected: no output, exit 0.

- [ ] **Step 4: Check-only run proves report-only behavior**

Run: `./scripts/bootstrap.sh --check-only; echo "exit=$?"`
Expected: exit=1; output includes the `check-vendor` guidance (both installer paths) and the recap block; Docker itself is not complained about (daemon is up). No `vendor/` directory exists afterward:

Run: `test ! -d docker/ci-toolchain/vendor && echo "vendor/ not created"`
Expected: `vendor/ not created` (it does not exist before this task; if it does, remove it first with `rm -rf docker/ci-toolchain/vendor`, it is gitignored).

- [ ] **Step 5: Normal run prints guidance, creates vendor/, exits 0**

Run: `./scripts/bootstrap.sh; echo "exit=$?"`
Expected: exit=0; packages line, hook line, `check-vendor` guidance (both installer paths), and the recap block ("bootstrap: real-target toolchain not built yet. One-time manual step:") all print; `docker/ci-toolchain/vendor/` now exists (`test -d docker/ci-toolchain/vendor`).

- [ ] **Step 6: Commit**

```bash
git add scripts/bootstrap.sh
git commit -m "feat(bootstrap): docker-first real-target setup with self-instructive vendor handling"
```

---

### Task 2: State minimum installer sizes in check-vendor guidance

**Files:**
- Modify: `Makefile:91` (XC8 "missing (or too small)" message)
- Modify: `Makefile:99` (MPLAB X "missing (or too small)" message)

**Interfaces:**
- Consumes: nothing new; same thresholds as today (>= 10,000,000 bytes XC8, >= 100,000,000 bytes MPLAB X), matching the Dockerfile's own assertions.
- Produces: `check-vendor` failure output that states the minimum-size expectation so a truncated browser download is recognized. Task 1 does not parse this output, only its exit status; the wording change is safe after Task 1.

- [ ] **Step 1: Edit the XC8 message**

Old:

```make
		echo "missing (or too small): $(XC8_INSTALLER)"; \
```

New:

```make
		echo "missing (or too small, expected at least ~10 MB): $(XC8_INSTALLER)"; \
```

- [ ] **Step 2: Edit the MPLAB X message**

Old:

```make
		echo "missing (or too small): $(MPLABX_INSTALLER)"; \
```

New:

```make
		echo "missing (or too small, expected at least ~100 MB): $(MPLABX_INSTALLER)"; \
```

- [ ] **Step 3: Verify the failure output**

Run: `make check-vendor; echo "exit=$?"`
Expected: target fails (make exits 2, because the recipe `exit 1`s when files are missing); output names both files with the new size expectations, both Microchip URLs, the save-as instructions, and the bot-challenge note.

- [ ] **Step 4: Commit**

```bash
git add Makefile
git commit -m "fix(Makefile): state minimum installer sizes in check-vendor guidance"
```

---

### Task 3: Docs sweep for the docker-first bootstrap contract

**Files:**
- Modify: `scripts/README.md:21-29` (Bootstrap section prose)
- Modify: `DEVELOPMENT.md` ("Native toolchain" section, bootstrap paragraph + native exception framing)
- Modify: `docs/docker-dev-plan.md` (Status paragraph + "What does NOT change" bullet)
- Modify: `AGENTS.md:40` ("covers the host-sim side only")
- Modify: `docs/ci-plan.md:480` (host-job scope wording)
- Modify: `docs/superpowers/specs/2026-08-11-bootstrap-docker-toolchain-design.md` (Status line flip)

**Interfaces:**
- Consumes: the Task 1 behavior (docker section exists in bootstrap.sh) and Task 2 (check-vendor size wording).
- Produces: consistent docs; no doc still tells the user to install MPLAB manually as the bootstrap-recommended path.

- [ ] **Step 1: scripts/README.md**

Old:

```markdown
then runs
`install-git-hooks.sh` below. Also checks whether `xc8-cc` is on `PATH`
and points at the README if it isn't. Real-target (XC8) builds need
MPLAB X + MPLAB XC8 v3.x installed manually (proprietary, license-gated,
an interactive installer, not something this script attempts); host
builds work fine without it.
```

New:

```markdown
then runs
`install-git-hooks.sh` below, and finally verifies the Docker toolchain
for real-target work: it checks Docker is installed and reachable,
tells you which two Microchip installer files to download and where to
put them (`docker/ci-toolchain/vendor/`), and builds the toolchain
image once they are in place. Real-target (XC8) builds and the `mdb`
gate run inside that image (root `Makefile`), so nothing MPLAB-ish is
installed on the host; host builds work with or without it.
```

- [ ] **Step 2: DEVELOPMENT.md**

Old:

```markdown
`./scripts/bootstrap.sh` sets up a fresh clone: installs the host
toolchain the CMake builds need and a pre-commit hook (trailing
newline/whitespace, no em-dash, `cppcheck` on staged `.c` files).
`--check-only` reports what's missing without installing anything. See
[scripts/README.md](scripts/README.md) for what the hook checks.

Real targets additionally need MPLAB X IDE v6.x and MPLAB XC8 v3.x
(`xc8-cc`), installed by hand (proprietary, license-gated); PIC18 also
needs the PIC18Fxxxx DFP, PIC16F193X the PIC12-16F1xxx DFP (neither
ships with XC8).
```

New:

```markdown
`./scripts/bootstrap.sh` sets up a fresh clone: installs the host
toolchain the CMake builds need and a pre-commit hook (trailing
newline/whitespace, no em-dash, `cppcheck` on staged `.c` files), then
verifies the Docker toolchain: it checks Docker is installed and
reachable, handles the two Microchip installer files
(`docker/ci-toolchain/vendor/`) self-instructively, and builds the
toolchain image once they are in place. `--check-only` reports what's
missing without installing anything. See
[scripts/README.md](scripts/README.md) for what the hook checks.

Docker is the default path for real-target work (next section); the
native exception path needs MPLAB X IDE and MPLAB XC8 (`xc8-cc`)
installed by hand (proprietary, license-gated): PIC18 additionally
needs the PIC18Fxxxx DFP, PIC16F193X the PIC12-16F1xxx DFP (neither
ships with XC8).
```

- [ ] **Step 3: docs/docker-dev-plan.md Status paragraph**

Old:

```markdown
Status: **implemented and fully verified against real builds**,
including the full image (XC8 + all three DFPs + MPLAB X) and a real
`mdb` gate run.
```

New:

```markdown
Status: **implemented and fully verified against real builds**,
including the full image (XC8 + all three DFPs + MPLAB X) and a real
`mdb` gate run. `scripts/bootstrap.sh` now runs `check-vendor` and
`image` itself when the installers are present (design:
`docs/superpowers/specs/2026-08-11-bootstrap-docker-toolchain-design.md`).
```

- [ ] **Step 4: docs/docker-dev-plan.md "What does NOT change" bullet**

Old:

```markdown
- `scripts/bootstrap.sh`, `scripts/sim-test-local.sh`,
  `scripts/sim-mdb-run.sh` are unchanged; the root Makefile's `mdb-test`
  target calls the same `scripts/sim-mdb-run.sh`, so CI, the old local
  script, and the new Makefile path all share one source of truth for
  the actual `mdb` command sequence.
```

New:

```markdown
- `scripts/sim-test-local.sh` and `scripts/sim-mdb-run.sh` are
  unchanged; `scripts/bootstrap.sh` now verifies Docker and the vendor
  installers and builds the toolchain image as part of setup (design:
  `docs/superpowers/specs/2026-08-11-bootstrap-docker-toolchain-design.md`).
  The root Makefile's `mdb-test` target calls the same
  `scripts/sim-mdb-run.sh`, so CI, the old local script, and the new
  Makefile path all share one source of truth for the actual `mdb`
  command sequence.
```

- [ ] **Step 5: AGENTS.md**

Old:

```markdown
`./scripts/bootstrap.sh` covers the host-sim side only. **Docker** (no
```

New:

```markdown
`./scripts/bootstrap.sh` covers the host-sim side plus the Docker
toolchain readiness checks. **Docker** (no
```

- [ ] **Step 6: docs/ci-plan.md**

Old:

```markdown
**Explicitly out of scope**: no XC8, no MPLAB X, no Docker. Pure host
tooling, matching what `scripts/bootstrap.sh` already sets up.
```

New:

```markdown
**Explicitly out of scope**: no XC8, no MPLAB X, no Docker. Pure host
tooling, matching the host-tooling subset of `scripts/bootstrap.sh`.
```

- [ ] **Step 7: Flip the design spec's Status line**

Old:

```markdown
Status: **approved 2026-08-11, not started**.
```

New:

```markdown
Status: **implemented 2026-08-11**.
```

- [ ] **Step 8: Verify docs (whitespace, no em-dash characters) and commit**

Run: `git add scripts/README.md DEVELOPMENT.md docs/docker-dev-plan.md AGENTS.md docs/ci-plan.md docs/superpowers/specs/2026-08-11-bootstrap-docker-toolchain-design.md && PRE_COMMIT_BASE_REF=master scripts/pre-commit-checks.sh`
Expected: PASS (the pre-commit checks run whitespace and em-dash rules on the staged lines against `master`; no `.c` files staged, so cppcheck is skipped).

Also search the six files for a literal em-dash character (pattern `\u2014`), scoped to the same six paths.
Expected: no matches (pre-existing em-dashes would show; the plan's edits add none).

Then commit:

```bash
git add scripts/README.md DEVELOPMENT.md docs/docker-dev-plan.md AGENTS.md docs/ci-plan.md docs/superpowers/specs/2026-08-11-bootstrap-docker-toolchain-design.md
git commit -m "docs(bootstrap): docker-first setup across dev docs and plan statuses"
```

---

### Task 4: Final end-to-end verification

**Files:** none (verification only).

**Interfaces:**
- Consumes: the finished Task 1-3 state.
- Produces: evidence that the shipped contract holds; the remaining user step (dropping the two installer files) is the only thing between this state and a built image.

- [ ] **Step 1: Re-run both bootstrap modes back to back**

Run: `./scripts/bootstrap.sh --check-only; echo "exit=$?"; ./scripts/bootstrap.sh; echo "exit=$?"`
Expected: first exits 1 with the guidance + recap and no `vendor/` writes from check-only itself (vendor/ already exists from Task 1, so verify with `test -d` before/after: check-only changes nothing); second exits 0 with the same guidance + recap.

- [ ] **Step 2: Confirm the ready-path logic is reachable**

Run: `docker image inspect pic8-hal-toolchain:local; echo "exit=$?"`
Expected: exit 1 (image not built yet). This confirms the branch the user will hit after placing the installers: `make check-vendor` passes, inspect fails, `make image` runs. The full `make xc8-build` / `make mdb-test` PASS leg requires the two human-fetched installer files and is out of reach for this session by design (documented in the spec).

- [ ] **Step 3: Confirm git state is clean and history tells the story**

Run: `git status --short && git log --oneline -4`
Expected: clean working tree; the four commits from Tasks 1-3 plus this plan's commit, each with a Conventional Commit message.

- [ ] **Step 4: Update this plan's completion state**

Add a `Status: **implemented 2026-08-11**` line under the header (mirroring the spec), or mark every checkbox `[x]` and commit:

```bash
git add docs/superpowers/plans/2026-08-11-bootstrap-docker-toolchain.md
git commit -m "plan(bootstrap): mark docker-first setup plan implemented"
```
