# Prompt for an external agent: implement the distribution plans

Copy everything below the line into the other agent. It's self-contained.

---

You're working in `/home/alexis/projects/epicurus`, a C99 HAL and tooling library for 8-bit PIC microcontrollers, branded "Epicurus." Read `AGENTS.md` at the repo root first: it documents the project's conventions (Conventional Commits, plan-doc-first for non-trivial work, no em-dashes, update the docs a change touches before calling it done) and you must follow them.

## What you're building

The library currently has no way for anyone outside this repo to consume it. Every real-target build lives in one of 29 hand-maintained `mcu/*-mplabx/Makefile`s with hardcoded sibling-relative paths (`EPIC_DIR ?= ../../../pic16f87xa-hal`), so the only way to use any of this is to clone the whole repo and work inside its layout.

You are replacing that with a declarative manifest plus a build driver, then generating per-family source bundles published as GitHub Release assets.

## Read these first, in this order

1. `docs/superpowers/specs/2026-08-05-distribution-design.md`, the agreed design and the reasoning behind every decision. Read it fully before writing any code.
2. `docs/superpowers/plans/2026-08-05-manifest-and-build-driver.md`, plan 1 of 3 (11 tasks).
3. `docs/superpowers/plans/2026-08-06-bundle-generator.md`, plan 2 of 3 (7 tasks).
4. `docs/superpowers/plans/2026-08-07-mplabx-projects-and-release.md`, plan 3 of 3 (7 tasks).

Also worth reading, because plan 1 changes where their content is tracked: `docs/mplabx-link-gaps-plan.md` and `scripts/ci-discover-xc8-matrix.py`.

## Execution

The plans are sequential and must be done in order. Plan 2 imports plan 1's `scripts/epicmanifest.py`; plan 3 extends plan 2's `scripts/make_bundle.py`. Do not start plan 2 until plan 1's "Done when" section is fully satisfied.

Within a plan, work task by task, in order. Each task is written TDD-style with the actual test code and the actual implementation code, exact commands to run, and the exact expected output. Follow the steps as written:

1. Write the failing test.
2. Run it and confirm it fails for the stated reason.
3. Write the implementation.
4. Run the test and confirm it passes.
5. Commit.

If a step's expected output does not match what you actually get, stop and investigate rather than adjusting the test to match reality. The plans were written against the real repo, so a mismatch usually means something about the repo changed or an earlier task was completed incorrectly.

Commit after each task. Do not batch tasks into one commit.

## Constraints that will bite you if you ignore them

- **No em-dashes.** Not in docs, not in commit messages, not in code comments. A pre-commit hook rejects the commit. Use a comma, a colon, or a period and a new sentence.
- **The toolchain container has no python3.** `docker/ci-toolchain/Dockerfile` is `debian:12-slim` plus `ca-certificates curl unzip make tar`, GTK runtime libs, and `cmake build-essential`. This is deliberate. It is why the build driver emits a POSIX `sh` script instead of calling `xc8-cc` directly: resolution runs where python3 exists (your host, or the GitHub runner), execution needs only `sh`. **Do not "simplify" this into a direct call, and do not modify the Dockerfile.** Adding python3 there would make every build depend on a human-gated `make ci-image-push`, since CI never builds that image.
- **Python 3.11 minimum, standard library only.** `tomllib` is stdlib from 3.11. Do not add any third-party dependency. Tests use `unittest`, not `pytest`, because this repo has no Python test infrastructure and no dependency install step.
- **Do not fix any `KNOWN_BROKEN` build failure.** 40 of 112 `(module, MCU)` legs genuinely fail to link today. They move into the manifest as `excluded` entries with reasons, and they stay failing. Repairing them is a separate project with open design questions, documented in `docs/mplabx-link-gaps-plan.md`. If you find yourself tempted, don't.
- **Do not change the host CMake/ctest build or `.github/workflows/host-tests.yml`.** The host side stays exactly as it is.
- **Files end with a trailing newline and have no trailing whitespace.** The pre-commit hook enforces both.

## You do not need XC8, MPLAB X, a licence, or hardware

Both verification gates run in CI against an already-published toolchain image. Your loop is: push the branch, read the workflow summary, fix, push again. Do not block trying to install a Microchip toolchain locally.

The one exception is plan 3, Task 2, which needs a human at an MPLAB X GUI to create three `.X` projects. `nbproject` XML is generated and hand-writing it produces projects that open but misbehave. When you reach that task, stop and hand it back rather than attempting it.

## The correctness argument you must not weaken

Plan 1's migration is gated on **byte-identical `.hex` output**: for every `(module, MCU)` pair that builds today, the old Makefile path and the new manifest path must produce the same file, byte for byte. Identical sources, identical flags, one compiler, so any difference means the manifest disagrees with the Makefile it replaces.

This is why plan 1 tells you to bootstrap the manifest mechanically (asking GNU make for its own resolved variables) rather than transcribing 29 source lists by hand, and why the Makefiles are deleted family by family only after that family's pairs all match. Do not delete any Makefile before its gate is green. Do not relax the comparison to "it still builds."

Plan 2 has the equivalent gate for bundles: each one is copied to a scratch directory **outside the repo** and built there, because building in place would let a file missing from the bundle quietly resolve back through the repo's sibling layout.

## Judgment calls the plans leave to you

Three items in plan 1, Task 3, Step 3 cannot be extracted mechanically and need you to check the repo rather than guess: each module's `depends_on`, the transcription of `KNOWN_BROKEN` into `supported`/`excluded` pairs with reasons, and the example names. The plan tells you where to verify each. A wrong `depends_on` shows up as a hex diff, so it gets caught, but checking first is cheaper than debugging.

If you hit something the plans genuinely did not anticipate, say so and stop. Do not invent a design decision and proceed silently: the spec records why each choice was made, and a change to any of them needs to go back to the repo owner.
