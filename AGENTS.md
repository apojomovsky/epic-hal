# AGENTS.md

8-bit PIC HAL and tooling library. Three families: PIC16F87XA, PIC18F2455
(both full peripheral coverage), PIC16F193X/Enhanced Mid-range
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
through the manifest (the bundle's `epicurus.mk` or a reference project).

## Build & toolchain

Two paths, pick either. **Native**: XC8/MPLAB X installed by hand
(license-gated), `export PATH=$PATH:/opt/microchip/xc8/v3.10/bin`,
`python3 scripts/epic_build.py build --module <name> --mcu <MCU> --run`;
`./scripts/bootstrap.sh` covers the host-sim side plus the Docker
toolchain readiness checks. **Docker** (no
local installs beyond two vendor files only a human can fetch,
Microchip's CDN blocks scripted downloads): root `Makefile`, `make
check-vendor` -> `make image` -> `make test` / `make xc8-build
MODULE=... MCU=...` / `make mdb-test MODULE=... MCU=... DEVICE=...
DFP=...` / `make shell`. Details: DEVELOPMENT.md's Docker section.
Same image is pushed to a **private** GHCR package CI pulls from
(`make ci-image-push`, human-triggered only; see DEVELOPMENT.md for
why it must stay private, EULA redistribution terms).

CI (`.github/workflows/ci.yml`): two jobs, `host` (host build+ctest,
every module, plus lint, no Docker) and `target` (one Docker pull, then
real XC8 cross-compile for every MCU variant, real `mdb`/MPLAB SIM runs
that check actual register/UART output not just "compiled", and the
isolated bundle-gate build). `target` pulls the private image; neither
job ever builds it.

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
5. Third-party code keeps its own style; these rules are first-party
   only.
