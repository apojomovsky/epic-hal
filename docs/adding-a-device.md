# Adding a new device: from datasheet to a verified implementation

Status: **living reference**, not a one-off task plan. Re-read this
whole document (agent or human) before starting any device-addition
work, and update it whenever a real addition teaches something this
version got wrong or left out. Supersedes `docs/multi-family-plan.md`'s
"How to add family #3" checklist as the operational procedure; that
document's own "Open questions" section stays as-is, it is a historical
record of specific decisions made for PIC18, not a checklist.

## Why this document is stricter than it looks

Every peripheral driver bug found in this repo so far (the XC8 codegen
sections of `pic16f87xa-hal/README.md` and `pic18fxx5x-hal/README.md`)
was invisible to code review, to
the host simulator, and to a clean `xc8-cc` compile-and-link. Every one
of them was only caught by actually running the compiled firmware under
`mdb` (MPLAB SIM, headless) and reading back real register values. Two
completely different, family-specific compiler misbehaviors (PIC16's
bank-switch misdirection, PIC18's runtime-address-to-flash-table
misdirection) both had the same shape: code that looks correct, builds
clean, links clean, and silently writes the wrong thing to a register
at runtime. Nothing short of running it catches this.

This is why the verification gate below (§4) is mandatory for **every**
peripheral added, no exceptions, even ones that look too simple to get
wrong. "It compiled" and "the host sim passed" are necessary, not
sufficient. A peripheral is not done until a real `mdb` register read
confirms it.

## How to use this document

You (agent or human) are expected to drive this mostly unattended once
started, but there are real checkpoints where you must stop and ask the
user rather than guess:

- After §1 (datasheet sourced, device identity confirmed): confirm the
  part number, package, and supported-variant list with the user before
  writing any code.
- After choosing Path A vs Path B (§2): confirm, since it changes the
  size of the remaining work by an order of magnitude.
- Any time the datasheet's prose and the DFP pack's machine-readable
  register data disagree, or the datasheet is ambiguous about a bit's
  behavior: flag it and ask, don't guess and move on. A guessed register
  bit is exactly the kind of thing the verification gate is designed to
  catch, but it's cheaper to just not guess.
- Before touching CI config, pushing anything, or deleting/renaming
  existing supported-variant lists: confirm with the user, per this
  repo's normal git-safety norms (see root `CLAUDE.md`).
- Before final sign-off: present the full verification checklist (§4,
  §6) filled in, not just "I think it's done."

Between checkpoints, work through the steps directly: write the driver,
build it, run it under `mdb`, read the registers, compare against
hand-computed expected values, fix if wrong, move to the next
peripheral. Don't ask permission for each individual step, that defeats
the point of delegating this.

**If you'd rather do this by hand instead of delegating to an agent**:
every step below still applies, in the same order, with the same
verification gate. Nothing here is agent-specific except the tone; a
human engineer following this document does the exact same builds and
the exact same `mdb` reads.

---

## §1. Source the datasheet and confirm device identity

1. Find the part's official datasheet on Microchip's site (`WebSearch`/
   `WebFetch` if you have them, otherwise ask the user for the
   `ww1.microchip.com` link). Record the DS number
   (`DS39582B` for PIC16F87XA, `DS39632E` for PIC18F2455 family), every
   citation in this codebase uses that number + section, e.g.
   `DS39632E §16.0`, and yours should too.
2. **Never download or commit the PDF.** `*.pdf` is gitignored on
   purpose (root `CLAUDE.md`). Reference the datasheet as a link to
   Microchip's own hosted copy. If you need to read it repeatedly while
   working, keep your own extracted notes in the scratchpad, not the
   PDF itself, and definitely not in the repo.
3. Cross-reference every register address and bit position you're about
   to transcribe against the toolchain's own machine-readable device
   data, not just the datasheet prose. The pinned XC8 image (see
   `docker/ci-toolchain/Dockerfile` for the exact tag) ships this per
   device:
   ```
   /opt/microchip/xc8/v<ver>/pic/packs/Microchip.<Family>_DFP/xc8/pic/include/proc/<part>.h
   /opt/microchip/xc8/v<ver>/pic/packs/Microchip.<Family>_DFP/edc/<PART>.PIC
   /opt/microchip/xc8/v<ver>/pic/packs/Microchip.<Family>_DFP/xc8/docs/chips/<part>.html
   ```
   `docker run --rm <image> bash -c 'find /opt/microchip/xc8/v<ver>/pic/packs -iname "*<part>*"'`
   to locate them (see `docs/ci-plan.md`'s Phase 2 for the established
   pattern of pulling files out of the toolchain image). If the part
   isn't in an already-pinned DFP pack at all, that's a real blocker,
   flag it to the user before going further, don't try to hand-roll
   register headers.
4. Confirm and record: exact part number(s) and package(s) in scope,
   pin count, RAM/flash/EEPROM size, peripheral set, and (critically)
   the addressing model (banked GPR + `pic_select_bank`-style RP0/RP1?
   linear + Access Bank like PIC18? something else entirely?) and
   interrupt architecture (single vector, no priority, like classic
   PIC16? dual vector with `IPEN`/priority bits, like PIC18? something
   else?). This is the input to §2's decision.

## §2. Decide the path: same family, or new family

**Path A (new variant, existing family)** if *all* of these hold
against an already-supported sibling in `pic16f87xa-hal/` or
`pic18fxx5x-hal/`:
- Same addressing model and same interrupt architecture (not just
  "similar", the same scheme, bit-for-bit compatible enough that the
  existing `pic16_irq.c`/`pic18_irq.c` register tables mostly still
  apply).
- Same peripheral module identities (Timer2 is Timer2, EUSART is
  EUSART), even if some are present/absent (that's normal, PSP is
  40/44-pin-only within PIC16F87XA already, `epic-tick`'s own family
  guard `PIC16F87XA_FAMILY_HAS_PSP` is the existing pattern for this).
- Register addresses for shared peripherals are the same, or
  differences are small enough to express as a per-MCU `#if` inside the
  existing driver, not a rewrite.

If any of those don't hold, it's **Path B (new family)**. Enhanced
Mid-range PIC16F1xxx is Path B even though it shares "PIC16" in the
name: it has its own BSR-like addressing distinct from classic PIC16
and PIC18 both (`docs/multi-family-plan.md`'s own note on this still
holds).

Go to §3 for Path A, §5 for Path B. Both paths end at §4 (the
verification gate) for every peripheral touched, and §6 (documentation
deliverables) before calling it done.

---

## §3. Path A: new variant, existing family

1. **Diff against the closest existing sibling**, not the family in the
   abstract: peripheral set (what's present/absent), RAM/flash/EEPROM
   size, and any register address deltas. Write this diff down before
   touching code, it's the actual scope of the work.
2. **Check the RAM/flash budget seriously, don't assume "smaller
   variant, same modules" is fine.** This repo has been burned by this
   exact assumption twice: `docs/mplabx-link-gaps-plan.md`'s Root cause
   2 (modules that never fit the smaller PIC16/PIC18 variants at all)
   and Root cause 3 (a single new `__at`-pinned byte tipping two
   already-marginal modules from "fits" to "genuine linker error"). If
   the new variant has less RAM or flash than the smallest currently
   supported one, expect some existing modules to need a `KNOWN_BROKEN`
   entry in `scripts/ci-discover-xc8-matrix.py` for it, not a silent
   assumption that everything still fits.
3. Add the MCU to every place that enumerates supported variants:
   - Each `mcu/*-mplabx/Makefile`'s `MCU` `ifeq` chain
     (`CFLAGS_DEVICE`).
   - `scripts/ci-discover-xc8-matrix.py`'s `PIC16_VARIANTS` /
     `PIC18_VARIANTS` list.
   - Any family-conditional macro whose applicability changes for this
     variant (`PIC16F87XA_FAMILY_HAS_PSP` and friends).
4. For every peripheral whose register layout differs on the new part
   (from §3.1's diff), update the driver with the minimal `#if`/`#ifdef`
   needed, citing the datasheet for the new part specifically, not
   inferring from the sibling's own citation.
5. Every peripheral, changed or not, goes through §4's verification gate
   for the new MCU, no exceptions (the whole point of "mandatory for
   every peripheral" is that "this part is basically the same as an
   existing one" is exactly the assumption that's failed before, on
   this exact codebase, more than once).
6. Full regression: rebuild every module, every already-supported MCU
   variant in the family, not just the new one, real-target and host,
   before considering this done. (`for mcu in ...; do make -C <dir>
   clean; make -C <dir> MCU=$mcu ...; done` over the full discovered
   matrix, matching what's actually been run after every fix this
   session, see recent commits touching `pic18_irq.c`/
   `pic18fxx5x_ccp.c` for the literal command shape.)

## §4. The verification gate (mandatory, every peripheral, every path)

This is the actual bug-prevention mechanism this whole document exists
to enforce. Run it exactly like this for every peripheral function
(or tight group of related functions) added or changed, before marking
it done.

1. **Implement the driver**, citing the datasheet section for every
   register, bit, and computed value. Match this repo's existing
   convention: family-neutral logic in `epic-common/`, register-specific
   bodies per family, same names/signatures across families (the
   "fixed contract" root `CLAUDE.md` describes).
2. **Extend the host-sim model** if the peripheral needs simulated
   behavior beyond a plain register (e.g. `pic16f87xa_sim.c`'s
   `sim_step_ssp()`-style peripheral stepping), so host tests can
   exercise real behavior, not just register plumbing.
3. **Write a host-testable example** that exercises the peripheral with
   *known* input values and asserts the *exact* expected register image
   or behavior, with the expected values computed and documented in the
   test's own header comment (see `pic18fxx5x-hal/tests/
   example_ccp_pwm.c`'s header for the exact style: "Expected register
   image: CCPR1L = 50 >> 2 = 0x0C ..."). If a suitable example already
   exists for this peripheral, extend it rather than writing a new one.
4. **Run the host test.** It must pass before moving on. This catches
   logic bugs; it does not catch the codegen-level bugs this whole gate
   exists for, don't stop here.
5. **Build for real target**, the pinned XC8 Docker image (resolve the
   tag from `docker/ci-toolchain/Dockerfile`'s `ARG`s the way
   `scripts/sim-test-local.sh` does), for **every** MCU variant in the
   family this peripheral applies to, not just one. A clean build is
   necessary, not sufficient, keep going.
6. **Run the same test under real `mdb`** (MPLAB SIM, headless). The
   `make mdb-test` recipe wraps `scripts/sim-mdb-run.sh`, which builds
   the `HARNESS=sim` firmware and drives `mdb.sh`. For families that
   report PASS/FAIL over UART (PIC16F87XA, PIC18Fxxxx), the default
   `MODE=uart` captures the `EPIC_HARNESS_RESULT` marker from the
   USART output file. For families without an EUSART driver yet
   (currently PIC16F193X, exercised via RA0/GPIO), pass `MODE=gpio`;
   the wrapper then reads the marker via `print PORTA` and checks bit
   0. The matching `epic_harness_log()` magic-string dispatch lives
   in `pic16f193x-hal/src/core/pic16f193x_harness_sim_target.c`.
   Use this exact protocol, established the hard way this session:
   - Use `stepi <N>` with a generous instruction count, **not**
     `run` + `wait`. `run`+`wait` does not reliably respect `break`-set
     breakpoints in this toolchain's headless mode (confirmed: `PC`
     landed past both the target function and its caller in one probe),
     and `wait`'s timeout is real *wall-clock* time, not simulated time,
     so it's jittery run to run. `stepi` is deterministic. (The one
     exception: if the thing under test genuinely depends on real
     peripheral timing advancing, like a Timer overflow actually firing,
     you need `run` + `wait` for that part, `stepi` does not reliably
     advance peripheral timing either. Use `stepi` to get to a stable
     point after initialization, `run`+`wait` only for the
     timing-dependent part, and treat register reads taken right after
     a `wait` as a snapshot that could be a few instructions early or
     late.)
   - `print <REGISTER>` (or `x /1xbr <addr>` for a raw byte) for every
     SFR the peripheral touched.
   - Compare against the hand-computed expected values from step 3's
     test header, byte for byte. Not "looks plausible", the literal
     expected value.
   - **For `MODE=gpio` families, when the peripheral has real hardware
     timing (a timer, a continuously-running conversion), don't rely
     solely on the main loop's bounded iteration count to reach
     `epic_harness_report()`.** Confirmed on PIC16F193X's Timer2/4/6:
     with three timers firing ISRs continuously, the MPLAB SIM's
     interrupt-servicing overhead starved the bounded main loop badly
     enough that it never reached the report call inside the `mdb`
     wait window, even though every ISR was firing correctly and the
     peripheral logic was right. Two independent fixes made this
     reliable: (1) an *early exit* from the bounded loop the moment the
     pass condition is already true (mirrors `pic18fxx5x-hal/tests/
     example_timer2.c`'s `if (overflows >= EXPECTED_OVERFLOWS) break;`
     idiom), and (2) having the slowest-firing instance's own ISR drive
     the RA0 marker directly the instant its condition is met, instead
     of depending on the main loop to notice and report. Both paths set
     RA0=1 on PASS; whichever gets there first wins. Do this for any
     `MODE=gpio` example whose pass condition depends on more than one
     concurrently-running interrupt source, not just when a bug is
     observed, it's cheaper to build it in than to debug a flaky gate
     later.
7. **If any value is wrong**, don't assume "not enough steps yet" and
   just increase the count. First check a **known-good control
   register**, something written via a plain compile-time-constant SFR
   access elsewhere in the same run. If the control register is correct
   and the one under test isn't, that's a real bug, not a timing issue.
   This is exactly the trick that distinguished a genuine PIC18 bug from
   a false alarm this session (`SSPCON`, unbanked, direct-address, read
   correctly; `SSPADD`, same file, runtime-addressed, read `0`).
8. **A register you wrote a known value to can legitimately read back
   different bits than you wrote**, and that is not automatically a
   codegen bug. Confirmed repeatedly across PIC16F193X's peripherals:
   `BAUDCON`'s `RCIDL` (read-only, hardware sets it whenever the
   receiver is idle, so a driver that writes `0x00` reads back `0x40`),
   both comparators' `CxOUT` (read-only live comparator output, bit 6),
   `FVRCON`'s `FVRRDY` (read-only, hardware sets it once the reference
   has stabilized), and CPS's `CPSOUT` (read-only raw oscillator
   output). Before treating a mismatch as a bug, check the datasheet's
   register table for which bits in that specific register are
   read-only status/flag bits versus writable control bits, and mask
   the read-only bits out of the comparison (or assert against the
   POR-then-hardware-set value, not the value you wrote). Only escalate
   to "real bug" once the mismatch remains after masking every
   documented read-only bit.
9. **Treat these patterns as high-risk, always verify them explicitly**,
   because every real bug found in this codebase so far had one of
   these shapes:
   - **Any SFR address that is a variable, struct field, or function
     parameter at the point of access**, not a literal `PIC_REG_*`
     token. On PIC16, a value read/written while a bank switch
     (`pic_select_bank`) is in effect can get misdirected to the wrong
     memory location. On PIC18, XC8 has compiled this exact shape to
     the program-memory table read/write mechanism (`TBLPTR`/`TABLAT`/
     `tblrd`/`tblwt`) instead of a data-memory access, silently writing
     nowhere. Both are real, both were found by this exact gate, not by
     inspection. If you must dispatch on a runtime value (which
     peripheral instance, which IRQ source), branch *before* touching
     any SFR so each branch's own register access uses a literal
     constant token (see `pic18_irq.c`'s `switch`-per-`case` shape, or
     `pic18fxx5x_ccp.c`'s `CCP_WRITE_*`/`READ*` macros, for the proven
     pattern).
   - **Any read-modify-write**, even with a compile-time-constant
     address: confirm the byte that comes back after the write actually
     reflects the intended change, not the pre-write value or something
     else entirely.
   - **Any computed value that depends on device clock speed or timing**
     (baud-rate divisors, prescaler/postscaler search loops): sanity
     check the computed value is actually in-range for the register
     width, don't just trust the formula ran without error. (PIC18's
     sim-target harness had exactly this bug: correct formula, wrong
     divisor choice for its own `FOSC_HZ`, silently overflowed `SPBRG`'s
     8 bits and returned an error sentinel that got truncated instead
     of surfaced.)
10. Only once host **and** real-`mdb` verification both pass does the
    peripheral count as done. Move to the next one.

## §5. Path B: new family from scratch

Bigger version of the same discipline, in dependency order (each step
assumes the previous ones are done and verified, not just written):

1. **New sibling tree** (`<partno>-hal/`), skeleton copied from
   whichever existing family is architecturally *closest* by addressing
   model and interrupt architecture (not by pin count or peripheral
   list, per `docs/multi-family-plan.md`'s own note, still correct).
   Expect to still write real driver code even when copying a skeleton.
2. **Platform header first** (`include/target/`, `include/host/`), then
   get the most minimal possible real-target build (a GPIO toggle, no
   peripherals, no interrupts) actually running under real `mdb` before
   writing a single peripheral driver. This validates the addressing
   model assumptions from §1 while the blast radius of being wrong is
   still one file, not ten.
3. **SFR map**, datasheet-cited, matching the existing families'
   `<family>fxx_sfr.h` convention.
4. **`epic-common`'s four-function harness target implementation** for
   the new family (mirrors `epic_harness_target.c`).
5. **IRQ backend next, before any peripheral that needs it**, and give
   it its own dedicated `mdb` smoke test (enable one timer interrupt,
   confirm it actually fires and the flag/enable bits read back
   correctly) *before* building peripherals on top of it. This is the
   single highest-risk component: both real bugs found in this
   session's two families were in or adjacent to interrupt/bank-select
   plumbing (PIC16's `pic_select_bank`-adjacent corruption, PIC18's
   `pic18_irq.c` table-driven dispatch), and every peripheral driver
   depends on `EPIC_IRQ_*` working correctly. Finding this bug on day
   one of a new family, isolated, is much cheaper than finding it after
   ten peripherals are built on top of a broken IRQ core.
6. **Peripherals one at a time**, each through §4's verification gate in
   full, no batching multiple unverified peripherals together. Two
   sub-patterns recurred often enough across PIC16F193X's 13
   peripherals to call out explicitly:
   - **Multiple identical-shape instances of one peripheral (Timer2/
     Timer4/Timer6; CCP1 through CCP5) get one driver with an instance
     selector, not N copy-pasted drivers.** Every register access
     still branches on the instance *before* touching any SFR, so each
     branch's own access stays a literal `PIC_REG_*` token (§4's
     high-risk pattern about runtime SFR addresses applies here just
     as much as to a single-instance driver; branching per-instance
     doesn't get a pass on that rule). When one instance's register
     shape genuinely differs (PIC16F193X's CCP4/CCP5 are plain CCP,
     no PWM/AS/PSTR registers at all, unlike CCP1-3's Enhanced CCP
     shape), confirm the absence directly against the DFP header
     (grep for the register name, confirm it doesn't exist) rather
     than assuming symmetry with the other instances; don't let a
     driver reference a macro for a register a smaller instance
     doesn't have.
   - **For a peripheral with a large, non-obvious register-to-function
     mapping (a segment LCD driver's segment-to-data-register layout,
     a crossbar/mux table), derive the mapping from the DFP header's
     own bitfield macros, not from hand-reading the datasheet's prose
     or a guessed formula.** Confirmed on PIC16F193X's LCD driver: the
     DFP header carries one `_LCDDATAn_SEGxCOMy_POSN` macro per actual
     segment/common/register/bit combination (hundreds of them for a
     full segment count), which is the vendor's own machine-readable
     statement of the mapping, strictly more reliable than re-deriving
     it by hand from a datasheet table. Grep the DFP header for the
     full family of macros, confirm the pattern is linear/regular
     across a sample large enough to be confident (not just the first
     one or two entries), then encode the derived formula in the
     driver with a comment citing where it came from, not the formula
     alone.
7. **`<family>-hal/MANUAL.md`** as you go, matching
   `pic16f87xa-hal/MANUAL.md`/`pic18fxx5x-hal/MANUAL.md`'s shape:
   datasheet-cited peripheral/register reference, pointing to
   `epic-common/MANUAL.md` for every family-agnostic convention instead
   of re-explaining it (see `docs/hal-manual-plan.md` for why that split
   exists and how it was carved out).
8. **Any compiler/codegen quirks discovered along the way** recorded as
   a live-gotcha section in `<family>-hal/README.md`, in the format
   established in `pic16f87xa-hal/README.md` and
   `pic18fxx5x-hal/README.md` (a standalone `docs/ARCHITECTURE.md` is
   only kept while it holds live conventions, as `pic16f193x-hal` does
   for its AGENTS.md-cited BSR findings): cross-check any claimed
   compiler "bug" against the actual XC8 User's Guide (extract it from
   the toolchain image, don't assume from general PIC knowledge) before
   writing it down as one, and cite the section. An earlier pass of this
   repo's own documentation called several things "genuine XC8 bugs"
   from empirical probing alone without doing that check, and had to be
   corrected; don't repeat it.
9. **CI wiring, done at foundation time, not deferred to "later":**
   `scripts/ci-discover-xc8-matrix.py`'s `FAMILIES` dict and its
   `if "<family>" in d` detection branch both need the new family the
   moment its first `mcu/<family>-mplabx/Makefile` exists and builds,
   even before every peripheral lands. Confirmed the hard way on
   PIC16F193X: the family's real-target build had been passing locally
   for a long time before anyone noticed the discovery script itself
   still only recognized the original two families, and `xc8-build.yml`
   was failing outright with `unrecognized family for
   pic16f193x-hal/mcu/pic16f193x-mplabx` (a hard `sys.exit`, not a
   silently-skipped family) the entire time. A local `make xc8-build
   MODULE=<new>-hal MCU=<mcu>` passing is not the same signal as the CI
   matrix actually including the family; run `python3
   scripts/ci-discover-xc8-matrix.py` directly and grep its JSON output
   for the new family's name as its own explicit checklist item, don't
   infer it from local builds working. `xc8-build.yml`'s matrix then
   picks the family up automatically via that discovery script (never
   hand-list a family or MCU inside the workflow file itself);
   `sim-tests.yml` gets the new family's pilot module once its
   sim-target harness exists (that one's matrix is hand-listed by
   design, see its own header comment, so it needs an explicit edit,
   unlike `xc8-build.yml`).
10. **Litmus test**: point an existing family-agnostic consumer (a
    `epic-*` module, the task manager, whatever exists by then) at the
    new family and confirm zero changes are needed to the consumer
    itself. If something in `epic-common/` needed to change to fit the
    new family, that's a signal the shared contract was accidentally
    family-specific somewhere, fix that contract, not a sign the whole
    approach is wrong (`docs/multi-family-plan.md`'s own framing, still
    correct).

## §6. Documentation and sign-off checklist

Before calling a device/family addition done, confirm all of these
exist and are accurate, not just present:

- [ ] Every peripheral passed §4's full gate (host + real `mdb`), not
      just "looks right" or "builds".
- [ ] `<family>-hal/MANUAL.md` covers every peripheral touched,
      datasheet-cited.
- [ ] Any genuinely surprising codegen finding is recorded in
      `<family>-hal/README.md` (or a kept `docs/ARCHITECTURE.md`, per
      the docs-lifecycle rules) with manual citations, not bare
      assertions.
- [ ] `scripts/ci-discover-xc8-matrix.py` reflects the new MCU(s)/family,
      verified by actually running `python3
      scripts/ci-discover-xc8-matrix.py` and finding the new family in
      its JSON output, not by "the Makefile exists so it must be
      picked up" (confirmed false on PIC16F193X: the Makefile existed
      and built clean for a long time before anyone noticed the
      discovery script itself had never been told about the family,
      and `xc8-build.yml` was failing outright the whole time). Any
      `KNOWN_BROKEN` entries are documented in
      `docs/mplabx-link-gaps-plan.md` with a real root cause, not just
      silently excluded.
- [ ] Full regression run: every module, every MCU variant in the
      affected family (or families, if a shared `epic-common` change
      was needed), host and real-target, immediately before the final
      commit, not from memory of an earlier run.
- [ ] User has signed off on the final state before anything gets
      pushed.

---

## Appendix: known-risky patterns, cross-referenced

A running list, update it when a new one is found. Each entry: what it
looks like, which family/families it's confirmed on, and where the full
account lives.

| Pattern | Confirmed on | Full account |
|---|---|---|
| SFR access while a `pic_select_bank`-style bank switch is in effect, via a plain C local/parameter | PIC16 (classic mid-range) | `pic16f87xa-hal/README.md` (XC8 codegen gotchas) |
| SFR address that is a runtime variable/struct-field/parameter at the point of access (not a literal token) | PIC18 | `pic18fxx5x-hal/README.md` (XC8 codegen gotchas) |
| Baud-rate/timing divisor math that can silently overflow the target register width | PIC18 (found in `pic18_harness_sim_target.c`) | `pic18fxx5x-hal/README.md` (XC8 codegen gotchas) |
| Missing `HARNESS=sim` → watchdog-off Makefile override, WDT resets a bounded diagnostic build mid-run | PIC16, PIC18 | `docs/ci-plan.md` Phase 4 |
| Dangling pointer: a HAL `_Init` stores the caller's pointer instead of copying the handle, and the caller's storage is a non-`static` local | PIC16 (fixed); PIC18's own driver already copies the handle, not affected | `docs/ci-plan.md` Phase 4 |
| Read-only status/flag bits (RCIDL, CxOUT, FVRRDY, CPSOUT, ...) reading back set even though the driver never wrote them, mistaken for a write not landing | PIC16F193X (Enhanced Mid-range) | `pic16f193x-hal/docs/ARCHITECTURE.md`; §4 step 8 above |
| `MODE=gpio` bounded-loop example starved by continuously-firing ISRs on MPLAB SIM, never reaching `epic_harness_report()` inside the `mdb` wait window, despite every ISR and the peripheral logic being correct | PIC16F193X (Timer2/4/6, 3 concurrent timer ISRs) | §4 step 6's sub-bullet above; the fix (early-exit + ISR-driven marker) is in `pic16f193x-hal/tests/example_timer246.c` |
| CI matrix discovery script (`scripts/ci-discover-xc8-matrix.py`) not updated when a new family's first `mcu/*-mplabx/Makefile` lands, so local real-target builds pass indefinitely while `xc8-build.yml` fails outright on every push | PIC16F193X | §5 step 9 above; §6's CI-wiring checklist item |

This table is deliberately family-specific in its "confirmed on" column
and deliberately generic in its "pattern" column: a new family should
assume none of these are ruled out until actually verified, per §4.
