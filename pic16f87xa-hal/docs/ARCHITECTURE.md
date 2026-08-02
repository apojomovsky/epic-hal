# `pic16f87xa-hal` architecture: XC8 v4.00 codegen notes

> Status: **investigation in progress, not closed out.** Written up after
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

`pic8-common/MANUAL.md` documents this repo's *portable* interrupt
contract (the four-function harness, `HAL_IRQ_*`, the dispatch pattern).
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

This directly validates `pic16f87xa_platform.h`'s `PIC8_PIE_ENABLE_BIT`/
`PIC8_PIE_DISABLE_BIT` macros (the fix for `HAL_IRQ_Enable`/
`HAL_IRQ_DisableSrc`'s PIE1/PIE2 read-modify-write): loading the operand
into W *before* the bank switch, doing the whole read-modify-write as a
single `iorwf`/`andwf <SFR>,f` inside one `asm()` block, is not a
workaround for a bug, it is the documented, intended way to hand-roll a
banked SFR access from C.

## Finding 2: a plain C write to `STATUS` is likely not a recognized bank-change idiom

**Plausible, not confirmed.** This is the current best explanation for
why the *original* `pic_select_bank` (a `static inline` function doing
`status = PIC8_REG8(PIC_REG_STATUS); status &= ~(RP0|RP1); status |=
(bank&3)<<5; PIC8_REG8(PIC_REG_STATUS) = status;`, no `asm()` involved at
all) reliably corrupted a caller's own live local value when called as a
real out-of-line function (confirmed via a dedicated probe:
`HAL_TIMER2_WritePeriod(200)` landed as `PR2=0` every time).

The compiler's bank-tracking optimizer (Finding 1) presumably recognizes
specific, compiler-generated bank-select sequences (or `BANKSEL`-style
assembler idioms, see §5.12.3.1's worked example, which uses
`BANKSEL (PORTB)` explicitly rather than a raw SFR write) as the trigger
to update its internal "current bank" belief. A **plain C assignment
through a generic pointer-dereference macro** (`PIC8_REG8(PIC_REG_STATUS)
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
`HAL_IRQ_ClearFlag` is called from both main-line code (via
`HAL_TIMER2_Init`) and from every peripheral `_IRQHandler` in the
interrupt dispatch chain, and the call graph table shows exactly two
compiled copies, `_HAL_IRQ_ClearFlag` (main-line) and
`i1_HAL_IRQ_ClearFlag` (interrupt). This is the compiler correctly doing
its documented job, not a defect; seeing an `i1_`-prefixed symbol in a
`.s`/`.sym`/`.map` file is expected and fine, not a red flag by itself.

## Finding 4: the interrupt call graph's own estimated stack depth exceeds PIC16's 8-level hardware limit

**Confirmed via the compiler's own call-graph analysis, not yet
confirmed as the actual root cause of the remaining sim-target hang.**

`pic8-tick`'s sim-target build (`pic8-tick/mcu/pic16f87xa-tick-mplabx`,
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
`_PIC16_IRQ_Handler -> _pic8_dispatch_all_irqs -> *_IRQHandler ->
i1_HAL_IRQ_ClearFlag`, i.e. the dispatch pattern's own design (every
peripheral `_IRQHandler` gets a strong reference and is unconditionally
called on any interrupt, `pic16_irq_dispatch.c`'s own documented
contract), not anything specific to `pic8-tick` or this session's
changes.

The compiler is aware of this class of problem and has a documented
mitigation, the `-mstackcall` option (§4.6.1.25: "the compiler will
revert to using a look-up table method of calling functions once the
stack is full"), off by default. A quick trial adding it to
`pic8-tick`'s PIC16 Makefile did **not** cleanly fix the sim-target
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
failure that could explain `HAL_IRQ_Disable` never reaching its matching
`HAL_IRQ_Restore`, observed as `GIE` staying disabled indefinitely after
the first successful interrupt cycle). It does not, by itself, prove
that *this specific failure* is caused by *this specific* stack
condition. **Finding 5 below tested this directly and the result argues
against depth being the (sole) mechanism.**

## Finding 5: trimming the dispatcher's call depth does not fix the hang, and can break earlier instead

**A direct, reproducible experiment against Finding 4's hypothesis;
result contradicts a pure stack-depth explanation.** If the hang really
were "interrupt-path call depth 10 exceeds the 8-level hardware stack,"
shrinking `pic8_dispatch_all_irqs` (`pic16_irq_dispatch.c`) down to only
the one handler `pic8-tick`'s test actually needs (`TIMER2_IRQHandler`)
should reduce worst-case interrupt-side depth well under 8 and fix the
hang. Tested directly (throwaway, uncommitted edits to
`pic8_dispatch_all_irqs`, rebuilt and run under real `mdb`/MPLAB SIM each
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

## Open, for whoever picks this back up

- The GIE-stuck-disabled hang (`docs/ci-plan.md` Phase 4) is still
  unresolved. Finding 5 rules out "just shrink the dispatcher" as a
  quick fix and points toward a storage-overlap investigation instead:
  compare the `.map`/`-Wa,-a` storage assignment between a working
  dispatcher configuration (e.g. `TIMER2_IRQHandler` + `CCP1_IRQHandler`
  only) and a broken one (`TIMER2_IRQHandler` alone) to find what,
  concretely, moved.
- Finding 2 remains a plausible-but-unconfirmed explanation; if anyone
  needs to write more hand-rolled bank-switching C (not asm) in this HAL,
  treat plain SFR writes to `STATUS` as unreliable for bank purposes
  until this is actually confirmed one way or the other, and prefer the
  `asm()`-based pattern from Finding 1.
- This document itself should be checked for staleness against whatever
  XC8 version `docker/ci-toolchain/Dockerfile` pins if that version is
  ever bumped; these findings are cited against v4.00 specifically.
