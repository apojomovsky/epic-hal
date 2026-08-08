# README rewrite: consumption-first, users of the v0.1.0 releases

Status: **approved 2026-08-08, in progress**.

## Problem

The root `README.md` is organized around the maintainer's view of the
project: a long "Why" section, host/Docker/real-hardware development
workflows, a full module catalog, and a deep Development section. Its
primary audience should be the people the v0.1.0 release pipeline now
serves: a user who downloads a per-family bundle and wants to build
against it, ideally from MPLAB X. The current README buries that flow.

## Goal

Rewrite the root README from scratch, reusing the header block (logo,
slogan "Built down to what the datasheet requires.", badges), organized
around **usage**. Contributing and development internals are demoted to
a short section and a linked-out `DEVELOPMENT.md`.

Decisions (user-approved):

- Audience: practicing engineers first, with an expandable path for
  newcomers to MPLAB X.
- Quick start leads with the MPLAB X reference project; the bare-make
  flow is the second path.
- Modules: a capability-grouped "what you can build" showcase plus the
  full catalog table kept as the reference.
- Philosophy: keep and sharpen the datasheet-faithful pitch; tie it to
  user value (no vendor HAL lock-in, host-first testing, one API across
  families).

## Target structure

1. **Hero** - kept verbatim: centered logo (dark/light variants),
   `# Epicurus`, slogan, badges (license, toolchain, CI). Add a release
   badge pointing at the latest GitHub Release.
2. **Pitch** - one tight paragraph plus 4-5 specific bullets (register
   level and datasheet-cited; one API across PIC16F87XA / PIC18Fxx5x /
   PIC16F193X; drop-in modules; ships as release bundles with a
   ready-to-open MPLAB X project; host-first testing).
3. **Quick start (5 minutes)** - the first-class flow:
   - MPLAB X path: download the bundle for your part (link to
     Releases), open `examples/epicurus-demo.X`, pick your part, Build,
     program. An expandable "New to MPLAB X?" note.
   - Bare make, no IDE: `EPICURUS_DIR` / `EPICURUS_MCU` /
     `EPICURUS_MODULES` + `include epicurus.mk`, a 6-line Makefile, one
     build command.
   - Add to an existing project: link to the bundle's `MPLABX.md`.
4. **Usage** - 3-4 short examples with real, verified API calls, each
   with a one-line "what this shows":
   - 500 ms LED blink on RB0 (GPIO + `epic_tick`) - the proven demo
     project code, cleaned for display.
   - UART echo / `printf` retarget (`epic_serial_init` /
     `epic_serial_available` / `epic_serial_read` / `epic_serial_write`).
   - Cooperative scheduler: `task_manager_init` + `task_spawn` +
     `task_manager_attach_timer0` + `task_manager_run`.
   - ADC oversample + moving average (`EPIC_ADC_*` +
     `epic_adcfilter_*`).
   Each example notes that the same source builds unchanged against any
   of the three families.
5. **What you can build** - modules grouped by capability (Timing &
   control, Communication, Storage, Math, Peripherals) with one-line
   blurbs; then the full catalog table kept below as the reference.
6. **Supported devices** - compact three-family table (parts, HAL,
   peripheral coverage).
7. **Documentation** - links to `epic-common/MANUAL.md`, family
   manuals, key docs.
8. **Contributing** - short, at the bottom: link to `AGENTS.md` and
   the plan docs, one paragraph. No how-to.
9. **License** - MIT, datasheet links as today.

## What moves out

- The current **Development** section (native bootstrap, Docker
  workflow, `make` targets, CI design) moves to a new `DEVELOPMENT.md`
  at the repo root, linked from the README's contributing/docs area.
  Content is copied, not rewritten, except for fixing any stale
  references found along the way.

## Example authenticity

Every code block must compile against the real API. Before finalizing,
verify each snippet by compiling it as a host program (the host sim
build compiles the same headers). The blink example is already proven
by `examples/epicurus-demo-*.X/main.c`.

## Verification

- Every relative link in the new README resolves (checked with a link
  scan over the markdown).
- The example snippets compile (host build smoke check).
- `PRE_COMMIT_BASE_REF=master scripts/pre-commit-checks.sh` passes
  (whitespace, no em-dash, cppcheck).
- Rendered preview reviewed for layout sanity (headers, code fences,
  badge alignment).

## Out of scope

- No changes to code, docs other than the README and the new
  `DEVELOPMENT.md`, or the release pipeline.
- No new content for contributing/development workflows; those are
  moved, not expanded.
