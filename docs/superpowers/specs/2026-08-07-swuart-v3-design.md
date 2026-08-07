# epic-swuart v3: CCP hardware capture/compare timing engine, design

Status: **agreed 2026-08-07, not started**.

Supersedes the timing-engine portions of
`docs/superpowers/specs/2026-08-07-swuart-v2-design.md` (v2). Not a
rewrite of the module: the public API, framing, error handling, and
module boundaries are unchanged from v1/v2 and are called out explicitly
as unchanged, not re-derived. This is the second timing-engine
supersession for this module; see v1's and v2's specs for the earlier
history (continuous oversampling, then edge-triggered software
scheduling), both real, both measured, both found insufficient in
different ways.

## Problem

v2 shipped Tasks 1-4 (scheduler core, straight-line `g_chan_a`/`g_chan_b`
dispatch, edge-triggered RX), each task-reviewed and passing host-sim
tests. A real `mdb`-measured probe of the actual compiled module (not
the simplified mirror Task 1's own probe used) found the real Timer1
ISR costs 1019 cycles on PIC16F877A at 20 MHz against the 521-cycle
budget, 196% over. This is not the timing model's fault: the scheduler
logic itself is correct and was independently re-confirmed. The cost is
in machinery the mirror never modeled: `TIMER1_IRQHandler` still calls
the table-driven `EPIC_IRQ_GetFlag`/`ClearFlag` internally (redundant,
dispatch already confirmed the flag), the `OverflowCallback` function
pointer costs a computed-goto indirection, and the real state machine's
`g_chan_a->field`/`g_chan_b->field` accesses cost real per-field
FSR/IRP bank-switching on classic PIC16 codegen. Together these account
for roughly 780 of the 1019 cycles; the scheduler's own logic is a
small fraction of the total.

Two paths were available: shave cycles from the existing software path
(bypass the redundant flag check, call the scheduler directly instead
of through a function pointer, bank-pin the handle structs), or remove
the ISR-execution-time dependency from the two operations that actually
need cycle-accurate timing (when did the RX edge happen, when should
the TX pin transition) by moving them into hardware. This design takes
the second path: all three target families' HALs already expose a CCP
(Capture/Compare/PWM) driver capable of exactly this, previously unused
by any higher-level module in this repo.

## Decisions

| Question | Decision |
|---|---|
| Scope of this redesign | Evolution again, not a rewrite. `EPIC_SWUART_Init/DeInit/Write/Read/GetErrorCount`, the ring-buffered non-blocking API shape, 8N1 framing, and 9600-baud-only scope are unchanged. RX pin placement (interrupt-capable pins, established in v2) is superseded by a new constraint: RX must be a CCP-capable pin, not any GPIO pin, since capture mode needs a dedicated hardware input |
| RX timing | CCP Capture mode. The start bit's falling edge latches Timer1's exact instruction-cycle count into `CCPRx`, in hardware, independent of ISR latency. This is the single property this redesign is built around: the recorded edge time is exact even if the ISR that reads it runs late |
| RX bit sampling after sync | The same CCP module switches from Capture to Compare mode (interrupt-only, `CCP_MODE_COMPARE_SOFT_IF`) once the start bit is captured, with `CCPRx` reprogrammed to an absolute future Timer1 count for each subsequent bit-center sample. Switches back to Capture mode after the stop bit |
| TX timing | CCP Compare mode, `CCP_MODE_COMPARE_SET` or `CCP_MODE_COMPARE_CLEAR` chosen per the actual next-bit value (not toggle mode, see below), `CCPRx` holding the absolute Timer1 count for that transition. Hardware drives the pin transition at exactly that count regardless of when software runs; software's only job is to have the *next* bit's compare value and mode loaded before the *current* one fires |
| Toggle mode (PIC18 only) | Investigated and rejected. PIC18's CCP has a hardware auto-toggle-on-match mode PIC16 lacks, initially thought to be a PIC18-specific advantage. It is not usable here: arbitrary serial data is not a fixed alternating pattern, so blind toggling would desynchronize the line the first time two consecutive bits share a value. SET/CLEAR, chosen explicitly per bit, is the correct primitive and is available identically on all three families. No per-family TX divergence needed beyond this |
| Timer1 | Free-running, continuous, never reset or rewritten per-bit (a real simplification from v2, which reprogrammed it every event). All scheduling deadlines are absolute 16-bit Timer1 counts; wraparound is handled for free by unsigned 16-bit subtraction, not by explicit rollover logic. Still exclusively owned by `epic-swuart` while any channel is active, same resource-ownership story as v1/v2 |
| Channel capacity | A real, disclosed, per-family hardware ceiling, not a user-configurable knob. One full channel (one CCP module for RX capture, one for TX compare) costs 2 CCP modules. PIC16F87XA and PIC18Fxx5x have 2 CCP modules each: max 1 channel. PIC16F193X has 5: max 2 channels, 1 module spare. `EPIC_SWUART_MAX_CHANNELS` is resolved by family detection inside `epic_swuart.h` (the same `#if defined(...)` pattern the module's own tests already use), not overridable, and `EPIC_SWUART_Init` returns `EPIC_INVALID` once the real ceiling is reached, the same convention already in place for a full registry |
| CCP resource allocation | PIC16F87XA / PIC18Fxx5x: CCP1 = RX, CCP2 = TX (the module's only channel). PIC16F193X: CCP1 = RX-A, CCP2 = TX-A, CCP3 = RX-B, CCP4 = TX-B, CCP5 unused |
| In-frame mode switches | Go through direct `CCPxCON`/`CCPRx` register writes inside `epic_swuart.c`, not the generic `EPIC_CCP_Init` (which re-does IRQ-enable bookkeeping unnecessary mid-frame, the same class of overhead this whole redesign exists to remove). The public `EPIC_CCP_*` API is unchanged; `Init`/`DeInit` at channel setup/teardown still use it normally |
| API shape | Unchanged: handle-based, non-blocking, ring-buffered. User-visible surface is identical to v1/v2 regardless of internal per-family CCP allocation |

## Architecture

### RX: capture, then compare, then capture again

Idle: the channel's CCP module sits in `CCP_MODE_CAPTURE_FALLING`. A
start bit's falling edge latches Timer1's count into `CCPRx` in
hardware. On that capture interrupt, the handler reads `CCPRx` (the
exact edge time), computes `edge_time + 1.5 * cycles_per_bit` (the
deglitch/first-sample deadline, same half-bit-later confirm timing v1
and v2 both already used), writes it back into the same module's
`CCPRx`, and switches `CCPxCON` to `CCP_MODE_COMPARE_SOFT_IF`. Each
subsequent compare match samples the RX pin, shifts the bit in,
advances the deadline by one `cycles_per_bit`, and rewrites `CCPRx`.
After the stop bit, the module switches back to `CCP_MODE_CAPTURE_
FALLING` for the next byte.

Because every deadline is an absolute Timer1 count rather than a
relative countdown, an ISR that runs late does not drift the schedule:
it only needs to finish before the *next* match fires, not land exactly
on time for the current one. This is the property that makes the
redesign robust to ISR cost in a way v2's relative-countdown scheme was
not.

### TX: compare-driven bit generation

`Write` computes the absolute Timer1 deadline for the start bit and
programs the channel's TX CCP module with that deadline and
`CCP_MODE_COMPARE_CLEAR` (start bit is a space/low). Each subsequent
compare match's handler advances the deadline by one `cycles_per_bit`
and reprograms `CCPxCON` to `CCP_MODE_COMPARE_SET` or `_CLEAR`
depending on the next bit's actual value (LSB-first shift, same framing
v1/v2 already established), until the stop bit (`SET`) completes the
frame. The physical transition happens in hardware at exactly the
programmed count; software's only obligation is to have the next
value and mode ready before the current match fires.

### Two channels, two independent CCP pairs

Unlike v2's single shared Timer1 with a 4-slot software scheduler, each
channel here owns its own CCP module pair, both reading the same
free-running Timer1 as their time base but otherwise fully independent:
no shared dispatch table, no cross-channel scheduling logic, no
straight-line-vs-loop dispatch concern (the concern that drove v2's own
`g_chan_a`/`g_chan_b` design no longer applies in the same way, since
there is no single ISR juggling multiple channels' due-ness; each
channel's CCP interrupt is its own, self-contained handler). On
PIC16F193X, channel A's ISRs (CCP1/CCP2) and channel B's ISRs
(CCP3/CCP4) are separate functions with no shared state beyond each
owning `EPIC_SWUART_HandleTypeDef *`.

## Error handling

Unchanged from v1/v2: one error counter per handle, a bad stop bit or
RX ring overflow both increment it, `EPIC_SWUART_GetErrorCount` reads it
under `EPIC_IRQ_Disable`/`Restore`.

## Testing

**Host-sim, a new, more severe gap than v2's already-accepted one.**
None of the three families' host simulators model CCP capture (latching
a simulated timer value into `CCPRx` on a simulated edge) or compare
(firing at a simulated timer match) at all, unlike v2's RX gap where at
least PIC16F193X's simulator modeled real IOC edge detection. Tests
follow the same category of workaround v2 already established for the
RBIF/IOC gap, just applied to CCP: manually write the simulated
`CCPRx`-backing state and call the relevant `CCPn_IRQHandler()`
directly to drive the state machine, rather than relying on the
simulator to generate capture/compare events on its own. This is a
real, disclosed testing-methodology limitation, not a silent one;
extending the host simulators to model CCP capture/compare is a
separate, real HAL-improvement project, out of scope here, the same
category of deferred work v2 already logged for RBIF/IOC.

**Real-target**: the `mdb` gate work from v2's own (unexecuted) Task 6
carries forward unchanged in spirit: real compiled firmware, run under
MPLAB SIM, checked for a real pass/fail marker. The specific probe that
motivated this whole redesign (measuring the real ISR's cycle cost)
should be re-run against the CCP-based implementation before trusting
it, the same "probe before trusting" discipline that has now caught a
real gap twice in this module's history.

## A documented hazard, unrelated to CCP, found during this investigation

`EPIC_GPIO_WritePin` on PIC16F87XA reads the *physical* PORT register
(this family has no LAT register) and writes the merged byte back. Two
output pins sharing a PORT (for example this module's own TX pins, or a
TX pin and any other output on the same port) that transition close
together in real time risk one write reading the other's not-yet-risen
voltage and clobbering it, under real capacitive loading (long wires,
level shifters). MPLAB SIM cannot catch this, it models digital logic
with zero rise time. Not a CCP concern and not fixed by this redesign
(the hardware-timed CCP transitions actually reduce how close together
two software-driven writes on the same port can land, but do not
eliminate the underlying RMW hazard for whatever else shares that
port). Recorded here so it lands in the module's documentation
(Task work for whoever writes the v3 implementation plan) rather than
staying an undocumented, previously-unknown risk.

## What this design deliberately does not do

- Change the public API, framing, baud-rate scope, or error-handling
  contract. All of that is v1's, unchanged through v2 and this redesign.
- Support more than one channel on PIC16F87XA or PIC18Fxx5x, or more
  than two on PIC16F193X. This is a real hardware ceiling (CCP module
  count), not an arbitrary limit, and is not user-configurable.
- Extend the host simulators to model CCP capture/compare. A real,
  separate, disclosed gap, the same category as v2's RBIF/IOC gap.
- Use PWM mode or any CCP capability beyond capture and compare.
- Fix the `EPIC_GPIO_WritePin` RMW hazard on PIC16F87XA. Documented,
  not addressed; a HAL-level fix (a shadow-register write path) is a
  separate, real project.
