# AGENTS.md

8-bit PIC HAL and tooling library. Families: PIC16F87XA, PIC18F2455 and
PIC16F88X (full peripheral coverage), PIC16F193X/Enhanced Mid-range
(foundation only, GPIO+Timer0, peripherals land incrementally). C99,
MPLAB XC8. Every module dual-builds: host simulation (gcc/CMake, no
hardware) and real-target (XC8 Makefile, produces `.hex`). Applications
never `#ifdef` between them, the split happens at build time via
include-path and linked-file selection.

## The one idea that matters most

`epic-common/` holds everything architecture-blind (status codes, the
4-function harness, shared CMake/Make fragments). Everything
register-specific (SFR maps, bank/BSR addressing, IRQ vectors,
peripheral bodies) lives per-family under a **fixed contract**: same
names/signatures across families, different bodies. Read
`epic-common/README.md` + `epic-common/MANUAL.md` before touching HAL
code; family manuals point back there instead of repeating it. Those
two are the shared-contract design docs. `docs/adding-a-device.md` is
the verification-gated playbook for a new device or family (used to
add PIC16F193X).

## Module anatomy

Every `epic-*` module: `README.md`, often `docs/ARCHITECTURE.md` +
`docs/API.md`, host-testable via CMake (`cmake -B build && cmake
--build build && ctest`), real-target via `python3 scripts/epic_build.py
build --module <name> --mcu <MCU> --run`. No top-level build, build each
module directly. Each HAL additionally has `MANUAL.md`, datasheet-cited
per-peripheral register reference; `epic-common/MANUAL.md` covers
shared conventions (naming, handle pattern, harness, interrupt model),
family manuals only cover what's actually per-family.

Each HAL's `src/` mirrors its build environments: `src/core/` and
`src/peripherals/` are shared, `src/target/` is real-hardware-only,
`src/sim/` is host-simulation-only, `src/mdb/` holds the MPLAB-SIM gate
variant. Never glob a HAL `src/` directory into a build; select files
through the manifest (the bundle's `epic-hal.mk` or a reference project).
The `epic-hal` CLI (`scripts/epic_hal.py`) scaffolds consumer projects
from a bundle: it writes `main.c`, a filled `Makefile`, and a patched
MPLAB X `.X` for a chosen part and module subset.

## Build & toolchain

First-time setup: `make bootstrap` (installs host packages, git hooks,
and the docker toolchain image; `make doctor` reports what is missing
without changing anything). **Native**: XC8/MPLAB X installed by hand
(license-gated), `export PATH=$PATH:/opt/microchip/xc8/v3.10/bin`,
`python3 scripts/epic_build.py build --module <name> --mcu <MCU> --run`;
`./scripts/bootstrap.sh` covers the host-sim side plus the Docker
toolchain readiness checks. **Docker** (no
local installs beyond two vendor files only a human can fetch,
Microchip's CDN blocks scripted downloads): root `Makefile`, `make
check-vendor` -> `make image` -> `make test` / `make xc8-build
MODULE=... MCU=...` / `make epiccc-build MODULE=... MCU=...` (the
epic-cc toolchain path, runs in the epic-cc dev image) / `make
mdb-test MODULE=... MCU=... DEVICE=...` / `make mdb-epiccc MODULE=...
MCU=... DEVICE=...` (deterministic toggle gate over an already built
epic-cc hex) / `make mdb-hex HEX=... DEVICE=...` (run a register-read
gate on an existing hex) / `make shell`. Details: DEVELOPMENT.md's
Docker section.
Same image is pushed to a **private** GHCR package CI pulls from
(`make ci-image-push`, human-triggered only; see DEVELOPMENT.md for
why it must stay private, EULA redistribution terms).

All builds and gates go through the Makefile targets, which own the
docker plumbing (container images, bind mounts, environment). Never
hand-roll a `docker run` or call a toolchain binary directly: if a
target is missing or wrong, fix the Makefile and use it, and record
the reason in the target's comment rather than a workaround in this
file. The only host-side step is `epic_build.py` resolution, which
emits a script for a container target to execute.

CI (`.github/workflows/ci.yml`): a `host` job (host build+ctest for
every module, the Python tooling tests, plus lint, no Docker) and one
per-family job per family, each a call into the reusable
`family-check.yml` (Docker pull, then real XC8 cross-compile for every
MCU variant of that family, real `mdb`/MPLAB SIM runs that check actual
register/UART output not just "compiled", the device-data audits, and
the isolated bundle-gate build). The family jobs pull the private
image; no job ever builds it.

## Picking up work

Work across epic-cc, epic-hal and epic-platformio is coordinated by
[epic-tasks](https://github.com/apojomovsky/epic-tasks). Several agents, from
different providers and on different machines, share one GitHub account, so the
board is the only place that knows what is already taken. **Do not choose a
ticket by reading the issue list.**

Run once per machine (and after any env change):

0. `epic-tasks doctor`: checks `EPIC_AGENT_ID`, `EPIC_TASKS_PROJECT`,
   `gh` auth with `project` scope, and board reachability. Fix what it
   reports before claiming.

For every ticket:

1. `epic-tasks next` to see what you may take, `epic-tasks claim <repo>#<n>` to
   take it. Exit 2 means another agent won the race, so go back to `next`.
   Exit 3 means the board is unreachable: do the work and say so in the pull
   request. Exit 4 means stop and ask.
2. Create a worktree under `.worktrees/` and branch as
   `<type>/<issue>-<slug>`, for example `feat/58-epic-cc-build-backend`
   (see Worktrees below, never work on `master`).
3. Work, then run the takeoff ritual (`make pre-pr-check` → `epic-tasks takeoff`).
4. Open the pull request with `Closes #N`, then
   `epic-tasks review <repo>#<n> --pr <url>`.
5. After the PR merges, remove the worktree:
   `git worktree remove .worktrees/<name>`. Never remove a worktree before
   merge, the branch must stay reachable for review.


Set `EPIC_AGENT_ID` (`<runtime>@<host>`) and `EPIC_TASKS_PROJECT` once per
runtime and machine. `claim` refuses to act without an identity, because an
anonymous claim tells the other agents nothing.

An issue also carries `area:*` labels naming the surfaces it touches
(`pic16-hal`, `pic18-hal`, `epic-math`, `build`, `docs`, `ci`). Two tickets
sharing an area cannot be worked at the same time even when neither blocks the
other, which is why selection goes through the tool: what is blocked, taken, or
conflicting is decided there, not in this file.

Several epic-hal tickets are blocked by epic-cc work, and the tool reads those
dependencies live from the issues, so a blocked ticket is never offered.

## Worktrees

**All feature work happens in a worktree under `.worktrees/`**, never on
`master`, and worktrees are removed only after the PR merges:

```bash
git fetch origin master
git worktree add .worktrees/<name> -b <branch> origin/master
# ... work, PR, merge ...
git worktree remove .worktrees/<name>
```

Branch names mirror the commit types: `feat/<description>`,
`fix/<description>`, `chore/<description>`, `docs/<description>`. The
worktree keeps your master checkout clean and lets several tasks run in
parallel without touching each other's trees. `.worktrees/` is
gitignored, so a worktree is never part of a diff.

Two things are shared by every worktree, so they are set up once, not
per tree: the git hooks (`make setup-hooks` writes into the common hooks
dir) and the toolchain image (`make image` produces a docker tag, not a
file in the tree). Per-worktree: each needs its own `cmake -B build`
output, and every container target bind-mounts the worktree it was run
from, so `make test`/`make xc8-build` do the right thing without extra
flags. The container HOME mount (`~/.cache/epic-hal-toolchain-home`) is
the one writable path they all share.

The one thing a worktree needs a copy of is `docker/ci-toolchain/vendor/`:
it is gitignored, so a new tree starts without the two installers.
`check-vendor` hard-links them from the main checkout, which costs no
disk and needs no flag, so this is invisible unless the main checkout
never had them either.

Worktree discipline is enforced by the takeoff ritual (`epic-tasks takeoff`
checks you are in a `.worktrees/` worktree and not on `master`).

## Development cycle

Fast inner loop stays host-sim only, either path: `cmake -B build &&
cmake --build build && ctest` (native) or `make test MODULE=<dir>`
(Docker), repeat. Only move to real-target + `mdb` once the host-sim
example passes, that loop is much slower.

Real-target build: `python3 scripts/epic_build.py build --module <name>
--mcu <mcu> --run` (native) or `make xc8-build MODULE=<name> MCU=<mcu>`
(Docker). `scripts/sim-mdb-run.sh` runs the `mdb` gate either way, inside
the container or directly if `xc8-cc`/`mdb.sh` are on `PATH` and
`$XC8_INSTALL_DIR` is set the same way (its own header comment covers
direct use); `make mdb-test` is just the Docker-wrapped call to it.

A build or test failing with too little output: `make shell` (Docker)
drops into the same container interactively for full output; native
already has a real shell.

A real-target register looking wrong: `docs/adding-a-device.md` §4 is
the debug protocol, for any real-target bug, not just new peripherals.
Covers `stepi` over `run`+`wait` (unreliable headless), checking a
known-good control register before blaming timing, and the
high-risk-pattern checklist (runtime SFR addresses, read-modify-write,
clock-derived divisors) that has caught every real bug found in this
codebase so far.

## Takeoff ritual (before every PR)

Run `make pre-pr-check` before opening a PR. It is a thin wrapper around
`epic-tasks takeoff`, the shared skeleton used by every epic repository
(canonical checks live in `epic-tasks/epic_tasks/takeoff.py`). It checks:

1. Working tree clean, branch not behind `origin/master` (or `$BASE_REF`).
2. **You are in a `.worktrees/` worktree**, not on `master`.
3. **No plan docs in the PR's final diff.** Plans
   (`docs/superpowers/plans/`) live through development; the final
   commit distills the durable facts into the living docs (the module's
   `README.md`/`docs/`, `MANUAL.md` for a register fact,
   `DEVELOPMENT.md` or `docs/adding-a-device.md` for a toolchain or
   debug gotcha) and `git rm`s the plan. Squash merging then keeps
   master plan-free. The plan stays visible in the PR's commit history.
4. Commit hygiene: conventional subjects, no attribution trailers, no
   em-dashes, no whitespace errors in the diff.
5. **Docstring compliance.** `scripts/doxygen_doc_check.py` over the C
   files the PR touches, `--brief-only` for `tests/` and `examples/`.
   Hard gate, and scoped to the diff, so a PR is never charged for a
   pre-existing violation elsewhere in the tree.
6. **Comment and doc prose review.** `scripts/prose-diff.sh` prints
   every added comment block and markdown hunk in the PR. It flags a
   few objective signals (a block over ~8 lines, a hardcoded count or
   pasted tree, a local `.pdf` link) but cannot judge content, so it
   never fails the ritual on its own. Read everything it printed
   against the Expression conventions below and fix what doesn't hold
   up; `make pre-pr-check PROSE=1` records that the review happened.
7. Hooks installed (`make setup-hooks`).
8. `make pre-pr-check TEST=1` also runs the host-sim suite (or `epic-tasks takeoff --test`).

The ritual exits 1 with the exact fix list while blocking items are
outstanding. It complements the pre-commit hook rather than repeating
it: the hook gates one commit's staged content, the ritual gates the
whole PR range. Don't skip it, CI covers the builds and the sim gates,
not the ritual. `epic-tasks takeoff --prose` is the same as `PROSE=1`.

## Ground rules

- **Approval gates are real.** Brainstorm -> design -> approve ->
  implement. Present a design and stop until you get a yes, even for
  work that looks small.
- **The license-gated vendor files never enter the repo.** The XC8 and
  MPLAB X installers under `docker/ci-toolchain/vendor/` and the
  datasheet PDFs are Microchip downloads a human fetches by hand, and
  both are gitignored. Redistribution is what forces the GHCR package
  to stay private (DEVELOPMENT.md).
- **A failing `mdb` gate is a defect, not a flaky check.** It asserts
  on real register and UART output, so debug the target with
  `docs/adding-a-device.md` §4 before touching the assertion. Loosening
  a gate to get green is how a silent miscompile ships.
- **No force pushes.** Rewriting a branch that already exists on the
  remote drops it for every other agent and clone; the pre-push hook
  refuses it. If history is genuinely messy, rebase onto master and
  get the human's explicit go-ahead before re-running with
  `EPIC_FORCE_PUSH_APPROVED=1 git push --force-with-lease`.

## Non-obvious things that will bite you

- **The toolchain container has no python3.** `epic_build.py` therefore
  resolves the manifest and emits a `sh` script rather than calling
  `xc8-cc` itself; resolution runs on the host or CI runner, execution
  runs in the container. Do not "simplify" this into a direct call.
- **XC8 inline asm is not GNU extended asm.** No operand constraints.
  Only file-scope `static volatile` symbols are addressable. PIC16 user
  globals need a leading `_` in the asm string; SFRs don't. STATUS bits
  are numeric (`STATUS,0`), never aliased. Reuse the mnemonics already
  proven in this codebase's inline asm (movf/addwf/subwf/addlw/movlw/
  movwf/clrf/incf/incfsz/rlf/rrf, register-bit btfsc/btfss, goto +
  labels); an untested mnemonic or STATUS-bit combination can be
  rejected by XC8 with error (876), so probe a candidate instruction
  before assuming it assembles. Full writeup:
  `epic-math/docs/ARCHITECTURE.md`.
- **Banking differs per family, not just "PIC16 vs PIC18."** Classic
  PIC16 (87XA): RP0/RP1 bank bits, `STATUS,7`=IRP selects a bank-*pair*.
  PIC18: Access Bank, no BSR. Enhanced Mid-range (193X): real BSR (32
  banks x 128B); runtime-dispatched SFR addresses there compile to safe
  FSR1:INDF1 indirect addressing, not the classic-PIC16 failure mode
  (verified, not assumed: `pic16f193x-hal/docs/ARCHITECTURE.md` Finding
  1). The linker scatters unpinned `static` by best-fit, not declaration
  order, pin anything bank-sensitive with `__at(addr)`.
- **Datasheet/app-note PDFs are not committed** (`*.pdf` gitignored).
  Link Microchip's own hosted copies, never a local path.
- **Interrupt model differs per family.** Classic PIC16: one vector, no
  priority, manual context save. PIC18: two vectors (high/low),
  `EPIC_IRQ_SetPriority` real. Enhanced Mid-range: one vector, no
  priority, but *automatic* hardware context save (no manual push/pop).
  Enable/disable API shape is otherwise identical,
  `epic-common/MANUAL.md` §6.

## Conventions

- **Commit whenever a piece of work is finished**, Conventional Commits
  (`type(scope): summary`; `feat`/`docs`/`plan`/`fix`/`refactor`/`style`).
  Scope is usually the module or `phaseN`. Don't batch unrelated changes.
  Subjects are release-notes copy: `scripts/release_notes.py` groups them
  into the GitHub Release verbatim, so write them for someone reading the
  release page. A change that breaks consumers needs `type(scope)!:` or a
  `BREAKING CHANGE:` footer, otherwise nothing flags it there.
- **Never `Co-Authored-By:` or any other attribution trailer.** Git
  history is the human author's record, and the release notes are built
  from these commits, so a trailer makes them speak for someone who did
  not sign off. The `commit-msg` hook rejects trailers and em-dashes;
  `make pre-pr-check` re-checks the whole PR range.
- **Update the docs a change touches before calling it done**: the
  module's `README.md`/`docs/API.md`/`docs/ARCHITECTURE.md` if
  behavior changed, `MANUAL.md` if a register fact changed.
- **Non-trivial work gets a plan doc first**, and the plan is
  **ephemeral**: it lives during the work and is deleted when the work
  lands. Git history is the archive. No
  `Status:` line bookkeeping; a design doc for implemented work is a
  bitacore, not documentation.
- **Before trusting an uncertain compiler/hardware behavior**, write a
  throwaway probe and inspect the generated `.s`/`.map`, don't assume
  from the datasheet alone. Has caught real wrong assumptions every
  time it's been tried (`epic-math`'s XC8 round-trip probe, the
  PIC16F193X BSR-addressing probe).
- **No em-dashes (—).** Not in docs, not in commit messages, not in code
  comments. Use a comma, a colon, or a period and a new sentence instead.
  Replacing one is a judgment call, not a swap: pick the replacement
  (and split or reorder the sentence when it needs it) so the result
  reads as prose. A mechanical em-dash-to-comma sweep produces comma
  splices, which is why `make pre-pr-check` flags the leftover space
  before a comma as a warning. The exception is ascii-art diagrams,
  where alignment may force them.
- **API naming:** module `epic-X` exports `epic_x_*` (lowercase, e.g.
  `epic_serial_init`); HALs export `EPIC_*` uppercase for the
  cross-family contract (EPIC_GPIO_Init); family-internal helpers use
  the family prefix (`pic16f87xa_*`, `pic18_*`, `pic16f193x_*`);
  epic-common harness glue uses `epic_*`; third-party keeps its own
  names. A module's public symbols never use a bare short prefix
  (`fsm_*`, `task_manager_*`).

## Expression conventions (comments and docs)

### Comments

1. **Why, not what.** Code says what it does; comments carry what the
   code cannot: the non-obvious reason, the datasheet fact, the
   invariant. A comment that restates the line below it is deleted.
2. **A comment must earn its lines.** More comment lines than code
   lines is a smell. Hard cap ~8 lines per block; longer needs a real
   justification (a hand-trace of non-obvious asm, a race or
   side-effect proof). Hand-traces survive only where behavior cannot
   be read from the code, compressed to the essential steps.
3. **No decoration.** No `/* ---- name ---- */` separators, no
   `@file`/`@brief` boilerplate that repeats the filename. A 1-3 line
   file header is fine when it adds context (which backend, what it
   rides on).
4. **No narrative.** No "fixed X by doing Y", no iteration or session
   prose. Verification claims about a change ("verified by
   simulation", "probe confirmed") belong in the PR and commit, not in
   the tree, where they go stale. Durable toolchain or hardware facts
   (with a date) are a different class and stay.
5. **Register maps and datasheet citations stay.** The
   datasheet-faithful contract is the exception to "why not what":
   bit-field encodings and SFR facts keep their citations.
6. `TODO`/`FIXME` carry a concrete reason or do not exist.

### Function docstrings (Doxygen style)

Every first-party function carries a Doxygen-style docstring:

- `@brief` on every function; a longer `@details` only when the
  behavior is not obvious.
- `@param name` per named argument, names matching the signature, with
  in/out semantics in the prose; never `@param[in]`/`@param[out]`.
- `@return` for non-void functions, nothing for void.
- The block is `/** ... */`, never `/*` or `//`.
- Placement: every signature in every checked file needs a doc block
  before it; the header remains the canonical public-API doc.
  Tests and examples: `@brief`-only.
- `scripts/doxygen_doc_check.py` is the compliance checker; run it
  before finishing work that touches functions.

### Docs lifecycle

1. `README.md` = what a human needs to use and maintain the module:
   purpose, usage, build/test commands, links. Living.
2. `MANUAL.md` = the datasheet-cited register/peripheral reference.
   Living.
3. Design docs are ephemeral: written during the work, deleted on
   completion. Git history is the archive.
4. No bitacores: findings narratives and session logs describing
   completed work are deleted. Live gotchas (asm rules, banking,
   debug protocol, tag formulas) live in README/DEVELOPMENT/MANUAL,
   the places a future maintainer actually reads.
5. **Write for a reader who wasn't there.** Clear, easy to follow,
   sized to the point being made: a doc that overwhelms with detail is
   as broken as one that omits the load-bearing fact.
6. **No coupling to volatile facts.** Test counts, module counts, a
   pasted directory tree, line numbers: describe the mechanism, never a
   snapshot that goes stale on the next merge.
7. **Diagrams earn their place.** A diagram is welcome where it
   clarifies structure or flow that prose would belabor; skip it for
   anything a sentence already says clearly.
8. Third-party code keeps its own style; these rules are first-party
   only.
