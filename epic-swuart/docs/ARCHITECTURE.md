# epic-swuart architecture

## Overview: CCP hardware timing, not software scheduling

v1 polled every GPIO pin on a fixed-rate tick (3x oversampled). v2 kept
a software scheduler but moved RX start-bit detection to a pin-change
interrupt. Both were measured on real hardware (`mdb`, not estimated)
to cost more cycles per event than the bit period allows: v1's ISR
measured 562-677 cycles against a 174-cycle N=3 budget, v2's measured
1019 cycles against a 521-cycle one-bit-period budget. In both cases
the scheduling *logic* was correct; the cost was software's inherent
dependency on how fast the ISR runs relative to when the next bit edge
or transition needs to happen.

v3 (the current design) removes that dependency for the two operations
that actually need cycle-accurate timing, by moving them into CCP
(Capture/Compare) hardware:

- **RX**: CCP Capture mode hardware-timestamps the start bit's falling
  edge into `CCPRx` the instant it happens, regardless of how late the
  ISR that reads it runs.
- **TX**: CCP Compare mode hardware-transitions the pin at a
  programmed absolute Timer1 count, regardless of how late the ISR
  that programs the *next* one runs.

Software's job shrinks to: read a hardware-latched timestamp, or
arm the next hardware-driven transition before the current one fires.
Neither needs to complete within a bit period, only before the
following event, which real measurement (below) shows this design
achieves with real margin, unlike v1 and v2.

## Per-family CCP allocation

One full channel costs two CCP modules: one for RX capture, one for TX
compare. All three mappings below were confirmed against each family's
own datasheet, not assumed from shared macro names.

| Family | Channel | RX pin (Capture) | TX pin (Compare) |
|---|---|---|---|
| PIC16F87XA | A (only channel) | RC2 / CCP1 | RC1 / CCP2 |
| PIC18Fxx5x | A (only channel) | RC2 / CCP1 | RC1 / CCP2 |
| PIC16F193X | A | RC2 / CCP1 | RC1 / CCP2 |
| PIC16F193X | B | RB5 / CCP3 | RD1 / CCP4 |

PIC16F87XA and PIC18Fxx5x have exactly two CCP modules total, both
spent on channel A: `EPIC_SWUART_MAX_CHANNELS` is 1. PIC16F193X has
five (CCP1-CCP5); channel A uses CCP1/CCP2 (the same pins as the other
two families), channel B uses CCP3/CCP4 (RB5/RD1, DS41364D's 40-pin
PDIP pin diagram for PIC16F1937), CCP5 is unused, and
`EPIC_SWUART_MAX_CHANNELS` is 2. Neither CCP3 nor CCP4 is
APFCON-remapped by `pic16f193x_ccp.c`, so the power-on-reset default
pin location applies to both. This is a real hardware ceiling
(`epic_swuart.h`'s `#if defined(PIC16F193X...)` family detection), not
a configurable value, and `EPIC_SWUART_Init` returns `EPIC_INVALID`
both for a pin combination that doesn't match a known slot and for a
slot that's already in use.

## RX: capture, then compare, then capture again

Idle: the channel's RX CCP module sits in `CCP_MODE_CAPTURE_FALLING`.
A start bit's falling edge latches Timer1's exact count into `CCPRx`
in hardware. The capture event handler (`rx_capture_event` in
`src/epic_swuart.c`) reads that latched edge time, computes
`edge_time + cycles_per_bit / 2` (the mid-start-bit deglitch confirm
point, the same half-bit-later timing v1 and v2 both already used),
writes it back into `CCPRx`, and switches the module to
`CCP_MODE_COMPARE_SOFT_IF` (compare-triggered interrupt only, no pin
action).

At that confirm match, if the RX pin is still low, the deadline
advances by one more full `cycles_per_bit`, landing at
`edge_time + 1.5 * cycles_per_bit`, d0's own center, before the first
real data-bit sample. Collapsing the confirm-then-sample sequence into
a single 1.5x hop (tried and rejected during design) would skip the
deglitch check entirely: at 1.5x post-edge the pin already reflects
d0's own value, not the start bit's stability, so a one-hop version
would silently stop rejecting noise starts. If the confirm sample
finds the pin has gone high again, the byte is abandoned as noise and
the module returns straight to `CCP_MODE_CAPTURE_FALLING`.

Each subsequent compare match samples the RX pin, shifts the bit into
`rx_shift` (LSB first), advances the deadline by one `cycles_per_bit`,
and rewrites `CCPRx`. After the stop bit is sampled (pushed to the RX
ring if high, dropped with `error_count` incremented if low), the
module switches back to `CCP_MODE_CAPTURE_FALLING` for the next byte.

Because every deadline programmed into `CCPRx` is an absolute Timer1
count, not a relative countdown, a handler that runs late does not
drift the schedule: `EPIC_CCP_SetCompare`/`SetMode` only need to land
before the *next* match's real time arrives, not exactly on time for
the one that just fired.

## TX: compare-driven bit generation, SET/CLEAR per bit

`EPIC_SWUART_Write` (on the transition from idle) computes the
absolute Timer1 deadline for the start bit
(`EPIC_TIMER1_ReadCounter() + SWUART_LEAD_CYCLES`, see below) and arms
the channel's TX CCP module with that deadline and
`CCP_MODE_COMPARE_CLEAR` (start bit is a space, i.e. low). Each
subsequent compare match (`tx_compare_event`) advances the deadline by
one `cycles_per_bit`, decides the *next* bit's mode
(`CCP_MODE_COMPARE_SET` if that bit is 1, `CCP_MODE_COMPARE_CLEAR` if
0), and reprograms `CCPxCON`/`CCPRx` before returning, so the mode
that fires at the deadline just written is always the one for the
transition that is actually due there. The stop bit is always `SET`
(mark); once its match fires the state machine lands back in
`TX_IDLE` and, if the TX ring is empty, sets the module to
`CCP_MODE_OFF` rather than leaving a stale compare armed.

**Why not toggle mode.** PIC18's CCP has a hardware
auto-toggle-on-match mode that PIC16 lacks, which looked like a
PIC18-specific advantage worth using. It was investigated and
rejected: toggle mode only produces the right waveform if consecutive
bit transitions strictly alternate level, which is true of a fixed
clock signal but not of arbitrary serial data. The first time two
consecutive bits share a value (extremely common: two 0 data bits in a
row, or a data bit matching the stop bit), a blind toggle would flip
the pin to the wrong level and desynchronize the rest of the frame.
Explicitly choosing SET or CLEAR per bit, based on the actual bit
value, is the only mode that's correct for arbitrary data, and it's
available identically on all three families, so there is no
per-family TX code path.

## Timer1: free-running, absolute deadlines

Timer1 free-runs continuously as the shared time base for every
channel's RX and TX; it is never reset or reprogrammed per-bit (a real
simplification from v2, which reprogrammed its one-shot timer on every
event). Every scheduling deadline stored in a handle
(`tx_deadline`/`rx_deadline`, both `uint16_t`) is an absolute Timer1
count, not a relative countdown, and wraparound is handled for free by
unsigned 16-bit subtraction/addition, not by explicit rollover logic.
`epic-swuart` still exclusively owns Timer1 for as long as any channel
is active, the same resource-ownership contract v1/v2 already
documented: do not also drive Timer1 directly from application code
while a channel is initialised.

## The `SWUART_LEAD_CYCLES` schedule-miss hazard

Arming a CCP compare deadline that has already passed (or is about to
pass before the write actually lands) is not a "slightly late" event
the way a software-scheduled miss would be: the hardware comparator
only matches Timer1's exact count once. If the deadline is already
behind Timer1 by the time `EPIC_CCP_SetCompare`/`SetMode` take effect,
the match is silently missed until Timer1 wraps all the way around
(65536 cycles, roughly 13 ms at 20 MHz) and reaches that count again,
by which point the frame is long since garbled.

`SWUART_LEAD_CYCLES` (`src/epic_swuart.c`) is the safety margin
`EPIC_SWUART_Write` adds to "now" when arming the very first deadline
of a new transmission, to guarantee the write lands before the
deadline it names. It is `120`, not the original design guess of `40`:
Task 2's real `mdb` probe
(`docs/superpowers/plans/probe-swuart-v3-ccp-cost.md`) measured 73-95
cycles of pure ISR dispatch latency alone on PIC16F877A (vector entry
to the first instruction of the event callback), before any of the
callback's own work. A 40-cycle margin would already be consumed by
dispatch latency alone, before `Write`'s own mainline code even runs;
120 leaves real headroom above the largest latency actually measured
on the same hardware/compiler.

The same probe reproduced a *related* hazard mechanism-level, at
`EPIC_CCP_Init` time rather than steady-state re-arm time:
`EPIC_CCP_Init` calls the table-driven `EPIC_IRQ_ClearFlag`/
`EPIC_IRQ_Enable` (150-250 cycles combined) *before* it writes
`CCPRxH`/`CCPRxL`/`CCPxCON`. Arming it with a compare value that is
already "near now" relative to a running Timer1 can miss the live
match before the module is even fully configured, falling back to the
same 65536-cycle wraparound recovery. This is why the TX CCP module is
initialised in `CCP_MODE_OFF` (see `EPIC_SWUART_Init`): at `Init` time
there is no live deadline to miss, since `CCP_MODE_OFF` drives no pin
action. The first real deadline is armed later, from
`EPIC_SWUART_Write`, past `EPIC_CCP_Init`'s one-time setup cost and
protected by `SWUART_LEAD_CYCLES`.

In-frame mode transitions (every bit boundary, both RX's
Capture-to-Compare/Compare-to-Capture switches and TX's SET/CLEAR
switches) go through `EPIC_CCP_SetMode`, a cheap mode-only register
write added to all three families' CCP drivers specifically for this
module, never the generic `EPIC_CCP_Init` (which would re-pay the
same table-driven IRQ bookkeeping cost on every single bit, the class
of overhead this whole redesign exists to remove). `EPIC_CCP_Init` is
only used once per channel, at `EPIC_SWUART_Init`/`DeInit`.

## TX idle-mark guarantee

The TX CCP module starts in `CCP_MODE_OFF` at `EPIC_SWUART_Init` (see
above), which means it does not drive the pin at all until the first
real bit's compare event fires. Left alone, the pin's level would then
depend on whatever the GPIO latch last held, which can be low, and a
low idle line is a break condition on a real wire (a receiver reads a
sustained low as more start bits, or as a device that has failed).
`EPIC_SWUART_Init` explicitly writes the TX pin high (`GPIO_PIN_SET`,
mark/idle) right after configuring it as an output, before the CCP
module is armed with anything. `EPIC_SWUART_DeInit` does the same
after tearing down the CCP module, unconditionally, regardless of what
state the TX state machine was in mid-frame, so a channel torn down
mid-transmission does not leave the wire held low either. This is a
deliberate, explicit design property (found missing and fixed during
this design's review), not an incidental side effect of GPIO
initialization order.

## Documented, not fixed: `EPIC_GPIO_WritePin` read-modify-write hazard (PIC16F87XA)

`EPIC_GPIO_WritePin` on PIC16F87XA reads the *physical* PORT register
(this family has no LAT register) and writes the merged byte back. Two
output pins sharing a PORT (for example, this module's own TX pin and
any other output on the same PORT) that transition close together in
real time risk one write reading the other's not-yet-risen voltage and
clobbering it, under real capacitive loading (long wires, level
shifters). MPLAB SIM cannot catch this: it models digital logic with
zero rise time. The CCP-hardware-timed TX transitions in this design
reduce how close together two *software*-driven writes on the same
port can land, but they do not eliminate the underlying RMW hazard for
whatever else shares that port. This hazard is not fixed by v3 (a
HAL-level shadow-register write path would be a separate, real
project); it is called out here so it is a known, documented risk
rather than a silent one for anyone wiring a TX pin onto a PORT shared
with other outputs on PIC16F87XA.

## Host testing

No host simulator for any of the three families models CCP capture or
compare hardware at all: the CCP driver's own `.c` file compiles
unchanged into the host build with zero simulated hardware behind it
(confirmed by reading the host build, not assumed). Tests therefore
cannot drive the state machine by injecting a simulated pin edge and
waiting for a simulated capture, the way, for example, `epic-serial`'s
host tests inject simulated USART RX. Instead, `src/epic_swuart.c`
exposes a set of test-only accessor functions
(`swuart_test_fire_tx_event[_b]`, `swuart_test_fire_rx_event[_b]`,
`swuart_test_set_capture`, `swuart_test_last_tx_mode[_b]`,
`swuart_test_last_tx_compare[_b]`), gated behind `EPIC_SWUART_TEST_HOOKS`,
that call the real `tx_compare_event`/`rx_capture_event` bodies
directly, as if a real CCP interrupt had just fired, letting tests
drive the real state machine logic without any real timing hardware
underneath it.

This is scoped to host-sim builds only: `EPIC_SWUART_TEST_HOOKS` is
defined by `epic-swuart/CMakeLists.txt`'s `target_compile_definitions`
on the `epic_swuart` CMake target, which only exists in the host-sim
CMake build. Real-target builds go through `scripts/epic_build.py`'s
manifest and the XC8 toolchain directly, never touch
`epic-swuart/CMakeLists.txt`, and therefore never define
`EPIC_SWUART_TEST_HOOKS`; the test-only functions are compiled out of
every real-target `.hex`.

This is a real, disclosed testing-methodology limitation, not a silent
one: host-sim passing tests verify the state machine's *logic* (bit
order, framing, error counting, ring behavior, dual-channel
independence) but say nothing about real CCP hardware timing, which is
exactly what the real-target verification below is for, and exactly
where that verification's own limits show up.

## Real-hardware verification: what is proven, and what is not

`epic-swuart` has a real `mdb` gate
(`epic-swuart/tests/sim_target_swuart.c`, wired into
`scripts/ci-target-sim.sh`), the first real-hardware behavioral
verification this module has ever had, on PIC16F877A. What it actually
proves:

- Real, compiled CCP2-compare-driven TX firmware runs under MPLAB SIM
  to completion without hanging or crashing (the gate writes one byte
  and confirms `h.tx_count` reaches 0).
- A separate, manual hand-trace of the real compiled `.hex`'s `PORTC`
  register at all 10 real CCP2 compare events for that byte confirmed
  the TX bit sequence is byte-exact hardware-correct.

**It does not prove real-hardware RX correctness.** Two different
full TX+RX loopback approaches were attempted for this gate and both
hit real, unresolved obstacles (see
`epic-swuart/tests/sim_target_swuart.c`'s header comment and
`docs/superpowers/plans/2026-08-07-swuart-v3.md`'s Task 8 section for
the full write-up): an MPLAB SIM SCL stimulus process driving RC2 from
RC1 never registered a CCP1 capture at all, and a breakpoint-driven
`write pin RC2 <level>` approach (matching each TX transition observed
on RC1) did make CCP1 capture real edges and run the whole
confirm/sample/stop chain with `error_count` staying 0, but the byte
that came out did not match what went in, and not by any pattern that
pointed at a simple off-by-one. The remaining timing gap was not
chased further within this task's budget. The shipped gate is
therefore TX-only, and this limitation is stated here plainly rather
than glossed over: **the module's real-hardware verification confirms
TX is byte-exact but leaves RX correctness on real silicon an open
question**, exactly the same honesty standard the rest of this
redesign has held to.

## Measured margins (Task 2 probe, PIC16F877A, 20 MHz, 9600 baud)

One bit period at 9600 baud / 20 MHz is 521 instruction cycles.
`docs/superpowers/plans/probe-swuart-v3-ccp-cost.md` measured the real,
compiled, linked driver code (not a mirror) under `mdb`:

| Event | Measured cycles | % of 521-cycle budget | Margin |
|---|---|---|---|
| CCP2 compare (TX bit transition) | 288 | 55% | 233 cycles |
| CCP1 capture (RX start-bit edge, real pin injection) | 404 | 78% | 117 cycles |

Both fit with real margin, a completely different outcome from v2's
own real measurement of 1019 cycles against the same 521-cycle budget
(196% over budget). Task 1's fix (a direct `PIR1`/`PIR2` bit clear in
`CCP1_IRQHandler`/`CCP2_IRQHandler` instead of the table-driven
`EPIC_IRQ_GetFlag`/`ClearFlag` the shared dispatcher had already made
redundant) measured at just 30 cycles of handler-level overhead and is
a meaningful share of why these numbers fit.
