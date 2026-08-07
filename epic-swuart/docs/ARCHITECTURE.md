# epic-swuart architecture

## The shared tick

One Timer1, one ISR, one small registry of up to
`EPIC_SWUART_MAX_CHANNELS` active handles. Every tick, `shared_tick()`
rewrites Timer1's counter (Timer1 has no period register, unlike
Timer2's peripheral-driven reload: it is a free-running 16-bit counter,
so the ISR must rewrite `TMR1H:TMR1L` on every overflow or the period
degrades to a full 65536-count wraparound after the first tick), then
services every active handle's `tx_step`/`rx_step`.

For the default (and required) two-channel case, `shared_tick()` is
straight-line code, not a loop indexed by a runtime variable: two
explicit `if (g_channels[N] && g_channels[N]->active) { tx_step(...);
rx_step(...); }` blocks, one per slot, calling the same functions a
loop would rather than duplicating their bodies. A probe
(`docs/superpowers/plans/probe-swuart-isr-budget.md`) measured that a
runtime-indexed loop over the channel registry compiles to a real
`goto`-based loop under XC8 at `-O2`, real overhead even for a
two-element array. `EPIC_SWUART_MAX_CHANNELS` stays a configurable
compile-time value; anything other than 2 falls back to the loop
shape, so behavior for a legal override is unchanged, just not on the
measured-fast path.

Ring-buffer index math (`tx_head`/`tx_tail`/`rx_head`/`rx_tail`) masks
with `(EPIC_SWUART_RING_SZ - 1u)` rather than using `%`, guarded by a
compile-time check (`_Static_assert` in `epic_swuart.h`) that
`EPIC_SWUART_RING_SZ` is a power of two.

The tick rate is `baud * N`. `docs/superpowers/plans/probe-swuart-isr-budget.md`
measured N=4 as technically reachable on the worst case (PIC16F87XA at
20 MHz), but only a 6.2% cycle margin on a minimal snapshot with no
ring-buffer or parameter-passing overhead; N=3 (29.9% margin) is the
production default. At 9600 baud and N=3: a tick every 34.72
microseconds.

**Known gap, not closed by this pass:** a real-target `mdb` measurement
(`docs/superpowers/sdd/2026-08-06-swuart/final-fix-wave-report.md`)
found the real single-channel worst-case ISR, from interrupt vector
entry through returning to the interrupted code, costs roughly 560-680
cycles depending on the shared-dispatch shape, against this 174-cycle
N=3 budget, still well over budget even after the grouped-read
dispatch fix (`pic16_irq_dispatch.c` et al.) and this straight-line
`shared_tick()`/ring-mask combination. Do not treat N=3 as verified to
fit on real hardware; that report's measurements are the current
authoritative numbers. That same real-target measurement also hit a
`mdb` symptom (`GIE` never observed set again after the first
interrupt, `run`+`wait` not reliably stopping at breakpoints) matching
this repo's own prior, unrelated investigation into a non-reentrant
storage-overlap/bank-misdirection bug class
(`pic16f87xa-hal/docs/ARCHITECTURE.md` Findings 4-9); whether the same
mechanism is at play here was not confirmed, see the report for what
was and wasn't ruled out.

## Why Timer1, not an edge interrupt

See `docs/superpowers/specs/2026-08-06-swuart-design.md` for the full
comparison against pin-change-interrupt start-bit detection and
busy-wait bit-banging. Short version: a fixed tick works on any GPIO
pin (PIC16F87XA/PIC18Fxx5x's port-change interrupt only covers
RB4:RB7), and generalises to N concurrent channels without juggling one
timer's one-shot reloads across independent, unsynchronized schedules.

## RX: confirming a start bit, not just detecting one

`RX_IDLE` samples the pin every tick. A high-to-low read looks like a
start bit, but could be noise, so `RX_CONFIRM_START` waits half a bit
period and re-samples before committing to the bit-timing grid. Only
after that confirmation does the state machine schedule the eight data
bits and the stop bit at one full bit period apart. A bad stop bit
(sampled low instead of high) means the byte is dropped and
`error_count` increments; the state machine returns to `RX_IDLE`
regardless, so one bad byte does not stall subsequent ones.

**Precondition:** the RX line must be idle-high (pulled up, or connected
to a live transmitter) whenever the channel is not actively receiving; a
floating or held-low RX pin will be misread as a continuous stream of
start bits.

## Zero per-family branching

Every call `epic_swuart.c` makes (`EPIC_GPIO_Init/ReadPin/WritePin`,
`EPIC_TIMER1_Init/Start/WriteCounter`, `EPIC_IRQ_Restore`) already has an
identical signature across PIC16F87XA, PIC18Fxx5x, and PIC16F193X. The
only per-family code in this module lives in the test files, which pick
the right `*_sim_drive_input`/`*_sim_read_output` function by name (the
host simulation backend, not the module itself, differs per family).

## Host testing

No real timer or GPIO hardware exists on the host. Tests call
`epic_harness_tick()` in a loop to advance simulated instruction cycles;
this drives the real, simulated Timer1 peripheral (not a hand-rolled
substitute), which asserts its overflow flag and fires
`shared_tick()` through the same `epic_dispatch_all_irqs` path a real
interrupt vector would use. TX is verified by reading the simulated
output pin (`*_sim_read_output`) at each bit's mid-point; RX is verified
by driving the simulated input pin (`*_sim_drive_input`) at the correct
tick offsets and checking the decoded byte through `EPIC_SWUART_Read`.
