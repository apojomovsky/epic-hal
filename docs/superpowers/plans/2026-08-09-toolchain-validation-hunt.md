# Active validation hunt: exercise every pre-toolchain-era module under real XC8 + MPLAB SIM

Status: **draft 2026-08-09, pending approval**.

## Problem

The original pieces of this project (epic-math's inline-asm backends, the
three HALs' peripheral drivers, and the module library) were written and
validated without the real compiler and simulator at hand: logic was
proven on the host sim, and real targets were only ever *compiled*, never
*run*. The recent sessions proved this gap is load-bearing: every real
bug found so far (the `pic_select_bank` write misdirection, the `stringdir`
PCLATH clobber, the MPLAB SIM GIE race, the TXIF dispatch wedge, the
interrupt-chain page scatter, the epic-math bank-1 fixup overflow) lived
in code that compiled and host-tested clean. The same gap almost
certainly hides more.

Today only 4 mdb (MPLAB SIM) gates exist: epic-tick 16F877A/18F4550,
epic-pic16f193x-firmware 16F1937, epic-swuart 16F877A. Everything else
real-target code has zero runtime coverage.

## Goal

1. Give every module a self-reporting sim example and an mdb gate, so its
   real compiled code runs under XC8 + MPLAB SIM and reports
   PASS/FAIL, permanently, in CI.
2. Probe the known fragile patterns at register level (the patterns that
   produced every real bug so far).
3. Fix whatever surfaces, add host tests for logic bugs, and leave the
   gates as permanent regression coverage.
4. Close the documented open items (Finding 2, `EPIC_SSP_ReadByte`'s
   missing bank select, the `stringdir` handler hardening).

## Coverage matrix (today)

| Module | Host tests | Real-target compile | mdb gate | Pattern probes |
|---|---|---|---|---|
| epic-math (PIC16 asm) | yes | yes | no | no |
| epic-math (PIC18 asm) | yes | yes | no | no |
| epic-serial | yes | yes | no | no |
| epic-taskmgr | yes | yes | no | no |
| epic-modbus | yes | yes | no | no |
| epic-bus | yes | yes | no | no |
| epic-console | yes | yes | no | no |
| epic-settings | yes | yes | no | no |
| epic-adcfilter | yes | yes | no | no |
| epic-debounce | yes | yes | no | no |
| epic-pid | yes | yes | no | no |
| epic-encoder | yes | yes | no | no |
| epic-fsm | yes | yes | no | no |
| epic-lcd | yes | yes | no | no |
| pic16f87xa-hal peripherals | partial | yes | via tick/swuart only | no |
| pic18fxx5x-hal peripherals | partial | yes | via tick only | no |
| pic16f193x-hal peripherals | partial | yes | via firmware-sim only | no |
| epic-usb | yes | yes | no (USB not simulatable) | no |
| epic-sdcard | yes | yes | no (needs a device) | no |

## Known fragile patterns to probe (the "where the bugs live")

1. **Banked writes after `pic_select_bank(N)`** (Finding 9's class): 42
   call sites in pic16f87xa-hal alone. Finding 9 verified 8; the rest
   are unverified. The same pattern exists in the other two families'
   equivalents. Probe: run each site's write with a known value and read
   the SFR back under mdb.
2. **`GetFlag`/`ClearFlag` in ISR paths** (the `stringdir` PCLATH-clobber
   class): 11 handler files still route flag handling through the
   table-driven helpers. PR #9's dispatch fix + TXIE gate neutralized
   the tick's exposure; the other families and the remaining handlers
   are un-audited. Probe: ISR-path register/PCLATH checks under load.
3. **Unpinned bank-sensitive statics** (the epic-math `0xA0` class):
   audit every `volatile` file-scope SFR-adjacent symbol for a missing
   `__at` pin; the linker's best-fit is the failure mode.
4. **epic-math asm backends**: the PIC16 smoke runs one call per group;
   the PIC18 golden-vector selftest has never run under mdb. The asm
   bodies are hand-written and untested against the full vector set on
   real silicon.
5. **RAM margins**: the excluded legs (epic-swuart/epic-taskmgr
   16F873A/874A) were excluded for RAM; re-verify the exclusions still
   hold and refresh `docs/mplabx-link-gaps-plan.md` if reality moved.
6. **The documented open items**: Finding 2 (the bank-select helper
   itself), `EPIC_SSP_ReadByte`'s missing `pic_select_bank(1)`.

## Approach

**The mdb gate is the mechanism**: a self-reporting sim example (the
harness pattern the tick/swuart sims already use: bounded run, real API
calls, `EPIC_HARNESS_RESULT: PASS/FAIL` over UART, or GPIO on
PIC16F193X), a manifest sim variant, and a slot in the sim-gate list.
Every gate runs the module's *real* code under the *real* toolchain and
checks observable output.

## Phases

### Phase 0: Inventory and audit (static, no new gates)

- Build the coverage matrix above into a checked-in doc
  (`docs/toolchain-coverage.md`) with a per-module status column.
- Audit pass over all three families for pattern 1 (banked-write
  sites), pattern 2 (ISR-path flag handling), pattern 3 (unpinned
  statics), and catalog every hit with a file:line reference.
- Audit `EPIC_IRQ_Disable/Restore` users for the MPLAB SIM GIE-race
  exposure (the `epic_tick_get` class), beyond epic-tick
  (epic-encoder, epic-serial, epic-swuart, epic-taskmgr all use them).
- Deliverable: the audit report, which doubles as the probe checklist.

### Phase 1: The sim-example template and infra

- Extract the self-reporting sim-example pattern from the tick/swuart
  sims into a documented template (bounded `main`, `epic_harness_*`,
  PASS/FAIL criteria, wait_ms/mode conventions).
- Add manifest sim variants + examples for each uncovered module. Per
  module, the sim example exercises its real API paths:
  - **epic-math**: PIC18 gets the full golden-vector replay (adapt
    `tests/target_selftest.c` into a sim variant); PIC16 gets a curated
    subset that fits flash/RAM (boundary + representative values per
    op, results checked against the host oracle's expected table).
  - **epic-serial**: TX path + `printf` retarget, verifying UART
    capture; RX via the sim's register injection if feasible, else
    documented TX-only.
  - **epic-taskmgr**: the multi-blink example bounded by the harness
    (already harness-shaped) with the PASS criteria it already has.
  - **epic-modbus**: a request/response round-trip over the sim UART
    (TX a request, verify the framed response in the capture).
  - **epic-bus**: an I2C/SPI register-read/write sequence, verifying
    the MSSP/SSP register traffic.
  - **epic-console**: a command line through the serial dispatcher,
    verifying the echo and the dispatched handler output.
  - **epic-settings**: write a blob, reset the sim, read back, verify
    CRC validation and defaults.
  - **epic-adcfilter**: drive the ADC model, oversample+average, verify
    the filtered value against the oracle.
  - **epic-debounce**: feed pin transitions, verify the debounced
    output against the expected sequence.
  - **epic-pid**: a step response against a reference trajectory,
    verifying the Q8.8 arithmetic under the real toolchain.
  - **epic-encoder**: a quadrature sequence, verifying the x4 count.
  - **epic-fsm**: a state walk, verifying every transition.
  - **epic-lcd**: drive the HD44780 sequence, verifying the nibble
    writes by reading the GPIO/SPI registers (the sim has no display).
- **epic-usb** and **epic-sdcard**: MPLAB SIM cannot simulate USB, and
  SD needs a block device. These get host-side logic tests (Phase 3)
  and a documented exclusion from mdb coverage, with the reason.
- Wire every new sim variant into `scripts/ci-target-sim.sh` (or a
  manifest-generated list) with per-family wait_ms/mode.

### Phase 2: Register-level pattern probes

For each fragile pattern, a dedicated probe program (throwaway, run
under mdb with register checks, or a committed probe that prints
register state to the UART):

1. **Banked-write correctness**: iterate the audited `pic_select_bank`
   sites; for each, write a known byte via the module's own API and
   read the SFR back under mdb. Expected: the value lands; any site
   that corrupts is a Finding-9-class bug (fix with the
   `EPIC_BANK*_WRITE8` pattern).
2. **ISR-path integrity**: a timer-driven load (like the tick sim) that
   checks INTCON/PIR/PCLATH state at halts, per family. This is the
   PR #9 methodology, extended to the un-audited families and the
   remaining `GetFlag`/`ClearFlag` handlers.
3. **Fixup-overflow sweep**: already implicit in the build; the new
   gates make the runtime side explicit.
4. **The open items**: a dedicated probe for Finding 2 (the bank-select
   helper's own write) and for `EPIC_SSP_ReadByte` (read after a known
   write, verify the value). The `stringdir` hardening is the handler
   refactor from PR #9's follow-up: convert the remaining 11 handler
   files to the direct-flag pattern once the gates prove the layout is
   stable.

### Phase 3: Fixes and new tests

- Every bug the gates or probes surface: fix at the source (the
  established patterns: `EPIC_BANK*_WRITE8`, direct flag clears,
  `__at` pins, read-retry, PCLATH handling).
- Host tests for logic bugs found (the modules' existing ctest
  suites; add cases that reproduce the bug).
- Keep every new sim gate as a permanent regression in CI.

### Phase 4: Verification and documentation

- Full verification via the existing `make target-ci` (extended matrix:
  all gates + all builds) and the host suite.
- Update `docs/toolchain-coverage.md` (all green), the HAL
  `ARCHITECTURE.md` findings, and `docs/mplabx-link-gaps-plan.md` if
  the RAM-margin re-check moves anything.

## Acceptance criteria

- Every module with a simulatable real-target path has an mdb gate that
  passes in CI (epic-usb/epic-sdcard documented exclusions with
  reasons).
- The pattern probes are run and their findings recorded; every Finding
  9-class or ISR-path hit is fixed or explicitly documented as
  low-risk.
- The three documented open items are closed (Finding 2 resolved or
  conclusively refuted, `EPIC_SSP_ReadByte` fixed, stringdir hardening
  landed).
- No regressions: full host suite, all mdb gates, all 84+ real-target
  builds green.
- New host tests exist for every logic bug found.

## Risks

- **Effort**: 13 new sim examples + 3 probe suites is substantial;
  each example is real code that must be written, verified, and
  maintained. Mitigation: the template + the harness pattern keep each
  example small; Phase 1 can land incrementally (a few modules per
  commit).
- **CI time**: more gates means a slower target job. Mitigation:
  per-gate wait_ms tuned to the minimum confirmed-passing budget (the
  existing gates run 5-60 s each; a handful of new ones is acceptable).
- **Sim fidelity**: some modules (LCD, bus, encoder) depend on the
  sim's peripheral model; a probe may find a *sim* bug rather than a
  firmware bug. Distinguish and record both; a sim-model bug is still a
  finding (the sim is the verification vehicle).
- **Peripheral-bound modules**: modbus/serial RX injection and the
  SD/USB exclusions mean some paths stay hardware-only. Recorded as
  documented coverage limits, not silent gaps.
