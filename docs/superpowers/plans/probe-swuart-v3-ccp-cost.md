# Probe: does the real fixed CCP mechanism fit the 521-cycle budget?

**Verdict: YES for both events, measured cleanly.** CCP2 compare-event
service (Task 1's fixed `CCP2_IRQHandler` -> `EventCallback`) costs
**288 cycles** (55% of the 521-cycle budget, 233 cycles of margin).
CCP1 capture-event service (`CCP1_IRQHandler` -> `EventCallback`,
verified via real pin injection, not just estimated) costs **404
cycles** (78% of budget, 117 cycles of margin). Both fit comfortably.
Measured on 2026-08-07 with XC8 v3.10 (native, `-mcpu=16f877a -O2
-std=c99`, DFP 1.8.167) under MPLAB SIM (`mdb`), same
breakpoint-and-stopwatch technique `probe-swuart-v2-real-isr-cost.md`
used, against a real probe program (`/tmp/swuart-v3-probe/probe.c`,
outside the repo) linking the actual `pic16f87xa_gpio.c` /
`pic16f87xa_timer1.c` / `pic16f87xa_ccp.c` / `pic16_irq.c` /
`pic16_irq_dispatch.c` / `pic16_isr_vector.c` sources, not a mirror.

**A second, more important finding surfaced along the way, and it
confirms rather than contradicts a hazard the v3 plan already
anticipated on paper.** The verbatim probe code specified by this
task's brief (`CompareValue = 521u` for CCP2, armed against a Timer1
that starts at reset) hits a real schedule-miss: `EPIC_CCP_Init()`
calls `EPIC_IRQ_ClearFlag`/`EPIC_IRQ_Enable` (both table-driven, the
same expensive lookups `probe-swuart-v2-real-isr-cost.md` measured at
60-160 cycles each) *before* it writes `CCPRxH`/`CCPRxL`/`CCPxCON`.
By the time GIE is finally enabled, Timer1 has already reached 1296,
past the CompareValue=521 target the live hardware comparator needed
to catch. The compare never fires on its first pass; it only fires
after Timer1 wraps all the way around (65536 cycles) and reaches 521
again, by which point Timer1 has *also* genuinely overflowed
(`TMR1IF` sets too), so the eventually-serviced interrupt is
contaminated with a spurious `TIMER1_IRQHandler` call alongside the
real CCP2 event. This is a live demonstration of the exact
"schedule-miss hazard" `docs/superpowers/plans/2026-08-07-swuart-v3.md`
already calls out (`SWUART_LEAD_CYCLES`'s reason for existing: "arming
a deadline that is already in the past... means the match is silently
missed until Timer1 wraps all the way around, roughly 13ms at 20MHz").
The measurements below separate this artifact from the real per-event
CCP cost using a controlled variant, and the write-up below records
what it implies for `SWUART_LEAD_CYCLES` and for `EPIC_CCP_Init`'s
call ordering.

## What was run

Probe source (verbatim, per this task's brief step 1),
`/tmp/swuart-v3-probe/probe.c`, PIC16F877A. Compiled with:

```sh
mkdir -p /tmp/swuart-v3-probe && cd /tmp/swuart-v3-probe
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin:/opt/microchip/mplabx/v6.35/mplab_platform/bin
REPO=/home/alexis/projects/epicurus
xc8-cc -mcpu=16f877a -I$REPO/pic16f87xa-hal/include/target -I$REPO/pic16f87xa-hal/include -I$REPO/epic-common/include \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile probe.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_timer1.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_ccp.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq_dispatch.c \
  $REPO/pic16f87xa-hal/src/core/pic16_isr_vector.c \
  -o probe.hex -ginhx32
```

(The brief's DFP path was correct verbatim: `/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8`.)

`pic16_irq_dispatch.c` declares every peripheral `IRQHandler` with a
strong prototype, so linking only the CCP-relevant sources requires
link-satisfying empty stubs for the peripherals not under test
(`TIMER0_IRQHandler`, `TIMER2_IRQHandler`, `SSP_IRQHandler`,
`USART_RX_IRQHandler`, `USART_TX_IRQHandler`, `ADC_IRQHandler`,
`EEPROM_IRQHandler`, `COMP_IRQHandler`, `PSP_IRQHandler`); their flags
are never set in this probe so the dispatcher's own flag-guards mean
these bodies never execute and cannot perturb the measurement.
`RB_IRQHandler` is not stubbed, it comes from the real
`pic16f87xa_gpio.c` already linked.

Symbol addresses from the linked ELF (`readelf -s probe.elf`):
`_on_ccp2_compare` 0x0585, `_CCP2_IRQHandler` 0x0610,
`_on_ccp1_capture` 0x05AC, `_CCP1_IRQHandler` 0x0624,
`_PIC16_IRQ_Handler` (dispatch entry, called from the vector
trampoline) 0x065C. The vector trampoline (`x /30i 0x0004` in `mdb`)
runs 0x0004 -> 0x0011, `GOTO 0x65C`. The `retfie` address was found by
disassembling forward from 0x065C (`x /80i 0x65C` in `mdb`, since
XC8's `-fasmfile` does not reliably produce a full linked listing):
`epic_dispatch_all_irqs` is called at 0x065E (`CALL 0x218`), and after
it returns the context-restore sequence ends in `RETFIE` at **0x066B**.

## Measured results: the verbatim probe's schedule-miss artifact

At the vector-entry breakpoint (`break *0x0004`), the verbatim probe's
first serviced interrupt shows:

```
print PIR1   -> PIR1=1   (bit0 = TMR1IF, set)
print PIR2   -> PIR2=1   (bit0 = CCP2IF, set)
print TMR1H  -> TMR1H=2
print TMR1L  -> TMR1L=13   (TMR1 = 525, just past 521)
```

Breaking right after `EPIC_IRQ_Restore(1)` sets GIE (address 0x0681,
the `RETURN` after `BSF INTCON,GIE`) shows why: `TMR1H=5, TMR1L=16`
(TMR1 = 1296) with `PIR1=0, PIR2=0`, i.e. the live compare match at
521 already came and went, unmatched, before interrupts were even
enabled. `Stopwatch` from reset to this first ISR entry reads **67008
cycles**, matching 65536 (one full Timer1 wrap) + 1472 (521 + the
~950-cycle gap between the missed live match and eventual re-match)
within measurement noise. Chaining a second `Continue`/`Continue` pair
past a full ISR pass shows the *next* entry is **another** ~65000+
cycles away, not ~521: because `on_ccp2_compare`'s
`g_deadline += 521u` recurrence blindly increments a deadline that
Timer1 (which keeps running through the ISR) has often already passed
by the time it is re-armed, every subsequent event repeats the same
full-wraparound miss once it falls behind once. This is the plan's own
documented hazard (`docs/superpowers/plans/2026-08-07-swuart-v3.md`
§`SWUART_LEAD_CYCLES`), reproduced empirically, not merely asserted.

The full contaminated ISR (0x0004 -> 0x066B, before `retfie`) measures
537-538 cycles across two reproductions. Segment breakdown
(breakpoint pairs, `Stopwatch clear` / `Continue` / `Stopwatch`):

| Segment | Addresses | Cycles |
|---|---|---|
| Trampoline | 0x0004 -> 0x065C | 15 |
| Dispatch fan-out up to the TMR1IF test | 0x065C -> 0x0228 | 27 |
| **Spurious `TIMER1_IRQHandler` call** (TMR1IF happened to be set) | 0x0228 -> 0x0231 | **256** |
| Remaining fan-out to CCP2 entry | 0x0231 -> 0x0610 | 33 |
| CCP2 handler + callback + restore | 0x0610 -> 0x066B | 221 |
| **Total (contaminated)** | | **537** (+2 for `retfie` = 539, 103% of budget) |

That 256-cycle chunk is not part of the CCP mechanism under test, it
is the real cost of a coincidental `TIMER1_IRQHandler` call (itself
still paying the `EPIC_IRQ_GetFlag`/`ClearFlag` table-driven cost,
unaffected by Task 1's fix since Task 1 only touched the CCP
handlers). Reporting 539/521 = 103% as "the CCP mechanism's cost"
would be wrong; it is an artifact of this specific probe's startup
race, not a property of a properly-armed CCP compare/capture event.

## Measured results: clean, isolated events

A controlled variant, `/tmp/swuart-v3-probe-clean/probe.c` (identical
to the brief's verbatim probe except `CCP2`'s initial `CompareValue`
is `3000u` instead of `521u`, comfortably past the ~1300-cycle setup
window so the live compare catches on its first, real pass with no
Timer1 wraparound involved), confirms the CCP mechanism's real cost
directly instead of only by subtraction. At the vector-entry
breakpoint: `PIR1=0` (no `TMR1IF`), `PIR2=1` (`CCP2IF` only),
`TMR1 = 3003`. Clean segment breakdown:

| Segment | Addresses | Cycles |
|---|---|---|
| Trampoline | 0x0004 -> 0x065C | 15 |
| Dispatch fan-out to CCP2 entry | 0x065C -> 0x0610 | 50 |
| CCP2 handler + callback + restore | 0x0610 -> 0x066B | 221 |
| **Total** | 0x0004 -> 0x066B + `retfie` | **286 + 2 = 288** |

288/521 = **55% of budget, fits with 233 cycles (45%) of margin.**
Reproduced twice (286 both runs). Cross-check: `0x0004 -> 0x0228`
(27) + `0x0231 -> 0x0610` (33) = 60 from the contaminated run's
non-TMR1 fan-out portions, versus 65 (15+50) here; consistent within
the ~5-cycle cost of the `BTFSS`/`GOTO` skip-vs-take difference on the
`TMR1IF` test. The CCP2 handler segment itself (0x0610 -> 0x066B) is
**221 cycles in both the contaminated and clean runs**, i.e. unaffected
by the startup race, exactly as expected since that race lives
entirely in the dispatch fan-out before CCP2's own handler runs.

**Capture event, real pin injection (not the compare-only fallback):**
driving `RC2` (CCP1's capture pin) via `mdb`'s `write pin RC2 high`
then `write pin RC2 low`, done from a breakpoint placed right after
`EPIC_IRQ_Restore(1)` (address 0x0681, so the pin toggle lands well
before Timer1 could accumulate 65536 cycles of drift), produced a
genuine, isolated capture event: `PIR1=4` (`CCP1IF` only, bit0
`TMR1IF` clear), `PIR2=0`. The brief anticipated pin injection might
not work and pre-approved falling back to compare-only measurement;
it was not needed here, `write pin <name> high|low` worked directly
(the doc's suggested `write pin` syntax needed `high`/`low`, not
`1`/`0`, which mdb rejects with "Invalid argument for pin state
setting"). Segment breakdown:

| Segment | Addresses | Cycles |
|---|---|---|
| Trampoline | 0x0004 -> 0x065C | 15 |
| Dispatch fan-out to CCP1 entry (CCP1IF is tested earlier than CCP2IF in the fan-out order) | 0x065C -> 0x0624 | 28 |
| CCP1 handler + callback (`EPIC_CCP_GetCapture`'s atomic read loop + arithmetic + `SetCompare`/`SetMode`) + restore | 0x0624 -> 0x066B | 359 |
| **Total** | 0x0004 -> 0x066B + `retfie` | **402 + 2 = 404** |

404/521 = **78% of budget, fits with 117 cycles (22%) of margin.**
Reproduced twice (402 both runs). The capture handler's own segment
(359 cycles) is noticeably larger than the compare handler's (221
cycles), consistent with `on_ccp1_capture`'s extra work: the atomic
capture-register read-with-retry loop (`EPIC_CCP_GetCapture`) that
`EPIC_CCP_SetCompare` alone does not pay.

## Dispatch-to-handler latency (for `SWUART_LEAD_CYCLES`)

Measured directly (clean runs), vector entry to the first instruction
of the `EventCallback` body:

- CCP2 compare: 0x0004 -> `_on_ccp2_compare` (0x0585) = **95 cycles**.
- CCP1 capture: 0x0004 -> `_on_ccp1_capture` (0x05AC) = **73 cycles**.

`SWUART_LEAD_CYCLES` (`docs/superpowers/plans/2026-08-07-swuart-v3.md`,
current guess 40, "start at 40... confirm or adjust against Task 2's
real measurement") governs a related but distinct latency: the gap
between reading Timer1 in `EPIC_SWUART_Write`'s mainline (non-ISR)
first-arm and the moment `SetCompare`/`SetMode`'s writes are live,
not this ISR's own dispatch latency. This probe did not isolate that
exact mainline call sequence (it does not exist yet; it lands in Task
3+). What it does show, directly, is that every real latency measured
here (73-95 cycles for ISR dispatch, 30 cycles for each `CCPn_IRQHandler`'s
own flag-clear-plus-callback-indirection overhead) is comfortably
above 40. Recommendation for Task 3: **do not keep the 40-cycle guess
as-is; raise `SWUART_LEAD_CYCLES` to at least 100** to have real margin
above the smallest overhead this probe actually measured on the same
hardware/compiler, and re-derive it precisely once the real
mainline arm sequence exists (it should be measured the same way, not
guessed a second time).

## A second, independent finding for Task 3: `EPIC_CCP_Init`'s write ordering

`EPIC_CCP_Init()` (`pic16f87xa_ccp.c`) calls
`EPIC_IRQ_ClearFlag`/`EPIC_IRQ_Enable` (table-driven, expensive)
*before* writing `CCPRxL`/`CCPRxH`/`CCPxCON`. If Timer1 is already
running when `EPIC_CCP_Init` is called with a compare deadline that is
near "now" (as this probe's verbatim `CompareValue = 521u` was), the
live match can be missed entirely before the module is even armed,
falling back to a 65536-cycle wraparound to recover, exactly the
"much worse failure than a late tick" the v3 plan already calls out
for the steady-state re-arm case. This is a real, reproduced,
mechanism-level instance of the same hazard, but at *initialization*
time rather than steady-state re-arm time, and it is worth Task 3
either avoiding `EPIC_CCP_Init`'s table-driven calls on the swuart's
hot init path, or bounding the smallest safe initial `CompareValue`
relative to `EPIC_CCP_Init`'s own real cost (order of ~150-250 cycles
per instance, from `EPIC_IRQ_ClearFlag` + `EPIC_IRQ_Enable`'s combined
table lookups), not just relative to `SWUART_LEAD_CYCLES`.

## Verdict against the 521-cycle budget

| Event | Real measured cycles (clean) | % of 521-cycle budget | Fits? |
|---|---|---|---|
| CCP2 compare (`CCP2_IRQHandler` -> `on_ccp2_compare`) | 288 | 55% | **Yes**, 233-cycle margin |
| CCP1 capture (`CCP1_IRQHandler` -> `on_ccp1_capture`, real pin injection) | 404 | 78% | **Yes**, 117-cycle margin |

Both events measured on the real, compiled, linked driver code (not a
mirror), fit the 521-cycle budget with real margin. Task 1's fix
(direct `PIR1`/`PIR2` bit clear in `CCP1_IRQHandler`/`CCP2_IRQHandler`
instead of table-driven `EPIC_IRQ_GetFlag`/`ClearFlag`) is doing its
job: each handler's own flag-clear-plus-dispatch overhead measured at
just 30 cycles. Tasks 3 onward may proceed on the CCP approach.
Two follow-up items are not optional cleanup, they are load-bearing
for Task 3's correctness: raise `SWUART_LEAD_CYCLES` from 40 to at
least 100 cycles, and account for `EPIC_CCP_Init`'s own ~150-250-cycle
table-driven setup cost when choosing how soon after `Init` a real
swuart channel's first deadline may safely land.
