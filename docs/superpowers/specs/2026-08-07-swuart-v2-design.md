# epic-swuart v2: edge-triggered timing engine, design

Status: **agreed 2026-08-07, superseded 2026-08-07 before its own
Tasks 5-7 were ever executed**. Only Tasks 1-4 of this design's plan
(`docs/superpowers/plans/2026-08-07-swuart-v2.md`) shipped (scheduler
core, straight-line dispatch, edge-triggered RX); a real `mdb`
measurement of the actual compiled result taken after those four tasks
landed (1019 cycles against the 521-cycle budget, 196% over) is what
triggered the second timing-engine supersession recorded below, before
Tasks 5-7 (error-handling/DeInit test carry-forward, the real-target
`mdb` gate, and documentation) were ever started under this plan. The
API, framing, and error-handling contract described below are still
accurate and unchanged; the timing engine is replaced by
`docs/superpowers/specs/2026-08-07-swuart-v3-design.md`'s CCP hardware
capture/compare design. Read that document for the current timing
architecture; this one is kept for the parts still in force and as the
historical record of why the pin-change-interrupt/software-scheduler
approach was chosen and where it fell short in practice.

Supersedes the timing-engine portions of
`docs/superpowers/specs/2026-08-06-swuart-design.md` (the v1 design). Not
a rewrite of the module: the API, framing, error handling, and module
boundaries below are the same ones v1 already shipped and are called out
explicitly as unchanged, not re-derived.

## Problem

v1 shipped, passed every host-sim test on all three families, and built
clean real-target `.hex` files, but a final review measured the real
compiled ISR on hardware (via `mdb` single-stepping, not estimation) and
found it costs 562-677 cycles per shared tick against a 174-cycle budget,
roughly 3x over. The chosen timing model, a fixed-rate timer firing 3x
per bit (N=3 oversampling) and walking every active channel on every
tick, divides the CPU's response budget by 3 for a benefit (sampling
precision) that a well-timed single sample doesn't need. A cheap fix
(optimizing the shared interrupt dispatch every module in this repo goes
through) was tried, measured honestly, and closed only part of the gap
(677 to 562 cycles, kept, real and worth having regardless) without
reaching budget. See `docs/superpowers/plans/probe-swuart-isr-budget.md`
and the `swuart-bitbang` branch history for the full measurement trail.

Microchip's own reference design for this exact problem, AN2290
("Bit-Banged Enhanced UART for 8-Bit PIC MCUs"), does not use continuous
oversampling: it detects the start bit via a pin-change interrupt, then
times the remaining bits with the timer reprogrammed to fire once per
bit, not N times per bit. This design adopts that approach, adapted for
two channels running full duplex at once instead of AN2290's one
half-duplex channel.

## Decisions

| Question | Decision |
|---|---|
| Scope of this redesign | Evolution, not a rewrite. `EPIC_SWUART_Init/DeInit/Write/Read/GetErrorCount`, the ring-buffered non-blocking API shape, 8N1 framing, 9600-baud-only scope, and the per-family fixed-contract HAL usage are all unchanged from v1 and are not revisited here |
| Start-bit detection | Pin-change interrupt on the RX pin's falling edge, not continuous polling |
| Bit timing after detection | One timer event per bit (not oversampled), same `RX_CONFIRM_START` half-bit deglitch check v1 already has |
| RX pin placement | Any interrupt-capable pin, not literally any GPIO pin: RB4:RB7 (a shared 4-pin group interrupt) on PIC16F87XA/PIC18Fxx5x, any PORTB pin (per-pin interrupt) on PIC16F193X. Already agreed with the user; TX pins are unaffected, they're never inputs |
| Multi-channel timing | A small, fixed-size (4-slot: TX and RX x 2 channels) soft-timer scheduler multiplexed off the one shared Timer1, not one timer per channel |
| Idle cost | Timer1 stops entirely when no channel has anything in flight (a real improvement over v1, which ticked constantly regardless of activity) |
| Dispatch optimization from the fix wave | Kept. Independent of this redesign, already verified correct on all three families, benefits every interrupt-driven module in the repo |
| Real-target verification | Add `epic-swuart` to `scripts/ci-target-sim.sh`'s `mdb` gate. v1 shipped with zero real-hardware behavioral verification, only "it compiles"; this redesign does not repeat that gap |

## Architecture

### The scheduler: four slots, one timer

Two channels, each full duplex, means up to four independent "next bit
is due" deadlines can be in flight: TX and RX for channel A, TX and RX
for channel B. One re-programmable one-shot timer (still Timer1, still
exclusively owned, same resource-ownership story v1 already documents)
services all four by always being set to fire at whichever deadline is
soonest.

Each active operation tracks a `uint16_t` cycle countdown, not an
absolute time. When the timer fires, the exact number of cycles that
just elapsed is known (it is whatever delta was programmed), so that
same delta is subtracted from every other active slot's countdown, the
slot(s) that hit zero are serviced, each computes a fresh one-bit-period
countdown (or drops out if its byte/frame is done), and the timer is
reprogrammed to the new minimum countdown across whatever remains
active. If nothing is active, the timer stops; a start-bit edge on an
armed RX pin is what wakes a channel back up.

This needs no dynamic allocation, no sorting, and no general-purpose
scheduler machinery: at most 4 slots, a linear minimum-of-4 comparison.

### RX: same state machine, new trigger

`RX_IDLE -> RX_CONFIRM_START -> RX_DATA0..7 -> RX_STOP` is v1's existing,
already-tested state machine and does not change in shape. What changes
is what drives `RX_IDLE -> RX_CONFIRM_START`: v1 polls the pin level
every fixed tick; v2 arms a pin-change interrupt while idle, and the
falling edge is what triggers the transition and schedules the
half-bit-later confirm check via the timer scheduler above. Every
sample point after that (the deglitch confirm, each data bit, the stop
bit) is one scheduler-driven timer event, one bit period apart, exactly
as v1's tick-counted version already spaces them, just driven by
deadlines instead of a fixed divider.

Where two channels' RX pins share one hardware interrupt (the RB4:RB7
group on two of three families), the shared handler reads the port once
and checks each *active* channel's specific pin bit against that one
read, using the existing `EPIC_GPIO_RegisterChangeCallback` API already
in every family's HAL. No new HAL functionality, no per-family branching
in `epic_swuart.c` (the zero-`#ifdef` claim v1 established is unaffected
by this redesign).

### TX: same state machine, scheduler-driven instead of tick-driven

`TX_IDLE -> TX_DATA -> TX_STOP` (v1's bit-index-counted version, not the
earlier rejected per-bit-enum design) is unchanged in shape. `Write`
starting a transmission from idle registers a first, immediate deadline
in the scheduler instead of relying on a periodic tick to notice
`tx_count > 0`.

## Budget: real numbers, correctly caveated

One bit period at 9600 baud, 20 MHz: 521 instruction cycles (corrected;
an earlier verbal estimate of ~2083 cycles in conversation used the raw
oscillator rate instead of the instruction-cycle rate and was wrong).
That is roughly 3x the old 174-cycle N=3 budget, not 12x.

The measured worst-case single-tick cost under v1's design (two
channels, TX and RX both mid-byte, serviced in the same interrupt) was
562-677 cycles, which is *not* comfortably under 521. The difference
that makes v2 viable is not a huge per-event margin, it is that the
"everything is due at once" case stops being universal (every 174
cycles, forever) and becomes rare (only when independent, unsynchronized
deadlines happen to coincide). A single interrupt occasionally running a
little over one bit period causes a little timing jitter, corrected at
the next scheduled event, not the permanently-behind state v1's
always-over-budget design was in.

**This is an open verification item, not a settled fact.** The
plan's first task must measure the real, compiled v2 ISR the same way
every prior probe in this project has: real `mdb` single-stepping on
PIC16F87XA, not an estimate. If the common case (one slot due) and the
rare coincidence case (multiple slots due) are both measured and only
the rare case is tight, that is an acceptable, understood tradeoff. If
even the common case does not fit, that is a real finding the plan must
surface, the same way the v1 probe surfaced its own gap.

## Error handling

Unchanged from v1: one error counter per handle, bad stop bit or RX ring
overflow both increment it, `EPIC_SWUART_GetErrorCount` reads it under
`EPIC_IRQ_Disable`/`Restore` (a fix-wave addition, kept).

## Testing

Host-sim tests are rewritten, not extended: v1's `CYCLES_PER_TICK`-based
fixed-rate pumping has no equivalent in an event-driven design. Tests
instead drive simulated pin edges (`*_sim_drive_input`) and let the
*real* simulated Timer1 fire on its own schedule (the host sim already
models real peripheral timing, this project has relied on that since
Task 1's probe), checked via `epic_harness_tick()` run until the
simulated interrupt actually fires, not a fixed loop count. The existing
test *scenarios* (single-channel TX, single-channel RX, two channels at
once, framing error, RX overflow, dual-channel `DeInit`) all still need
covering; only the mechanism that drives simulated time changes.

Real-target: `scripts/ci-target-sim.sh` gains an `epic-swuart` entry
(the manifest entry and `tests/example_swuart.c` already exist from v1),
so CI actually runs the compiled firmware under `mdb` rather than only
building it. This is also how the plan's first-task budget
re-measurement gets done for real, not a one-off local probe.

## What this design deliberately does not do

- Change the public API, framing, baud-rate scope, or error-handling
  contract. All of that is v1's, already shipped, already reviewed.
- Support RX on a genuinely arbitrary GPIO pin. Interrupt-capable pins
  only, already agreed.
- Solve for more than two concurrent channels, or for baud rates other
  than 9600.
- Revisit the dispatch optimization from the fix wave; it is kept as-is.
