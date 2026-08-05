# `epic-tick` architecture

A 1 ms timebase, the STM32Cube `HAL_GetTick`/`HAL_Delay` equivalent for
8-bit PICs, built on the HAL's Timer2.

## What it is

`epic-tick` gives firmware a monotonic millisecond counter (`epic_tick_get`),
a blocking delay (`epic_tick_delay_ms`), and a non-blocking elapsed-time
helper (`epic_tick_elapsed_since`). It is the single most-used Cube utility,
and the foundation other timed modules (timeouts, debouncing, serial flush
deadlines) stand on. It works on every PIC this repo supports, PIC16F87XA
and PIC18F2455/2550/4455/4550, and on the host simulator, from one
family-agnostic source file.

## How the tick is produced

Timer2 is auto-reload (PR2): on a period match it raises TMR2IF, the HAL's
`TIMER2_IRQHandler` clears the flag and calls the handle's `OverflowCallback`,
which is `epic_tick_on_overflow` here, it increments a `volatile uint32`
millisecond counter. The module installs the callback through the Timer2
handle (`OverflowCallback` field set before `EPIC_TIMER2_Init`), exactly the
pattern `epic-taskmgr` uses for its Timer0 tick. It does **not** redefine
`TIMER2_IRQHandler`, the HAL driver owns that handler.

The handle is `static` because the PIC16 Timer2 driver stores the caller's
pointer (not a copy), so the handle must outlive the ISR; the PIC18 driver
copies the handle, so static is safe there too.

## Period math

TMR2IF fires every `prescaler × (PR2+1) × postscaler` instruction cycles; one
instruction cycle = Fosc/4. For a 1 ms tick we need `Fosc/4000` instruction
cycles. `compute_period()` searches prescaler {1,4,16} × postscaler {1..16} ×
(PR2+1) {1..256} for the configuration closest to the target. For the common
Fosc values it is exact:

| Fosc | prescaler | PR2 | postscaler | product |
|---|---|---|---|---|
| 20 MHz | 1:4 | 249 | 1:5 | 4·250·5 = 5000 |
| 48 MHz | 1:16 | 249 | 1:3 | 16·250·3 = 12000 |

## Atomicity and the host/target execution models

`epic_tick_get()` disables interrupts around the 32-bit read (an 8-bit core
reads it in four bytes; the ISR could update it mid-read). On the host sim
the ISR fires synchronously inside `epic_harness_tick()`, so the
disable/restore is harmless there.

`epic_tick_delay_ms()` pumps `epic_harness_tick()` while it waits, so on the
**host** simulated time advances (the sim only advances when pumped; one tick
= `prescaler×(PR2+1)×postscaler` sim steps, e.g. 5000 at 20 MHz). On a **real
target** `epic_harness_tick()` is a no-op and the Timer2 ISR advances the
counter in real time while the call spins. The delay guarantees at least the
requested milliseconds (overshoot ≤ ~1 tick).

## Why Timer2 and not Timer0

Timer2 is auto-reload (PR2), so the ISR need only increment the counter, no
manual reload write. Timer0 has no auto-reload (the task manager reloads it in
its own ISR), so Timer2 is the cleaner choice for a standalone timebase.
`epic-taskmgr` keeps Timer0; the two coexist on their own timers.
