# Plan: HAL-4, the epic-cc sim gate (epic-hal#60)

Replace the mdb/MPLAB SIM gate on the epic-cc path with epic-cc's own
simulator (`crates/sim`), making the `epiccc-gate` CI job fully public.
The XC8 + mdb jobs stay as the differential oracle.

## What shipped

1. `scripts/sim-runner/`: a minimal Rust bin crate, path-dependent on
   `<repo>/epic-cc/crates/sim` (CI checks the pinned epic-cc out there;
   locally `ln -s ~/projects/epic-cc epic-cc`). No deps beyond
   `pic14-sim`. It loads a hex, resolves the part via
   `device::resolve`, raises the firmware's interrupt source the way
   the peripheral would (flag latched through its enable bit, vector
   taken when GIE allows), and requires the watched SFR bit to toggle
   across step-counted samples: the mdb toggle contract.
2. `.github/workflows/ci.yml` `epiccc-gate`: keeps the public build
   steps (pinned driver from source, sha-verified v0.0.3 clang bundle),
   re-pins `EPIC_CC_PIN` to epic-cc master at the HAL-3 close-out
   (c64440e), builds {877A, 887} x {blink, tick}, gates blink on both
   devices in crates/sim, and no longer touches docker at all.
3. `Makefile` `sim-epiccc`: the same gate locally, inside the epic-cc
   dev image (the host has no Rust toolchain).
4. Docs: DEVELOPMENT.md "The sim gate (HAL-4)" + pin paragraph
   rewritten; AGENTS.md CI paragraph updated.

## Findings recorded upstream (blockers for the rest of the matrix)

- epic-cc#173 (filed this session, with a minimal repro and an mdb
  differential): crates/sim drops banked SFR writes; PIE1 (0x8C, bank
  1) never reads back as written. Blocks the tick sim gates (their
  enable bit is PIE1:TMR2IE); a config change here once it lands.
- epic-cc#125/#126/#143 (pre-existing): the 4550 slice still stops on
  the i64 aggregate-copy, bool-trunc and indirect-memcpy isel gaps.
  Reproduced from the current master driver during this session; the
  4550 smoke waits on those.

## Verification

- Both blink gates pass locally via `make sim-epiccc` (deterministic:
  step-counted, not wall-clock).
- The tick hexes verified correct against the mdb oracle (PIE1=0x02,
  INTCON=0xC2, PIR1=0 under MPLAB SIM), isolating the failure to the
  simulator, which is the #173 evidence.
- Host suite via `make pre-pr-check TEST=1`.

## Ephemeral

This plan is deleted in the final commit before merge; durable facts
live in DEVELOPMENT.md and AGENTS.md.
