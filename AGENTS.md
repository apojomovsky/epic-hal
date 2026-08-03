# AGENTS.md

8-bit PIC HAL and tooling library (Microchip PIC16F87XA + PIC18F2455
families), C99, MPLAB XC8. Every module dual-builds: a host simulation
backend (gcc/CMake, runs as a normal program, no hardware) and a
real-target build (XC8 Makefile, produces a `.hex`). Applications never
`#ifdef` between them, the split happens at build time via include-path
and linked-file selection.

## The one idea that matters most

`pic8-common/` holds everything architecture-blind (status codes, the
4-function test/firmware harness, shared CMake/Make fragments). Everything
register-specific (SFR maps, bank/BSR addressing, IRQ vectors, peripheral
bodies) lives per-family under a **fixed contract**: same names/signatures
across families, different bodies. Read `pic8-common/README.md` +
`pic8-common/MANUAL.md` before touching any HAL code, every family manual
points back there instead of repeating it. Full design:
`docs/multi-family-plan.md`. `docs/adding-a-device.md` is the playbook if a
new device variant or a third family shows up.

## Module anatomy

Every `pic8-*` module: `README.md` (what/why), often
`docs/ARCHITECTURE.md` + `docs/API.md`, host-testable via CMake
(`cmake -B build && cmake --build build && ctest`), real-target via
`mcu/<family>-*-mplabx/Makefile`
(`export PATH=$PATH:/opt/microchip/xc8/v3.10/bin && make MCU=...`). No
top-level build, build each module directly.

The two HALs (`pic16f87xa-hal/`, `pic18fxx5x-hal/`) additionally have
`MANUAL.md`, per-peripheral register reference, datasheet-cited.
`pic8-common/MANUAL.md` has the shared conventions (naming, handle
pattern, harness, interrupt model); family manuals only cover what's
actually per-family.

## Non-obvious things that will bite you

- **XC8 inline asm is not GNU extended asm.** No operand constraints. Only
  file-scope `static volatile` symbols are addressable, not function
  params, not local statics. PIC16 user globals need a leading `_` in the
  asm string; SFRs (`FSR`, `STATUS`, `PORTD`, …) don't. STATUS bits are
  numeric (`STATUS,0`), never aliased (`STATUS,C` fails to link). Full
  empirically-probed writeup: `pic8-math/docs/ARCHITECTURE.md`.
- **PIC16 indirect addressing (`FSR`/`INDF`) banking**: `STATUS,7` is
  `IRP`, it selects a 256-byte bank-*pair*, not a per-bank switch. The
  linker scatters `static` placement by best-fit, not declaration order,
  pin anything that needs a known bank with `__at(addr)` rather than
  relying on default placement.
- **Datasheet/app-note PDFs are not committed** (`*.pdf` is gitignored).
  Reference them as links to Microchip's own hosted copies
  (`ww1.microchip.com/downloads/en/...`), never as a local path.
- **PIC16 has one interrupt vector, no priority; PIC18 has two (high/low)
  with `HAL_IRQ_SetPriority`.** The enable/disable API shape is otherwise
  identical, see `pic8-common/MANUAL.md` §6.

## Conventions

- **Commit whenever a piece of work is finished**, using Conventional
  Commits (`type(scope): summary`, types `feat`, `docs`, `plan`, `fix`,
  `refactor`, `style`). Scope is usually the module (`feat(pic8-lcd):
  ...`) or `phaseN` for multi-family work. Don't batch unrelated changes
  into one commit, split them and commit each as its own piece of work
  finishes.
- **Before calling a development or fix task done, update the docs it
  touches.** That means the module's `README.md`/`docs/API.md`/
  `docs/ARCHITECTURE.md` if a public API, behavior, or status changed; the
  relevant `MANUAL.md` chapter if a register-level fact changed; and the
  `Status:` line (and any claim it makes) of the `docs/<name>-plan.md` the
  work closes out, if there is one. A repo-wide documentation audit found
  `Status: not started` lines on modules that had shipped long ago, and
  cross-references pointing at sections that no longer existed, as the
  norm rather than the exception, exactly because this step kept getting
  skipped. Don't repeat that: docs are part of the deliverable, not an
  afterthought.
- Non-trivial work gets a plan doc first: `docs/<name>-plan.md`, a
  `Status:` line, explicit **solved vs. pending** framing when design
  validation happens before implementation.
- Before trusting an uncertain compiler or hardware behavior (bank
  placement, inline-asm symbol binding, timing), write a throwaway probe
  and inspect the generated `.s`/`.map` rather than assume from the
  datasheet alone. This repo does that repeatedly (`pic8-math`'s XC8
  round-trip probe is the canonical example) and it has caught real wrong
  assumptions every time it's been tried.
- **No em-dashes (—).** Not in docs, not in commit messages, not in code
  comments. Use a comma, a colon, or a period and a new sentence instead.
