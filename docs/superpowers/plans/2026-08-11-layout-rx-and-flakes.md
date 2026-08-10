# Plan: layout hardening, statics pinning, RX harness, flake hunt (quality tasks 5, 8-10)

Date: 2026-08-11. Status: in progress.

Executes quality-roadmap tasks 5, 8, 9, 10 (tasks 6-7 landed 2026-08-11,
PR #18). Task 5 has three parts: (a) PIC16 math asm pinning for the full
golden-vector replay, (b) the layout-budget doc, (c) the hex-rebuild
identity check. All four tasks ship as permanent gates where a script or
a CI step can be, per the campaign convention (gates become regression
nets, never paper over).

## Task 9: pin or verify the 20 unpinned IRQ-shared statics

### Solved

- The RISK set (docs/toolchain-coverage.md section H) is hand-audited:
  14 symbols on 87XA, 6 on 193X, all in banked GPR, linker best-fit,
  nothing pinned except the two scratch bytes (pic16_mscratch common
  RAM 0x70-0x7F and the CCP callbacks). The multi-byte struct copies
  (g_t0_storage, g_ccp_callbacks[3], g_handle[5], g_t1_handle, g_usart)
  are the worst scatter cases; the CCP handle fragility was already
  removed at the source (PR #19, driver-owned callbacks), the class
  remains for the rest.
- The check must be mechanical, like the SFR audit: a script that scans
  the banked HALs and module sources for file-scope mutable statics,
  classifies single-byte vs multi-byte, determines IRQ-sharing
  (referenced from a dispatch-reachable handler and from main-line
  code), and verifies placement from a built .map (bank-resident, no
  __at collisions, multi-byte objects never cross a bank boundary).

### Design

1. `scripts/statics-audit.py`: host python3, scans the two banked HALs
   and the module sources (the manifest's source lists, so the scan
   tracks the real builds), emits the IRQ-shared statics table with
   type-size classification. Options: `--map <file>` additionally
   checks the .map of a built hex: each shared static's address, bank,
   and bank-crossing for multi-byte objects.
2. Pin the genuinely risky objects with `__at()` into a documented
   safe region per family (verify no collisions with the existing pins:
   pic16_mscratch 0x70-0x7F, CCP callbacks at 0x140). Prefer
   bank-1/2 GPR below the SFRs, chosen so a rebuild cannot move the
   object across a bank or into an ISR-baked IRP window.
3. The audit runs in CI (target job, next to the SFR/config audits) and
   in `make audit`.

### Verification

- statics-audit passes on the built matrix hexes (or the gate hexes).
- Full matrix + the 32 mdb gates stay green (the pinning changes .hex
  bytes, so the gate suite is the regression net).

## Task 5a: PIC16 math asm pinning, full golden-vector replay

### Solved

- The PIC16 math gate runs the smoke (target_smoke16.c) because the
  full golden-vector selftest (target_selftest.c) overflows the layout
  budget: past ~4.2 K words the linker scatters the hand-asm routines'
  internal gotos across pages and aborts with fixup-overflow 1356. The
  selftest is deliberately PIC16-sized (golden_vectors.h is 3.8 KB; the
  fail-report function keeps ~150 CHECK_EQ sites small) and ends with
  epic_harness_report(g_fail == 0), the exact marker the mdb gate
  greps.
- The fix: pin the PIC16 asm leaf routines below 0x800 (page 0) with
  the XC8 function-placement mechanism, so their internal gotos are
  page-local regardless of how much C code surrounds them, then make
  the selftest the PIC16 example and let the existing epic-math 16F877A
  mdb gate run the full replay.

### Design

1. Pin the five asm-leaf files' routines (pic_math_addsub.c,
   pic_math_bcd.c, pic_math_div.c, pic_math_mul.c; scratch stays as
   is) below 0x800. Empirically confirm the XC8 v4.00 placement
   mechanism (function __at() vs #pragma code) against the generated
   .map, and that the pin range fits with the ISR vector area and the
   rest of the page-0 budget.
2. Switch the PIC16F87XA manifest example from target_smoke16.c to
   target_selftest.c; delete the smoke file if nothing else uses it.
3. The sim gate for epic-math 16F877A then runs the full replay; set
   its wait_ms budget empirically (PIC16 has no hardware multiply, the
   sim is slower than PIC18's 5000 ms).

### Verification

- 16F877A math mdb gate: EPIC_HARNESS_RESULT: PASS with the full
  vector replay (not just the smoke), all golden tables run.
- Matrix stays green for all four 87XA math legs.
- docs/ARCHITECTURE.md testing tiers + toolchain-coverage.md updated.

## Task 5b: layout-budget doc

One place (docs/layout-budgets.md) collecting the recorded budgets:
page-0 ISR body bound (~4.2 K words 87XA, PCLATH-less vector goto),
hand-asm internal-goto page constraint, the 8-level hardware stack
(ISR + dispatch depth, the lcd 5-level dispatch overflow), the CCP
IRP=1 handle-bank constraint (now moot at the source after PR #19, but
the pattern is recorded), PIC16 bank-2/3 pointer reachability, the
math scratch common-RAM pin, the TMR0/OPTION_REG bank rules, and the
pinned-region inventory (addresses in use, so future pins collide
loudly instead of silently). toolchain-coverage.md points to it.

## Task 5c: hex-rebuild identity check

`scripts/hex-identity-audit.py`: build each matrix hex twice into
separate dirs and sha256-compare the .hex files, so a codegen or
layout drift shows up as a reviewable diff instead of a flaky gate.
Runs in the CI target job and `make audit`. Empirically confirm XC8
v4.00 determinism first (the Makefile-to-epic_build migration was
verified byte-identical, so determinism is expected).

## Task 10: gate flake hardening

### Design

1. `scripts/ci-target-sim.sh` gains a REPEAT env (default 1): each
   gate runs N times, any run failing fails the gate, and the summary
   table gains a per-run breakdown. Backward compatible, so the CI
   call is unchanged and a local hunt is
   `REPEAT=5 bash scripts/ci-target-sim.sh`.
2. Flake hunt: run the documented wedge-class gates (16F877A live-ISR
   + polled-TX gates: adc-uart, rb-uart, tick-serial, encoder-tick,
   swuart-tick, uart-ssp, tick) REPEAT=5 locally, fix every surfaced
   flake at the source (wedge-proofing: the documented landing zones
   are TX-free run loops, GIE-alive assertions, bounded waits).
3. A workflow_dispatch-only CI job runs the repeat (REPEAT=5) so the
   full hunt is reproducible without a PR, without slowing the PR
   target job.

### Verification

- REPEAT=5 run of the wedge-class gates: 0 flakes after the fixes.
- PR target job unchanged (REPEAT unset).

## Task 8: RX harness (target-in-the-loop)

### Solved

- The sim wall is absolute (documented, evidence-backed in
   toolchain-coverage.md): RCREG/RCIF writes masked, the stim +
   packetin() path crashes the sim, pin waveform raises FERR without
   RCIF. RX logic IS injectable host-side (task 4 landed sim RX
   injection for epic-serial and swuart). The real-toolchain RX path
   can only be exercised by firmware talking to a host through the
   actual UART, which needs silicon. This task builds the harness and
   verifies every leg that can be verified without a board; the
   silicon leg is explicitly pending (task 1's domain).

### Design

1. New combo `tests/epic-combo-rx-loopback` (structure mirrors the
   other combos): firmware that enables USART RX (RCIE), implements
   line framing (accumulate to \n, echo the line back with a status
   prefix), emits a boot banner over TX, and keeps the run loop
   TX-free (the documented wedge-landing-zone rule).
2. Host-sim test in the module: RX injection through the sim hooks,
   byte-exact echo and framing checks (reuses the epic-serial hook
   pattern; verify the host sim supports RX injection for the new
   module the same way).
3. mdb gate entry in ci-target-sim.sh: TX side (boot banner) plus
   EXTRA_MDB register prints proving RX is enabled (RCSTA/SPEN/CREN,
   PIE1 RCIE), no RX under sim.
4. Host driver scripts/serial-rx-loop.py (pyserial): the framing
   protocol driver for a real UART, syntax-checked, hardware-pending.
5. Docs: toolchain-coverage.md RX-wall section gains the harness
   status; the module README documents the silicon leg.

### Verification

- New combo builds (matrix leg) and its mdb gate passes (TX side).
- Host-sim test passes (RX logic byte-exact).
- No existing gate changes.

## Execution order

1. Me (main checkout): task 9 audit script + pins, task 5a pins +
   PIC16 selftest replay, task 5b doc, then task 10 (needs the final
   layouts from 9/5a).
2. Agent (worktree): task 5c (hex-identity audit + CI).
3. Agent (worktree): task 8 (RX harness).
4. One PR per task group, merge-as-is when CI green; roadmap statuses
   updated on landing.

## Follow-up (recorded, not done)

- The silicon leg of task 8 (run serial-rx-loop.py against a real
  board) is task 1's domain.
- epic-usb / epic-sdcard real-target manifest entries remain.
- **CI target-job speed (2026-08-11, user note)**: the target job is
  the long pole (16+ min vs ~1 min host); investigate options after
  tasks 5, 8-10 land: the 112-build matrix (parallelize? per-family
  sharding? build cache?), the 32 mdb gates (sequential by design,
  wait_ms budgets), the audits (hex-identity doubles the matrix cost),
  and the single image pull. Capture the options, tradeoffs, and a
  recommendation in docs/superpowers/plans/2026-08-11-ci-speed.md.
