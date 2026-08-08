# epic-swuart RX hot-path fix: relative-reload timing + direct SFR access

Status: **implemented 2026-08-08**. PIC16F87XA channel A ships the
collapsed one-pass RX state machine with the measured
`RX_CAPTURE_OVERHEAD_CYCLES = 325`; see
`docs/superpowers/plans/probe-swuart-rx-hotpath.md` for the real `mdb`
measurements and verdict, and `docs/superpowers/plans/2026-08-08-swuart-rx-hotpath.md`
for the plan that executed it. PIC18Fxx5x and PIC16F193X's channel B
still carry the old confirm-then-arm scheme until a follow-up ports
this pattern.

A refinement of `docs/superpowers/specs/2026-08-07-swuart-v3-design.md` (v3),
not a new timing-engine generation. v3's CCP capture/compare architecture
stays: RX still uses CCP Capture mode, TX still uses CCP Compare mode.
This spec only replaces v3's RX confirm-and-first-sample sequence and
cuts real, measured overhead from the RX capture handler on
PIC16F87XA. TX is unchanged and out of scope entirely.

## Problem

v3's RX confirm step arms a *second* CCP compare event at an absolute
deadline, `edge_time + 0.5 * cycles_per_bit` (260 cycles at 9600
baud/20 MHz), racing a free-running Timer1 to that value. The real
measured cost to service the capture interrupt and arm that deadline
is 404 cycles (`docs/superpowers/plans/probe-swuart-v3-ccp-cost.md`).
404 > 260: the deadline is already in the past by the time software
arms it, on every single received byte, recoverable only via a 65536
cycle Timer1 wraparound. This is almost certainly the unresolved cause
of Task 8's real-hardware RX loopback producing a mismatched byte
instead of the correct one.

Comparing against Microchip's own AN555 ("Software Implementation of
Asynchronous UART Functions", 1997, PIC16C71/6X/7X/8X, same classic
PIC16 core generation as PIC16F877A) surfaced two things. First,
AN555's own fastest demonstrated case, 19200 baud at 10 MHz, has only
~130 cycles per bit of budget, a quarter of our 521, and works
correctly in hand-written assembly. Our own 404-cycle RX cost against a
4x larger budget is not the silicon being harder to work with; it is
identifiable, real overhead this codebase's own HAL abstraction adds
on top of the same core. Second, AN555's timing model does not have
our race at all: its deglitch check is unscheduled (checked
synchronously, in the same interrupt invocation that detected the
edge, not at a precomputed future instant), and the next real timing
decision is a *relative* reload from the moment of that check, not an
*absolute* deadline computed from the original edge, corrected by one
small fixed latency constant (`_Cycle_Offset1 = 24` in their source).
Because nothing is running independently while software decides what
to load, there is no clock to lose a race against.

## Decisions

| Question | Decision |
|---|---|
| Scope | PIC16F87XA only, channel A (CCP1 = RX). Same pattern ported to PIC18Fxx5x and PIC16F193X (including PIC16F193X's channel B, CCP3) in a follow-up, once this family's real numbers are proven. TX (`tx_compare_event`, `EPIC_SWUART_Write`) is untouched |
| RX state machine | Collapse `RX_IDLE` and `RX_CONFIRM_START` into one state, one synchronous handler pass. The deglitch check (is the pin still low) happens immediately inside the capture handler, not as a separately scheduled compare event. `RX_CONFIRM_START` is removed from the enum |
| Deglitch tradeoff, disclosed | Checking "still low right now" instead of "still low at a confirmed half-bit-period mark" is a slightly weaker noise filter. AN555 makes the identical tradeoff for the identical reason (its own confirm check is also synchronous, not scheduled). Accepted, not hidden |
| First-sample deadline | Relative reload: read Timer1 fresh inside the handler (after the handler's own overhead has already elapsed), then arm `now + (1.5 * cycles_per_bit - RX_CAPTURE_OVERHEAD_CYCLES)`. `RX_CAPTURE_OVERHEAD_CYCLES` is a real, measured constant (this plan's own AN555-style `_Cycle_Offset1`), not guessed, derived from a real `mdb` probe of the new handler exactly the way `SWUART_LEAD_CYCLES = 120` was derived, not assumed from the old 404-cycle number (that number describes the old, different code path) |
| Hot-path register access | New `epic_swuart.c`-internal helpers read `CCPR1H:CCPR1L` and write `CCPR1H:CCPR1L`/`CCP1CON` via PIC16F87XA's known literal addresses (`0x16`/`0x15`/`0x17`), bypassing `EPIC_CCP_GetCapture`'s atomic retry-loop and `EPIC_CCP_SetCompare`/`SetMode`'s generic call overhead, for this one hot path only. Hardcoded to CCP1 specifically, not parameterised by instance: making it generic would reintroduce the branch/lookup overhead this exists to remove. Channel B's own fast path (CCP3, PIC16F193X) gets its own separate hardcoded helper when ported, matching the existing `on_rx_event_a`/`_b` thin-wrapper-per-slot pattern already in this file |
| Retry-loop removal, justified | `EPIC_CCP_GetCapture`'s retry loop defends against a second real capture landing mid-read. Skipping it in this one call site is safe: the earliest a legitimate second falling edge can occur is one full bit period away (~521 cycles at 9600 baud), vastly longer than the ~10-15 cycles a plain 2-byte read takes. This is a targeted, justified simplification for this one call site, not a change to `EPIC_CCP_GetCapture` itself, which keeps its retry loop for every other caller |
| Public API | `EPIC_CCP_GetCapture`/`SetCompare`/`SetMode` and `EPIC_SWUART_Init/DeInit/Write/Read/GetErrorCount` are unchanged. Framing and error-handling behavior are unchanged |
| Testing | Host-sim cannot model real CCP timing at all (confirmed repeatedly in v3's own execution); the existing test-hook idiom (`epic_swuart.c`'s `#if EPIC_SWUART_TEST_HOOKS`-gated functions, fired directly by tests) continues to verify state-machine *correctness*. The real margin claim can only be validated by a real `mdb` probe, which is mandatory and comes first in the resulting plan, the same discipline that caught the original 404-vs-260 race and the 404-vs-130 AN555 gap in the first place |

## Architecture

### Collapsed RX entry: one pass, not two

Old: capture fires -> arm a second compare event 0.5 bit-periods away ->
(separately, later) that event fires -> check the pin -> if real, arm a
*third* event for d0's sample. Three interrupt round-trips, one of
which never actually meets its own deadline.

New: capture fires -> read the captured edge time (fast path) ->
immediately check the pin -> if noise, switch back to
`CCP_MODE_CAPTURE_FALLING` and stop; if real, compute d0's deadline as
a relative reload from Timer1's *current* value and arm it (fast path)
-> switch to `CCP_MODE_COMPARE_SOFT_IF`. Two interrupt round-trips
total for start-bit sync (capture, then d0's own sample event), both
real, neither racing anything.

```c
#define CCP1_CPRL_ADDR 0x15U
#define CCP1_CPRH_ADDR 0x16U
#define CCP1_CON_ADDR  0x17U

/* RX_CAPTURE_OVERHEAD_CYCLES: cycles elapsed, on real PIC16F87XA
 * hardware, between the real falling edge and the point in
 * rx_capture_event_fast() where Timer1 is read fresh (the AN555-style
 * _Cycle_Offset1 correction). Measured via a real mdb probe, the first
 * task of this spec's plan; do not guess this value or carry over the
 * old 404-cycle capture-handler total, which describes different code. */
#define RX_CAPTURE_OVERHEAD_CYCLES 325u /* measured on PIC16F877A, see
  docs/superpowers/plans/probe-swuart-rx-hotpath.md: 3 cycles edge-to-vector
  plus 322 vector-to-Timer1-read */

static void rx_capture_event_fast(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state != RX_IDLE) {
        /* steady-state per-bit sampling: unchanged from v3, still a
         * full cycles_per_bit hop per event, already proven to fit
         * comfortably (v3's TX-side measurement, 288/521 cycles, is
         * representative of this same class of per-bit compare-event
         * cost; RX's own per-bit event was not separately isolated by
         * v3's probe and should be spot-checked by this plan's probe
         * too, not assumed identical). */
        uint8_t sample = (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_SET) ? 1u : 0u;

        if (h->rx_state == RX_STOP) {
            if (sample != 0u) {
                rx_push(h, h->rx_shift);
            } else {
                h->error_count++;
            }
            h->rx_state = RX_IDLE;
            EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_CAPTURE_FALLING;
            return;
        }

        h->rx_shift = (uint8_t)((h->rx_shift >> 1) | (sample ? 0x80u : 0u));
        h->rx_bit_index++;
        h->rx_state = (h->rx_bit_index < 8u) ? (uint8_t)(RX_DATA0 + h->rx_bit_index) : RX_STOP;
        h->rx_deadline = (uint16_t)(h->rx_deadline + g_cycles_per_bit);
        EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
        EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
        return;
    }

    /* RX_IDLE: a start-bit falling edge just latched into CCPR1.
     * Immediate deglitch check, no second scheduled event. */
    if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
        /* noise: pin already back high, stay in Capture mode. */
        return;
    }

    h->rx_shift = 0u;
    h->rx_bit_index = 0u;
    h->rx_state = RX_DATA0;

    uint16_t now = EPIC_TIMER1_ReadCounter();
    uint16_t target_offset = (uint16_t)(g_cycles_per_bit + g_cycles_per_bit / 2u);
    h->rx_deadline = (uint16_t)(now + target_offset - RX_CAPTURE_OVERHEAD_CYCLES);

    EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
    EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
    EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_COMPARE_SOFT_IF;
}
```

(This is illustrative of the mechanism, not a final diff; the
implementation plan works out the exact call-site wiring, including
how `test_get_capture`'s test-hook indirection and the removed
`RX_CONFIRM_START` enum value ripple through the rest of the file.)

### Why "now + target_offset - overhead" and not "edge_time + 1.5x"

`now` is read *after* the deglitch check, i.e. after the handler's own
real latency has already elapsed. `target_offset` is what the deadline
would need to be if `now` were exactly the edge time. Since `now` is
already later than the edge by the handler's real overhead,
subtracting that overhead once corrects for it, landing the arm at
approximately the intended 1.5 bit-period mark relative to the *real*
edge, not relative to `now`. This is arithmetically identical to
AN555's `_Cycle_Offset1` correction, just against a much larger real
overhead than their ~24 cycles, which is exactly why this plan's first
task must measure it for real rather than assume a small constant
works.

### Steady-state per-bit sampling is unchanged

Only the capture-to-first-sample transition changes. Every subsequent
per-bit compare event (`RX_DATA0` through `RX_STOP`) keeps v3's
existing absolute-deadline-plus-one-`cycles_per_bit` arithmetic: once
inside the byte, each event only needs to beat the *next* event by a
full bit period, not a half one, and v3's TX-side measurement (288 of
521 cycles) already demonstrates that class of event fits with real
margin. This plan's probe should confirm RX's own per-bit event
directly rather than assume TX's number transfers exactly, since the
RX side does a pin read and a shift the TX side doesn't.

## Error handling

Unchanged: one error counter per handle, incremented on a bad stop bit
or RX ring overflow (`rx_push`), read atomically by
`EPIC_SWUART_GetErrorCount`. The deglitch check's noise-rejection path
(pin already high) is now a single early return with no state change,
functionally equivalent to v3's `RX_CONFIRM_START` noise branch, just
without the intervening scheduled event.

## Testing

Host-sim: `epic_swuart.c`'s `#if EPIC_SWUART_TEST_HOOKS` functions get
updated for the collapsed state machine (no more firing a separate
"confirm" event; one fire now does capture-and-arm-first-sample in a
single call). `test_swuart_rx.c` and `test_swuart_dual.c` need their
event-firing sequences reduced by one fire per received byte to match.

Real-target: a new `mdb` probe, this plan's mandatory first task,
measuring the real cost of `rx_capture_event_fast`'s `RX_IDLE` branch
on PIC16F877A, from which `RX_CAPTURE_OVERHEAD_CYCLES`'s real value is
derived, the same way `SWUART_LEAD_CYCLES = 120` was derived from a
real probe rather than guessed. If the real number does not leave
comfortable margin against 9600 baud's 521-cycle budget even after
this fix, that is a real finding to report back, not something to
paper over with a larger fudge factor.

## What this spec deliberately does not do

- Touch TX (`tx_compare_event`, `EPIC_SWUART_Write`). It already has
  real, measured, comfortable margin and no race condition.
- Change the public API, framing, or error-handling contract.
- Port the fix to PIC18Fxx5x or PIC16F193X in this pass. Same pattern,
  different literal addresses, a follow-up once PIC16F87XA's real
  numbers are proven.
- Remove `EPIC_CCP_GetCapture`'s retry-loop protection for any caller
  other than this one hot-path call site. The generic accessor keeps
  its safety guarantee for everyone else.
- Extend host-sim to model real CCP timing. Still not possible, still
  out of scope, same as v3.
