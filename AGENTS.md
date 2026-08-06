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
code; family manuals point back there instead of repeating it. Full
design: `docs/multi-family-plan.md`. `docs/adding-a-device.md` is the
verification-gated playbook for a new device or family (used to add
PIC16F193X, see `docs/pic16f193x-plan.md`).

## Module anatomy

Every `epic-*` module: `README.md`, often `docs/ARCHITECTURE.md` +
`docs/API.md`, host-testable via CMake (`cmake -B build && cmake
--build build && ctest`), real-target via `python3 scripts/epic_build.py
build --module <name> --mcu <MCU> --run`. No top-level build, build each
module directly. Each HAL additionally has `MANUAL.md`, datasheet-cited
per-peripheral register reference; `epic-common/MANUAL.md` covers
shared conventions (naming, handle pattern, harness, interrupt model),
family manuals only cover what's actually per-family.

## Build & toolchain

Two paths, pick either. **Native**: XC8/MPLAB X installed by hand
(license-gated), `export PATH=$PATH:/opt/microchip/xc8/v3.10/bin`,
`python3 scripts/epic_build.py build --module <name> --mcu <MCU> --run`;
`./scripts/bootstrap.sh` covers the host-sim side only. **Docker** (no
local installs beyond two vendor files only a human can fetch,
Microchip's CDN blocks scripted downloads): root `Makefile`, `make
check-vendor` -> `make image` -> `make test` / `make xc8-build
MODULE=... MCU=...` / `make mdb-test MODULE=... MCU=... DEVICE=...
DFP=...` / `make shell`. Details: `docs/docker-dev-plan.md`. Same image
is pushed to a **private** GHCR package CI pulls from
(`make ci-image-push`, human-triggered only; see that doc for why it
must stay private, EULA redistribution terms).

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
  are numeric (`STATUS,0`), never aliased. Full writeup:
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
  behavior changed, `MANUAL.md` if a register fact changed, the
  `Status:` line of the relevant `docs/<name>-plan.md`. A repo-wide
  audit once found stale `Status: not started` lines on shipped modules
  as the norm, not the exception, because this step kept getting
  skipped.
- **Non-trivial work gets a plan doc first**: `docs/<name>-plan.md`, a
  `Status:` line, explicit solved-vs-pending framing.
- **Before trusting an uncertain compiler/hardware behavior**, write a
  throwaway probe and inspect the generated `.s`/`.map`, don't assume
  from the datasheet alone. Has caught real wrong assumptions every
  time it's been tried (`epic-math`'s XC8 round-trip probe, the
  PIC16F193X BSR-addressing probe).
- **No em-dashes (—).** Not in docs, not in commit messages, not in code
  comments. Use a comma, a colon, or a period and a new sentence instead.
