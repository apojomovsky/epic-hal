# `epic-tick` API reference

Authoritative declarations: [`include/epic_tick.h`](../include/epic_tick.h).

### `void epic_tick_init(uint32_t fosc_hz)`
Start the 1 ms timebase. Computes the Timer2 PR2/prescaler/postscaler closest
to 1 ms from `fosc_hz` (exact for common Fosc: 4/8/16/20/32/48 MHz), installs
the tick ISR via the Timer2 `OverflowCallback`, and enables the timer. Call
once at startup. The Timer2 ISR increments the internal counter in the
background from then on.

### `uint32_t epic_tick_get(void)`
Milliseconds since `epic_tick_init`. Monotonic; wraps every ~49.7 days
(2³² ms). The 32-bit read is atomic against the ISR (interrupts disabled
around the read), so no torn read.

### `void epic_tick_delay_ms(uint32_t ms)`
Block for `ms` milliseconds. Guarantees at least `ms` (may overshoot by up to
~1 tick). On the host sim it pumps `epic_harness_tick()` so simulated time
advances; on a real target it spins while the Timer2 ISR advances the counter.

### `uint32_t epic_tick_elapsed_since(uint32_t t0)`
`epic_tick_get() - t0`, the non-blocking idiom
`if (epic_tick_elapsed_since(t0) >= timeout)` instead of blocking.
Wraparound-safe (unsigned subtraction), so it stays correct across the
~49.7-day roll.

## Usage

```c
epic_tick_init(FOSC_HZ);            /* once */

epic_tick_delay_ms(100);            /* blocking 100 ms */

/* non-blocking timeout */
uint32_t t0 = epic_tick_get();
do_something_chunk();
if (epic_tick_elapsed_since(t0) >= 50u) { /* took longer than 50 ms */ }
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `epic_tick_init` | start the 1 ms Timer2 timebase |
| `epic_tick_get` | monotonic ms count (atomic read) |
| `epic_tick_delay_ms` | blocking delay in ms |
| `epic_tick_elapsed_since` | non-blocking ms-since-t0 |
