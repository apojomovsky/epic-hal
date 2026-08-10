# Quality roadmap: top 5 tasks

Status: **reference document, 2026-08-10**. Ranked by risk-reduction per
effort, grounded in what the validation-hunt and combination-matrix
campaigns (PR #12/#13) actually uncovered. Tasks 2-4 and 6-7 are done
as of 2026-08-11 (see per-task Status lines); the extended list after
the honorable mentions tracks the newer tasks.

## 1. Real-silicon bring-up pass (the compatibility task)

**Problem**: every mdb gate validates against MPLAB SIM, and the
campaigns documented the model's gaps as walls: no RX injection of any
kind, the 16F877A data-EEPROM model inert (writes never complete, RD
never refreshes EEDATA), the MSSP data path unmodeled, TXIF never
clears on TXREG write, TMR0IF unreliable with the prescaler on TMR0,
and the ISR wedge classes (GIE-on with a pending interrupt, the
polled-TX wait, unbounded tick spins). Gates that passed against those
gaps (EEPROM reads on 16F877A, every RX path, the MSSP master-side
traffic) are codegen-correct but silicon-unverified. The RX paths
(epic-serial RX, console parser, modbus RX framing, swuart RX) have
zero real-toolchain coverage at all.

**Task**: a board bring-up campaign using the MPLAB X reference
projects (PR #5): run the gate scenarios on real silicon, starting with
the tick/serial/settings/encoder examples, following
docs/adding-a-device.md section 4's debug protocol. Turn every
documented sim wall into a hardware-verified behavior or an explicit
silicon-pending item.

**Why**: the only task that directly de-risks "works in the sim, wrong
on the board". Everything else improves sim-validated code; this one
validates the validation.

## 2. Complete the class-G GIE-race conversion

**Problem**: epic_tick_get was converted from Disable/Restore to
read-twice-retry after Finding 10.1 (the sim vectors inside GIE=0
windows, tearing reads and leaving GIE cleared after ISR return; the
same hazard class applies on real silicon semantics). The same pattern
remains in epic_serial (including a Disable/Restore inside the TX ISR
callback), epic_swuart (3 sites), epic_taskmgr (10 sites), and
epic_encoder (3 sites). These are the documented real-hardware
hang/tear class.

**Task**: convert the remaining sites to lock-free or retry patterns
(read-twice-retry for multi-byte reads; single-byte atomic stores for
the writes; remove unnecessary ISR-context Disable/Restore). The
existing gates exercise most sites, so each conversion is verifiable.

**Why**: direct reduction of real-silicon unexpected behavior; the
pattern is proven and the conversion is mechanical.
Status: done (2026-08-11, PR #16). All epic-serial/epic-swuart/
epic-encoder/epic-taskmgr sites converted to read-twice-retry or
publish-last single-byte disciplines; task_start/task_reset/
task_set_period retain documented critical sections (a retry cannot
atomicize a multi-byte TCB write).

## 3. The stringdir ISR-handler conversion (class F)

**Problem**: 10 PIC16F87XA and 4 PIC16F193X handlers still route
EPIC_IRQ_GetFlag/ClearFlag through the ROM-descriptor table, the
stringdir/retlw path that clobbers PCLATH when the table sits on
another flash page. Documented latent hazard from the dispatch work;
the CCP1/CCP2 direct-flag pattern (EPIC_BIT_CLR on the literal PIR
register) is the proven-safe reference.

**Task**: convert the handlers to direct flag ops (compile-time
register tokens, no table), keeping the API surface identical. The
gates are the regression net.

**Why**: closes the last known PCLATH hazard in the ISR path,
mechanically, with full regression coverage.
Status: done (2026-08-11, PR #16). All 14 handlers (10 87XA, 4 193X)
converted to direct flag ops; the CCP1/CCP2 direct-flag pattern is now
the family norm.

## 4. Host-sim property and fuzz testing

**Problem**: the mdb gates are single-scenario by design (bounded,
deterministic), so they cannot probe the state spaces where logic bugs
hide. The host sims are instant, so the cost of broad testing there is
near zero.

**Task**: extend the host suites with property and randomized tests:
ring-buffer stress, scheduler invariants (task counts, period ratios,
one-shot semantics under random spawn/kill), settings CRC round-trips
with random blobs, algebraic identities on the math fixed-point ops,
console parser fuzzing (random byte streams must never wedge the line
state machine).

**Why**: cheapest bug-finding per unit effort in the list; the host
tests run in seconds, so the property space is bounded only by what we
write.
Status: done (2026-08-11, PR #20). Seeded-deterministic property and
fuzz tests landed for epic-math, epic-serial, epic-taskmgr,
epic-settings, epic-console, and epic-swuart (algebraic identities,
ring stress, scheduler invariants vs a semantics model, CRC
round-trips + corruption sweeps, line-machine fuzzing, TX/RX ring and
error-count fuzzing); every test was mutation-verified against a
deliberately broken implementation.

## 5. Deterministic-layout hardening

**Problem**: the XC8 layout lottery cost more session time than any
single bug: the 1356 fixup overflows (epic-math PIC16, epic-pid), the
flash-page-0 ISR-body constraint, the 8-level stack overflows (lcd),
the __at pins that fixed the dispatch and the math scratch. A small
codegen shift (one dispatch local) silently moved a passing gate into a
wedged layout.

**Task**: (a) apply the recorded __at pinning follow-up to the PIC16
math asm routines (enabling the full golden-vector PIC16 replay), (b)
document the layout budgets (page-0 ISR body, hand-asm page
constraint, stack depth) in one place, (c) add a CI check that the gate
.hex files are byte-identical across a rebuild, so layout drift shows
up as a reviewable diff instead of a flaky gate.

**Why**: removes the class of failure that has produced the most
churn, and makes the gate suite stable against future codegen changes.

## Honorable mentions

- epic-usb / epic-sdcard real-target manifest entries (compile-only
  today; host tests run in CI).
- Pin the 20 unpinned IRQ-shared statics (the audit's RISK set in
  docs/toolchain-coverage.md).
- A firmware-in-the-loop RX harness for the modules whose RX paths the
  sim cannot reach (epic-serial RX, console, modbus RX, swuart RX).

## Extended list (2026-08-11, from the follow-up discussion)

6. **SFR-map audit against the DFP** - the PIE2 bug was a misread
   memory map (PIE2 = Bank 1, "fixed" to Bank 2). A mechanical script
   that cross-checks every SFR address/bit constant in the HALs
   against the DFP headers kills that bug class. Cheap, and would have
   caught the regression before the combination matrix did.
   Status: done (2026-08-11, scripts/sfr-map-audit.py in CI's target
   job, `make audit` locally). First run caught three real bugs: the
   TRISE IBF/OBF/IBOV layout was the 16C74-era one (IBF=1, OBF=2,
   IBOV=3) instead of the 16F87XA's (IBF=7, OBF=6, IBOV=5); a bogus
   PIC_EECON1_EEIF (EECON1 has no EEIF, the flag is PIR2<EEIF>); and a
   bogus PIC_PIR2_OSFIF (PIR2 bit 7 is unimplemented on the 87XA; OSF
   is 193X-only). All 14 MCUs now match the DFP.
7. **Config-key audit** - the PIC16/PIC18 config-key mixup silently
   broke 4 matrix legs. A CI check that each manifest example's config
   keys exist for the target MCU's DFP config options.
   Status: done (2026-08-11, scripts/config-key-audit.py in CI's
   target job, `make audit` locally). XC8 validates pragmas at link
   time, so the audit links each example's config TU with a trivial
   main per supported MCU and fails on diagnostic 1363; 149 config TUs
   link clean, and the planted-key repro proves the gate catches a
   mixup.
8. **The RX wall, via a target-in-the-loop harness** - the sim cannot
   inject RX; the only real-toolchain RX coverage is firmware talking
   to a host through the actual UART (or the USB CDC path). Largest
   uncovered surface.
9. **Pin or verify the 20 unpinned IRQ-shared statics** (the audit's
   RISK set in docs/toolchain-coverage.md) - the multi-byte struct
   copies are the scatter-sensitive ones.
10. **Gate flake hardening** - several gates documented the sim-wedge
    landing zones; a CI repeat-run (each gate N times) surfaces the
    remaining flakes and drives the wedge-proofing pass.
