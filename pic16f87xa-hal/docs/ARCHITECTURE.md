# `pic16f87xa-hal` architecture: XC8 v4.00 codegen notes

> Status: **root cause of the Phase 4 sim-target hang found and fixed
> (Finding 9)**, after an extensive detour (Findings 4-8) chasing two
> well-motivated but ultimately wrong theories. Written up after
> `docs/ci-plan.md` Phase 4's sim-target pilot debugging surfaced several
> real, surprising interactions between this HAL's PIC16 interrupt/bank
> code and XC8 v4.00's code generator. Earlier drafts of this account (in
> commit messages and `docs/ci-plan.md`) called several of these findings
> "genuine XC8 bugs" without checking that against the compiler's own
> documentation first; this document corrects that. Citations below are
> to `MPLAB XC8 C Compiler User's Guide for PIC MCU`, DS-50002737L,
> shipped at `docs/MPLAB_XC8_C_Compiler_User_Guide_for_PIC.pdf` inside a
> real XC8 v4.00 install (`docker/ci-toolchain/`'s image), not assumed or
> recalled from general PIC knowledge.

## Why this exists

`epic-common/MANUAL.md` documents this repo's *portable* interrupt
contract (the four-function harness, `EPIC_IRQ_*`, the dispatch pattern).
This document is narrower and lower-level: specific things XC8 v4.00
does with *this family's* generated code around banking, interrupts, and
the hardware call stack, discovered because Phase 3/4 of `docs/ci-plan.md`
was the first time this repo's PIC16 firmware actually *ran* (under MPLAB
SIM) instead of only being linked. None of this is PIC16F87XA-datasheet
material; it belongs here rather than in `MANUAL.md`, which is
datasheet-cited peripheral reference.

## Finding 1: inline `asm()` resets the compiler's bank tracking

**Confirmed, documented.** §5.12.2, *In-line Assembly*:

> "...the compiler will reset all bank tracking once it encounters
> in-line assembly, so any Special Function Registers (SFRs) or bits
> within SFRs that specify the current bank do not need to be preserved
> by in-line assembly."

The compiler tracks, as a compile-time optimization, which bank it
*believes* is currently selected, so it can omit a redundant bank-select
instruction before a direct-addressed access if it already thinks the
right bank is active. Encountering an `asm()` statement invalidates that
belief entirely (the compiler cannot scan hand-written asm for register
effects, §5.9.4.1), so the *next* SFR access after an asm block gets a
fresh, correct bank-select regardless of what the asm actually did to
`STATUS`.

This directly validates `pic16f87xa_platform.h`'s `EPIC_PIE_ENABLE_BIT`/
`EPIC_PIE_DISABLE_BIT` macros (the fix for `EPIC_IRQ_Enable`/
`EPIC_IRQ_DisableSrc`'s PIE1/PIE2 read-modify-write): loading the operand
into W *before* the bank switch, doing the whole read-modify-write as a
single `iorwf`/`andwf <SFR>,f` inside one `asm()` block, is not a
workaround for a bug, it is the documented, intended way to hand-roll a
banked SFR access from C.

## Finding 2: a plain C write to `STATUS` is likely not a recognized bank-change idiom

**Plausible, not confirmed.** This is the current best explanation for
why the *original* `pic_select_bank` (a `static inline` function doing
`status = EPIC_REG8(PIC_REG_STATUS); status &= ~(RP0|RP1); status |=
(bank&3)<<5; EPIC_REG8(PIC_REG_STATUS) = status;`, no `asm()` involved at
all) reliably corrupted a caller's own live local value when called as a
real out-of-line function (confirmed via a dedicated probe:
`EPIC_TIMER2_WritePeriod(200)` landed as `PR2=0` every time).

The compiler's bank-tracking optimizer (Finding 1) presumably recognizes
specific, compiler-generated bank-select sequences (or `BANKSEL`-style
assembler idioms, see §5.12.3.1's worked example, which uses
`BANKSEL (PORTB)` explicitly rather than a raw SFR write) as the trigger
to update its internal "current bank" belief. A **plain C assignment
through a generic pointer-dereference macro** (`EPIC_REG8(PIC_REG_STATUS)
= status;`, indistinguishable at the IR level from writing any other
byte-sized object) may not be recognized as such a trigger at all. If so,
the compiler could still believe it is in whatever bank it was in before
the call, and legitimately omit a bank-select it should have re-emitted
for the caller's own subsequent direct-addressed local-variable access,
exactly matching the observed corruption.

**Not yet verified**: this would require inspecting the actual generated
`.s` for the original (pre-fix) `pic_select_bank` and confirming the
compiler's own bank-tracking state annotations (if the list file exposes
them) around the corrupted access. Not done; the fix (converting
`pic_select_bank` to a macro, so it is textually inlined before the
compiler ever gets a chance to apply function-call codegen to it at all)
sidesteps the question rather than resolving it, and works regardless of
which explanation is correct.

## Finding 3: function duplication across call graphs is real, documented, and working correctly

**Confirmed, not a bug.** §5.9.7, *Function Duplication*:

> "MPLAB XC8 compiler employs a feature that duplicates the generated
> code associated with any function that uses a non-reentrant model and
> that is called from more than one call graph. There is one call graph
> associated with main-line code and one for each interrupt function...
> The compiler will duplicate the output for any non-reentrant function
> called from more than one call graph. This makes the function appear
> to be reentrant."

Duplicated symbols get an `i1_` prefix (`i2_` for a PIC18 high-priority
interrupt). This is directly visible in this HAL's own generated `.s`:
`EPIC_IRQ_ClearFlag` is called from both main-line code (via
`EPIC_TIMER2_Init`) and from every peripheral `_IRQHandler` in the
interrupt dispatch chain, and the call graph table shows exactly two
compiled copies, `_EPIC_IRQ_ClearFlag` (main-line) and
`i1_EPIC_IRQ_ClearFlag` (interrupt). This is the compiler correctly doing
its documented job, not a defect; seeing an `i1_`-prefixed symbol in a
`.s`/`.sym`/`.map` file is expected and fine, not a red flag by itself.

## Finding 4: the interrupt call graph's own estimated stack depth exceeds PIC16's 8-level hardware limit

**Confirmed via the compiler's own call-graph analysis, not yet
confirmed as the actual root cause of the remaining sim-target hang.**

`epic-tick`'s sim-target build (`epic-tick/mcu/pic16f87xa-tick-mplabx`,
`HARNESS=sim`) produces two separate call-graph tables in the generated
`.s` (`;!Call Graph Tables:`), one rooted at `_main` (estimated maximum
stack depth **5**, well within budget) and a separate one rooted at
`_PIC16_IRQ_Handler` (estimated maximum stack depth **10**), because an
interrupt's stack usage adds on top of whatever depth main-line code was
at when it fired, not a fresh count from zero. PIC16F87XA's hardware
call stack is fixed at 8 levels (§3.6.5: "An 8-bit PIC device has a
limited hardware stack... If the nesting of function calls and
interrupts is too deep, the stack will overflow (wraps around and
overwrites previous entries). Code will then fail at a later point").
The deepest interrupt-side path is
`_PIC16_IRQ_Handler -> _epic_dispatch_all_irqs -> *_IRQHandler ->
i1_EPIC_IRQ_ClearFlag`, i.e. the dispatch pattern's own design (every
peripheral `_IRQHandler` gets a strong reference and is unconditionally
called on any interrupt, `pic16_irq_dispatch.c`'s own documented
contract), not anything specific to `epic-tick` or this session's
changes.

The compiler is aware of this class of problem and has a documented
mitigation, the `-mstackcall` option (§4.6.1.25: "the compiler will
revert to using a look-up table method of calling functions once the
stack is full"), off by default. A quick trial adding it to
`epic-tick`'s PIC16 Makefile did **not** cleanly fix the sim-target
hang (the captured UART output went from "first delay completes, hangs
on the second" to "no output at all", i.e. some other change in
behavior, not obviously better); reverted rather than committed
half-verified. Whether that's `-mstackcall` needing to be paired with
something else, an interaction with the duplicated-function machinery of
Finding 3, or a red herring, is not yet determined.

**What this does and does not establish**: this is real, compiler-stated
evidence that a genuine risk condition exists (a documented feature that
exists specifically because unmitigated hardware-stack overflow "wraps
around and overwrites previous entries," which is exactly the kind of
failure that could explain `EPIC_IRQ_Disable` never reaching its matching
`EPIC_IRQ_Restore`, observed as `GIE` staying disabled indefinitely after
the first successful interrupt cycle). It does not, by itself, prove
that *this specific failure* is caused by *this specific* stack
condition. **Finding 5 below tested this directly and the result argues
against depth being the (sole) mechanism.**

## Finding 5: trimming the dispatcher's call depth does not fix the hang, and can break earlier instead

**A direct, reproducible experiment against Finding 4's hypothesis;
result contradicts a pure stack-depth explanation.** If the hang really
were "interrupt-path call depth 10 exceeds the 8-level hardware stack,"
shrinking `epic_dispatch_all_irqs` (`pic16_irq_dispatch.c`) down to only
the one handler `epic-tick`'s test actually needs (`TIMER2_IRQHandler`)
should reduce worst-case interrupt-side depth well under 8 and fix the
hang. Tested directly (throwaway, uncommitted edits to
`epic_dispatch_all_irqs`, rebuilt and run under real `mdb`/MPLAB SIM each
time, `run` + real-time `wait 10000` + `halt`, checking for the first
delay's `"tick: delay(10) -> %lu ms"` log line as the pass signal, since
that's what the known-good baseline reliably produces before it hangs):

| Dispatcher body | Result |
|---|---|
| All 13 handlers (baseline, `master`) | First delay's log line appears, hangs on the second (documented behavior) |
| `TIMER2_IRQHandler()` only | **No output at all**, worse than baseline |
| `TIMER2_IRQHandler(); CCP1_IRQHandler();` only | First delay's log line appears (matches baseline) |
| `TIMER0/TIMER1/TIMER2/CCP1_IRQHandler()` only | Matches baseline |
| `TIMER1_IRQHandler(); TIMER2_IRQHandler();` (no CCP1) | **No output at all** |
| `TIMER0_IRQHandler(); TIMER2_IRQHandler();` (no CCP1) | **No output at all** |

Binary-searching which handler's presence flips the result landed on:
**`TIMER2_IRQHandler` alone is broken; adding `CCP1_IRQHandler` back
(which *increases* call depth) fixes it.** This is the opposite of what
a pure call-depth theory predicts: adding depth should never fix an
overflow, only worsen it. None of these configurations eliminate the
hang either (nothing tested reached a full PASS), so this doesn't
directly locate a fix, but it strongly suggests the mechanism is not
simply "the interrupt path is too deep." A more consistent explanation:
XC8's non-reentrant storage-overlap assignment (which functions' locals
get to share RAM, decided from the *whole program's* call-graph shape,
not a per-function local decision) shifts when the interrupt call graph's
shape changes, and some configurations happen to produce a harmful
overlap between an unrelated pair of functions while others don't. This
would make the underlying issue closer to Finding 2 (storage overlap
across a boundary the compiler's analysis didn't fully account for) than
to Finding 4 (raw depth), with Finding 4's depth-10-vs-8 warning being a
real, correctly-reported risk that happens to coexist with this rather
than being demonstrated as the actual trigger.

**Not done**: actually locating the specific overlapping pair (would
need the linker `.map`'s PSECT placement, or `-Wa,-a` per-symbol storage
assignment, compared between a working and broken dispatcher
configuration) to confirm this overlap theory concretely rather than
inferring it from the bisection pattern above.

## Finding 6: the XC8 v4.00 release notes' own indirect-call/`-mstackcall` known issue, tested and ruled out

**A second official, well-motivated lead, also directly tested and
falsified.** The XC8 v4.00 release notes (`docs/Readme_XC8_for_PIC.htm`
inside the toolchain image, §6 Known Issues) list `XC8E-11, "Stack
overflow"`: "When the managed stack is used (the `stackcall`
suboption)... if these functions are indirect function calls (made via a
pointer) the compiler will actually encode them using a regular call
instruction and when these calls return, the stack will overflow. The
managed stack works as expected for all direct function calls... but not
for indirect calls that exceed the stack depth." This matches Finding
4's `-mstackcall` trial producing *worse* behavior instead of better,
and this HAL has exactly one indirect (function-pointer) call sitting in
the interrupt path: `USART_TX_IRQHandler` calling `g_usart->
TxCpltCallback()` (`pic16f87xa_usart.c:150`), reachable because
`pic16_harness_sim_target.c` registers a no-op callback purely to work
around a separate, real bug (`EPIC_USART_Init` only sets `TXEN` when a
non-null `TxCpltCallback` is supplied, `pic16f87xa_usart.c:61`).

Tested directly (throwaway, uncommitted): made `TXEN` unconditional in
`EPIC_USART_Init` and left `TxCpltCallback` `NULL` in the harness, so the
indirect call is entirely absent from the compiled interrupt path for
this binary. Rebuilt, ran under real `mdb`, checked both the UART output
and register state directly (`run` + `wait 30000` + `halt` +
`print INTCON`/`PIR1`/`PIE1`): **no change**. Still hangs after the
first delay, `INTCON=64` (`PEIE=1, GIE=0`), the identical symptom as
baseline. Reverted (`git checkout --`) once confirmed.

This rules out the indirect-call/`-mstackcall` gap as the (sole) cause
too, despite being a real, officially-documented, and specifically
matching-shaped issue. Combined with Finding 5, both of the two most
plausible, best-evidenced theories (call depth, indirect calls escaping
`-mstackcall`) have now been tested and falsified individually. The
overlap-sensitivity observation from Finding 5's bisection still stands
(it's an experimental result, not a theory it was testing), so it
remains the best lead, just without yet knowing *which* two things
overlap.

## Finding 7: pinning the storage that goes stale just moves the corruption, doesn't remove it

**Whack-a-mole, not a fix; strengthens Finding 5 rather than resolving
the bug.** Tried the cheapest targeted mitigation available: make
`EPIC_IRQ_Disable`'s internal locals (`s`, `prev`) and `epic_tick_get`'s
locals (`prev`, `t`, the ones live across the disable/restore window)
`static`, so they get a permanently dedicated address instead of
participating in XC8's non-reentrant storage-overlap pool at all
(throwaway, uncommitted, `pic16_irq.c` and `epic_tick.c`). Rebuilt, ran
under real `mdb`.

Result: the *original* symptom is gone. `INTCON` reads `192`
(`GIE=1, PEIE=1`) after the hang, not the usual `64`. But the test still
produces no UART output at all, and register inspection shows `PR2`
(Timer2's period register) reading `0`, which should hold a
compiler-computed nonzero value, exactly the same corruption signature
this session already fixed once before for a different call
(`EPIC_TIMER2_WritePeriod` landing `PR2=0` due to the original
`pic_select_bank`-as-function bug, unrelated to this specific edit).
Reverted (`git checkout --`).

Read the manual's own claim on this (§5.7.2.1, Compiled Stack
Operation): "The compiler takes into account that interrupt functions,
and functions they call, need their own dedicated memory." That's the
documented guarantee; this session's whole investigation is evidence
that guarantee doesn't reliably hold for this program. There is no
compiler option to disable this overlap analysis outright, and the
alternative that would sidestep it entirely (the software/reentrant
stack, §5.7.2.2) is explicitly **not available for classic mid-range
PIC16** ("available only for Enhanced Mid-range and PIC18 devices"), so
there's no blunt escape hatch for this device family, only the compiled
stack's overlap-sharing model, bugs and all.

**Conclusion**: pinning individual variables one at a time is not a
convergent strategy, it just relocates which live-across-an-interrupt
value gets corrupted. A real fix needs either (a) the `.map`/`-Wa,-a`
forensic pass to find and pin *every* overlapping pair at once, with no
guarantee that's a finite or small set, or (b) a structural change that
reduces how much state and how many functions the interrupt path and
main-line path actually share, rather than fighting the allocator
variable by variable.

## Finding 8: the flagged `.sym` storage comparison, done; found two candidates, neither was it

**The mechanical next step from Finding 5/7 was actually carried out.**
Built the `TIMER2_IRQHandler`-only (broken) and `TIMER2_IRQHandler` +
`CCP1_IRQHandler` (matches baseline) dispatcher configurations, pulled
both `.sym` files, and diffed every auto/param local's `(bank, address)`
assignment between them (a Python pass grouping symbols by storage slot,
scratchpad-only, not committed). Confirmed the broken config independently
reproduces the exact `PR2=0` signature from Finding 7 (`INTCON=192`,
`GIE=1`, but `PR2=0`), so this pair is a legitimate, minimal repro of
that same corruption class, not a different bug.

The diff found `compute_period`'s storage shifted by 4 bytes between
configs (an incidental consequence of `CCP1_IRQHandler` changing the
interrupt call graph's own footprint), and in the *broken* config only,
`compute_period`'s `best_pr2` lands on the exact same `(BANK1, 176)`
address as `epic_harness_init`'s `cycles` parameter. This looked like
the smoking gun (`best_pr2` is the value that ultimately becomes `PR2`),
so it was tested directly: made `compute_period`'s locals `static`
(throwaway). **`PR2` was still `0`.** Checked the next candidate from
the same data, `epic_tick_init`'s own locals (the caller that actually
holds the value between `compute_period` returning and
`EPIC_TIMER2_Init` consuming it): no colliding function was found at any
of its addresses at all, ruling that candidate out without even needing
to test it. Both throwaway edits reverted.

**Where this leaves the forensic approach**: it worked exactly as
designed (found a real, concrete storage collision, distinguishable from
noise), but the first candidate it surfaced wasn't the actual cause, and
the second had no collision at all. Whatever is corrupting `PR2` in this
specific repro is not explained by a simple pairwise `(bank, address)`
overlap between two named C functions' compiled-stack frames, at least
not among the ones directly touching `PR2`'s value on its way to the
register. Two real possibilities: (a) there's a *different* colliding
pair not yet checked (the `.sym` diff has ~15 multi-owner slots total,
only 2 were tested), or (b) the mechanism isn't storage overlap at the
C-variable level at all, and needs actual instruction-level tracing of
the `PR2` write itself (single-stepping through
`EPIC_TIMER2_Init`/`EPIC_TIMER2_WritePeriod`'s generated assembly from
reset, watching `PR2`'s value change) rather than more `.sym` inference.
Neither was pursued further in this session.

## Finding 9: root cause found, unrelated to Findings 4-8's storage-overlap detour

**The actual bug, localized and fixed.** Findings 4 through 8 spent
considerable effort chasing two theories, interrupt call-graph stack
depth and a systemic non-reentrant storage-overlap collision, based on
symptoms (`GIE` stuck disabled, `PR2` reading `0`) observed downstream
of where the real corruption happens. Both theories turned out to be
wrong, or at best measuring a secondary effect. The deciding piece of
evidence: `compute_period()` and `EPIC_TIMER2_WritePeriod()` both run
during `epic_tick_init`, entirely *before* `EPIC_IRQ_Restore(1)` ever
enables `GIE`. No interrupt can be involved in corrupting `PR2` at that
point, full stop, regardless of call-graph shape or storage overlap.
That single fact ruled out every interrupt-timing-dependent theory this
document had been pursuing.

Once reframed as "this must be deterministic, not a race," the fix was
mechanical: `mdb` instruction-stepping (`stepi`, not `run`+`wait`, see
the methodology note below) through the exact sequence with a fixed
step budget, reading each candidate variable's memory address directly
(`x /1xbr <addr>`, addresses from a fresh `.sym`), confirmed:
- `compute_period@best_pr2` (the search loop's own result): correct
  (`249`, matching the datasheet-formula hand calculation for 20 MHz),
  stays correct long after the function returns.
- `epic_tick_init@pr2` (the caller's copy, received via the `uint8_t
  *pr2` output parameter): also correct (`249`).
- The actual `PR2` hardware register: `0` at the same point in time.

The only thing left between "correct value sitting in a C variable" and
"wrong value in the register" is `EPIC_TIMER2_WritePeriod` itself:

```c
void EPIC_TIMER2_WritePeriod(uint8_t period)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    EPIC_REG8(PIC_REG_PR2) = period;   /* <-- period misdirected here */
    pic_select_bank(prev);
}
```

This is the *exact* failure shape Finding 1 already proved and fixed
for `EPIC_PIE_ENABLE_BIT`/`EPIC_PIE_DISABLE_BIT`: a plain C-level access
to something the compiler assumes is Bank-0-resident (here, the
`period` parameter), performed after `pic_select_bank(1)`'s bank switch,
gets misdirected. The only difference is *which* function it hit.
`EPIC_USART_Init`'s SPBRG write has the identical shape
(`EPIC_REG8(PIC_REG_SPBRG) = h->SPBRG;` after its own
`pic_select_bank(1)`) and was confirmed corrupted too (`SPBRG` read `4`,
should be `129` for 9600 baud at 20 MHz), just masked until now because
MPLAB SIM's `uart1io` capture isn't baud-timing-sensitive, so a wrong
baud rate doesn't visibly break the captured byte stream in simulation,
only on real hardware talking to a real receiver.

**Fix**, mirroring Finding 1's already-proven pattern exactly: load the
value into W through a bank-independent common-RAM scratch byte
*before* switching banks, then a single `movwf <SFR>` while banked
touches nothing else. New `EPIC_BANK1_WRITE8(sfr, value)` macro
(`target/pic16f87xa_platform.h`) with its own scratch byte
(`epic_bank1_scratch` at `0x71`, deliberately separate from
`epic_irq_pie_scratch` at `0x70`, unrelated subsystems). Applied to both
`EPIC_TIMER2_WritePeriod` and `EPIC_USART_Init`. Verified: `PR2` reads
`249`, `SPBRG` reads `129`, and `epic-tick`'s PIC16 sim-target test
reaches `EPIC_HARNESS_RESULT: PASS` reliably (5/5 runs). Full host
suite and all 38 previously-passing PIC16 `(module, MCU)` real-target
builds re-verified clean, no regressions from the new scratch byte
(the same class of regression Root cause 3 in
`docs/mplabx-link-gaps-plan.md` hit last time one was added).

**A methodology correction, worth keeping**: `run` + `wait N` + `halt`
does **not** reliably stop at a `break`-set breakpoint in `mdb`'s
scripted/headless mode; in one probe during this investigation, `wait`
returned and `halt` reported a `PC` address past *both* the target
function and its caller, well beyond where the breakpoint should have
stopped execution. Whether `run`/`wait` simply ignore breakpoints
entirely in this mode, or something else is going on, wasn't fully
determined, since `stepi` (deterministic instruction count, no reliance
on breakpoint-triggered halting, already established as the more
trustworthy tool for register-level work earlier in Phase 4) sidestepped
the question and got useful data directly. Treat `break`+`run`+`wait`
combinations in this toolchain's `mdb` as unverified until proven
otherwise; `stepi` plus direct memory reads (`x /1xbr <addr>`, addresses
from a matching `.sym`) is the reliable path for anything that doesn't
depend on real peripheral timing.

**Why Findings 4-8 weren't wasted**: Finding 5's dispatcher bisection
result (adding `CCP1_IRQHandler` back "fixing" a broken config) is now
explained too, retroactively: different dispatcher compositions shift
the compiled-stack storage layout enough to change other, *unrelated*
things (which functions' non-reentrant storage happens to overlap with
what), which can coincidentally paper over or unmask *this* bug's
visible symptoms without touching its actual cause. That's a real,
confirmed compiler behavior (Finding 5's data), just not this bug's
mechanism. Finding 6 (ruling out the indirect-call/`-mstackcall` gap)
and Finding 1 (the bank-tracking-reset mechanism this fix directly
relies on) both hold up entirely unchanged.

## Finding 9 follow-up: the other six call sites, audited and fixed

**Done, not just flagged.** The follow-up item this document originally
left open (the same `pic_select_bank(N)` shape appearing in
`pic16f87xa_adc.c`, `_eeprom.c`, `_ssp.c`, `_vref.c`, `_comp.c`, and
`_psp.c`, unaudited) was carried out. Each site was checked empirically
under real-target `mdb`, not assumed safe or unsafe from reading the
source: a throwaway probe (`EPIC_ADC_Init`/`EPIC_VREF_Init`/
`EPIC_COMP_Init`/`EPIC_PSP_Enable` with known values, and separately
`EPIC_SSP_Init`/`EPIC_EEPROM_WriteByte`) confirmed every one of them was
actually corrupted, landing `0` (or another wrong value) in `ADCON1`,
`CVRCON`, `CMCON`, `TRISE`, `SSPADD`, `SSPSTAT`, `SSPCON2`, `EEDATA`,
`EEADR`, `EECON1`, and `EECON2`, the identical mechanism as `PR2`/
`SPBRG`. One genuinely new data point: `EPIC_SSP_Init`/`EPIC_EEPROM_*`'s
internal bank-switch helpers (`ssp_b1_write`, `b2_write`/`b3_write`)
are tiny `static` functions that XC8 fully inlines (confirmed: zero
trace of their names survives in the generated `.s`), and the
corruption still happened anyway, comparing a same-Bank-0 register
(`SSPCON`, always correct) against the Bank-1 ones (`SSPADD`, wrong) in
the same probe run. Inlining does not sidestep this bug; the
misdirection happens in the flattened code too.

Fixed with the same pattern, extended to cover Banks 2 and 3
(`EPIC_BANK2_WRITE8`/`READ8`, `EPIC_BANK3_WRITE8`/`READ8`, needed for
EEPROM specifically since its own call sites interleave both banks
back to back, so neither macro can assume the incoming `RP1:RP0` state
the way the Bank-1-only macros safely do). SSP and EEPROM's helpers
take a runtime address parameter but the inline-asm macros need a
literal SFR name at compile time; fixed by dispatching on the address
*before* any bank switch begins (a plain comparison in ordinary Bank 0
context, nothing at risk there) and only then invoking the named macro,
since every real call site passes a compile-time-constant address
anyway.

Re-verified via the same probes with the fix applied: `ADCON1=0x80`,
`CVRCON=0x8A`, `CMCON=0xC5`, `TRISE=0x17`, `SSPADD=42`, `EEADR=66`,
`EECON1=6`, all matching hand-computed expected values. Full host suite
and all 38 previously-passing PIC16 `(module, MCU)` real-target builds
re-verified clean.

**Noticed but not fixed, unrelated bug**: `EPIC_SSP_ReadByte`
(`pic16f87xa_ssp.c`) writes `SSPSTAT` (Bank 1, `0x94`) with no
`pic_select_bank(1)` at all, relying on whatever bank happens to already
be selected. Not the corruption class this document is about (nothing
to do with a bank-switch-and-restore sequence at all, since there isn't
one here); flagged for whoever next touches SSP.

## Open, for whoever picks this back up

- Finding 2 (the *other* `pic_select_bank`-related bug, in the
  now-macro'd bank-select helper itself) remains a
  plausible-but-unconfirmed explanation for its own, separate symptom;
  not resolved by Finding 9's fix, which addresses a different call site
  entirely.
- `EPIC_SSP_ReadByte`'s missing `pic_select_bank(1)` (noted above): a
  real, separate bug, not yet fixed.
- This document itself should be checked for staleness against whatever
  XC8 version `docker/ci-toolchain/Dockerfile` pins if that version is
  ever bumped; these findings are cited against v4.00 specifically.

## Finding 10: the remaining sim-target hang, root-caused and fixed

**Finding 9's fix (the `pic_select_bank` misdirection in
`EPIC_TIMER2_WritePeriod`/`EPIC_USART_Init`) was necessary but not
sufficient: the merged PR #6 state still hangs the `epic-tick` sim-target
gate** (verified 0/3 at the merge commit `87848c2`; the "5/5 PASS" in
Finding 9 was evidently measured on an unmerged branch state). The hang
survived Findings 4-9's theories because it has three independent
contributors, each fixed in this session:

1. **MPLAB SIM can vector inside a `GIE=0` critical section.** A request
   latched while `GIE` was set is delivered even after
   `EPIC_IRQ_Disable` clears it, so `epic_tick_get`'s disable-around-
   read neither prevents the interrupt (tearing the 4-byte counter read,
   observed as `E10=2` for a 10 ms delay) nor reliably survives it (the
   ISR's return can leave `GIE` cleared, stopping the tick dead). Fix:
   `epic_tick_get` reads the counter twice and retries on change (the
   HAL's own CCP-capture pattern), no `GIE` manipulation at all.
2. **TXIF is a read-only status bit that stays set whenever TXREG is
   empty, so the dispatcher's `if (pir1 & TXIF)` branch fired the
   USART TX handler (and its callback, through XC8's PC-relative
   function-pointer table `i1fptable`, whose targets must share the
   table's flash page) on *every* ISR.** Once the linker scattered the
   callback (`s_tx_cplt`) to a different page than the table, the ISR
   jumped into garbage and wedged interrupt delivery. Finding 6's
   falsification was incomplete: it removed the callback but kept the
   handler (and its `GetFlag`/`stringdir` call) running every ISR. Fix:
   gate the TX branch on `TXIE` (the same flag-gating shape as the TMR1
   branch), so a disabled source's always-set status bit never
   dispatches anything.
3. **The dispatcher's PCLATH-less handler calls are genuinely
   layout-sensitive** (Finding 5's overlap observation was the visible
   symptom of the page scatter): XC8 emits no PCLATH setup for the
   interrupt call-graph's calls, assuming the linker co-locates them,
   and best-fit scatters them across pages. Fix: pin
   `epic_dispatch_all_irqs` to `0x900` (`__at`), co-locating it with
   the handlers (the ones in the failing layout were all page 1; the
   pin makes the co-location deterministic regardless of the linker's
   mood).

**Verification (all in the exact CI Docker toolchain)**: `epic-tick`
16F877A sim gate 8/8, `epic-tick` 18F4550, `epic-swuart` 16F877A, and
`epic-pic16f193x-firmware` 16F1937 (gpio mode) all PASS; the full
real-target matrix 84/84; all affected modules' host ctests pass;
pre-commit checks clean. The stringdir PCLATH-clobber class (Finding
10.2's second half) remains a latent hazard in the un-refactored
handlers' `GetFlag`/`ClearFlag` table lookups, but is no longer
reachable from `epic-tick`'s ISR (the TX branch is gated and the TMR2
handler's table lookups co-locate in current layouts); converting the
remaining handlers to the CCP1/CCP2 direct-flag pattern is tracked as
follow-up hardening, not required for the gate.
