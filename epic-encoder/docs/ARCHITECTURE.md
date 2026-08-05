# `epic-encoder` architecture

The design of `epic-encoder`: a vendor-agnostic, interrupt-driven x4
incremental quadrature decoder, instantiable (one `encoder_t` per A/B
channel pair), composable with `epic-pid`. Why a software decoder at all,
why push (ISR-driven) not poll, why a Gray-code transition table, why two
separate error counters, why one whole-port callback in the HAL rather
than a per-instance registry, and the read-before-clear ordering the
RB-change handler depends on. Full implementation plan:
`docs/epic-encoder-plan.md` (the source of the design discussion; this
document is the post-implementation write-up, including which RBIF
sim-modeling approach actually landed and the measured footprint).

## What it is and why it has to be software

A decoder for a standard 2-channel (A/B) incremental quadrature encoder,
x4 resolution (counts every edge of both channels). Composable with
`epic-pid` the way the whole module was motivated: feed
`encoder_get_position()` into `pid_update()`'s `measurement` argument
every control cycle (see `examples/example_encoder_pid_loop.c` for the
direct demonstration).

Neither PIC16F87XA nor PIC18F2455/2550/4455/4550 has any encoder-aware
peripheral (no QEI mode; confirmed by grepping both families'
`peripherals/*_ccp.h`, whose capture-mode options are edge/4th/16th
capture into a timer, nothing quadrature-aware). So the decode is
software, interrupt-driven, using the one thing both families do have:
the RB<7:4> interrupt-on-change (IOC) source. Wire encoder channel A and
B to two of RB4-RB7; any edge on either fires one shared interrupt;
decode with a software Gray-code state-transition table.

That IOC source was a real HAL gap: unlike Timer0/Timer1/Timer2 (which
every family already exposed a weak `*_IRQHandler` + callback for),
neither HAL had any hook for the RB-change interrupt at all, no weak
handler, no callback, and it was not in either family's
`epic_dispatch_all_irqs` fan-out. Phase 0a added exactly that missing
piece to both HALs (mirroring the existing Timer2 pattern, not a new
convention) before this module could be built on it. See the addendum
each HAL's `MANUAL.md` now has under its GPIO section.

## Why push (ISR callback), not poll

Unlike `epic-debounce`'s `debounce_poll()` (called once per scheduler
tick), a quadrature signal can transition between poll calls faster than
any reasonable poll period, a poll-driven design would just miss
counts. This is edge-triggered: the application registers one function as
the HAL's RB-change callback (`EPIC_GPIO_RegisterChangeCallback`), that
function forwards the received PORTB byte to `encoder_update()` for each
`encoder_t` instance it owns. Fanning one byte out to N instances is
application-level composition, not a HAL or encoder registry, exactly the
"compose by staying decoupled" reasoning `epic-fsm` and `epic-taskmgr`
already use. See `examples/example_encoder_hal.c` for two instances
sharing one port wired end to end.

## Why a Gray-code transition table, and why it needs no `epic-math`

The standard, widely-used technique for quadrature decode with no
dedicated hardware: track a 2-bit state `(A<<1)|B`, look up
`QUAD_TABLE[(prev_state<<2)|new_state]` for a `{-1, 0, +1}` step. With
state ordering 00,01,11,10 (true Gray code, adjacent states differ by one
bit) the table is:

```c
static const int8_t QUAD_TABLE[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0,
};
```

This is a well-established reference table (used in numerous open-source
rotary-encoder libraries), not novel work, and per this repo's own
convention of not trusting an unverified table blindly it is checked by a
throwaway probe and by Phase 1's "known-good full rotation" test case
(walks a full rotation both directions, asserts the total and
`error_count == 0`). Only integer add/compare/shift are needed, no
multiply, so unlike `epic-pid` this module does **not** link `epic-math`.

### The sign convention, and a plan inconsistency this module resolved

The plan's prose labeled the physical direction `00->01->11->10->00`
"forward, +8 over two cycles". The verbatim table it also specifies makes
that direction **-8**: the table counts `00->10->11->01->00` positive
(+4 per electrical cycle) and `00->01->11->10->00` negative. Verified
before the test was written by a throwaway probe (the repo's "probe,
don't assume" convention, the same discipline `epic-math`'s XC8
round-trip probe is the canonical example of):

```
00->01->11->10 (plan calls this 'forward'): end pos=-8 errors=0
00->10->11->01 (opposite direction):        end pos= 8 errors=0
```

The table is the non-redesignable shipped artifact ("implement exactly
this", "do not redesign"), so the test asserts the table's actual output
and documents the sign as a wiring convention: which physical rotation
is "forward" is fixed by the table, and invertible by swapping `pin_a`
and `pin_b` at `encoder_init` (see `docs/API.md`). No `direction_invert`
flag, swapping two constructor arguments already solves it. What the
rotation test cases actually prove is the known-good-against-the-table
property the plan requires: a full rotation is exactly +/-4 per
electrical cycle, the opposite direction gives the opposite sign, and
`error_count` stays 0 (every step is a valid single-bit Gray transition).

## Why two error counters, not one

Three distinct failure modes, two distinct counters:

- **Impossible transitions** (`QUAD_TABLE` returns 0 for a genuine state
  change, i.e. both bits appeared to flip between samples) usually mean a
  missed edge or bit-level noise corrupted a sample. Tracked in
  `error_count`, always on, free (the table lookup already happens).
  `encoder_update` still advances `last_state` to the new sample so the
  decoder resyncs rather than getting stuck against the old one; position
  is unchanged (delta == 0).
- **Bounce/glitch** (a real, valid-looking transition arriving
  suspiciously soon after the last accepted one) is a mechanical-contact
  problem, not a decode-correctness problem. Tracked separately in
  `glitch_count`, only active when `min_edge_interval_ms != 0` for that
  instance. The gate intentionally does **not** advance `last_state` on
  rejection, so the next sample compares against the last *accepted*
  state, not the rejected one.

Keeping these as two counters (not one) is deliberate: a climbing
`error_count` means "the software isn't keeping up or something's
electrically corrupting samples", a climbing `glitch_count` means "the
gate is doing its job filtering real mechanical bounce". Conflating them
would destroy the diagnostic value that was the actual point of adding
either counter.

### Glitch-gate resolution, and why it is per-instance and optional

`epic-tick`'s 1 ms timebase is coarse relative to true electrical bounce
timescales but adequate for mechanical detent bounce (typically resolves
within a few ms). That resolution limit is documented plainly in
`docs/API.md`, not silently oversold. The gate is per-instance and
optional (`min_edge_interval_ms == 0` disables it), not a global config,
so a clean Hall-effect channel on the same firmware isn't held to a
timing gate a bouncy mechanical one next to it needs. Hall-effect
encoders generally don't bounce (no physical contact) but can suffer
electrical noise from nearby motor windings/PWM, and unlike STM32's
hardware QEI this software approach can also simply miss edges if they
arrive faster than the ISR can service them, the `error_count` case
above.

## Why one whole-port callback in the HAL, not a per-instance registry

`epic-encoder` wants to run more than one encoder off the same RB<7:4>
interrupt (at most two per port, see below). The HAL layer does **not**
grow a multi-subscriber registry for this, one callback slot is enough,
exactly matching Timer2's one-`OverflowCallback`-per-handle shape but
simpler (there is only ever one PORTB, so no handle struct). Fanning one
received byte out to N `encoder_t` instances is application-level
composition (a static array and a `for` loop, demonstrated in
`example_encoder_hal.c`), the same "compose by staying decoupled"
reasoning the rest of this repo uses. Do not build a registry into the
HAL or into `epic-encoder` itself to save the application five lines.

## The read-before-clear ordering the RB-change handler depends on

DS39582B §14.11.3 / DS39632E's equivalent RB-change section both document
that clearing RBIF **before** reading PORTB is wrong: the mismatch
comparator latches "the value of PORTB at the last read", so reading
PORTB is what ends the current mismatch condition and re-arms detection
of the next one; clearing the flag first (or not reading at all) risks
an immediate spurious re-interrupt or, worse, silently missing a change
that lands in the gap. So `RB_IRQHandler` does, in this exact order: read
`PORTB` into a local, clear RBIF, then call the callback with that
already-read value, never letting the callback (or anything else) do a
second, later read of PORTB (by then it may no longer reflect the byte
the mismatch logic actually cleared against). The read/clear site has a
comment saying so plainly, since that ordering is what a future reader
could otherwise "simplify" into a bug.

## The RBIF host-sim model: what landed and why

The plan left two options for testing `RB_IRQHandler` on the host sim,
where the sim does not auto-assert RBIF on a PORTB mismatch:

1. Faithfully model the datasheet mismatch comparator (RBIF asserts when
   a pin within RB<7:4> configured as input changes value relative to
   the PORTB value at the last CPU read of PORTB), hooking into wherever
   the sim intercepts SFR reads.
2. A documented test-only fallback: poke the RBIF bit directly in the
   test, then call `RB_IRQHandler()` / `epic_dispatch_all_irqs()` and
   assert on the callback's observed argument and the post-call flag
   state.

**Option 2 landed.** The faithful model would require intercepting
every CPU read of PORTB, but on the host every SFR access is the
`PIC8_REG8(addr)` macro indexing the memory-backed register file
(`pic8f87xa_sim_sfr[]`) directly, there is no function to hook. Modeling
"snapshot on every PORTB read" would mean changing the platform header
(`include/host/pic16f87xa_platform.h`, the macro every SFR access goes
through) to route PORTB reads through a function, for both families, a
change that touches all SFR access for the sake of the one feature that
needs it. That is genuinely disproportionate, so the documented fallback
is used: `example_rb_change.c` in each HAL's own example suite asserts
RBIF directly in INTCON, then calls the handler and checks the
callback's observed byte and the post-call flag state.

This proves the handler's own read/clear/callback ordering, which is the
part that actually matters here (the part that could be "simplified" into
a bug). It does not prove the sim's mismatch behavior, because the sim
does not model it, but that is the right trade: the sim's mismatch model
is not what ships in firmware, the handler's read-before-clear order is.
If a real-target mismatch-behavior test is ever needed, that belongs in
an on-target test, not in the host sim.

## At most two encoders per port, PORTB only

RB<7:4> is 4 pins; one A/B pair uses 2, so at most two independent
`encoder_t` instances can share one port's IOC. This is a real hardware
ceiling on these two families: mismatch-style IOC exists **only** on
PORTB on both (confirmed via the IRQ enums, `PIC16_IRQ_RB` /
`PIC18_IRQ_RB` are the only IOC sources listed, nothing analogous for
PORTA/C/D). Stated plainly in `docs/API.md` as a hard constraint, not a
tunable, and listed under the explicitly-out-of-scope future work (a
third encoder or one on another port is not possible on this hardware).

## What `encoder_update` is pure and what needs the HAL

`encoder_update()` itself is pure C, no HAL dependency, same as
`fsm_dispatch()`. But `encoder_get_position()` reads a `volatile
int32_t` that the ISR writes asynchronously to the caller's mainline
code, exactly `epic_tick_get()`'s situation (a 4-byte read on an 8-bit
core that an ISR can tear mid-read), and the fix is the same: wrap the
read in `EPIC_IRQ_Disable()` / `EPIC_IRQ_Restore()` (the family-neutral
`core/hal_irq.h` signature). That one call is the entire HAL surface
this module touches. The 16-bit diagnostic counters are wrapped the same
way for consistency even though a torn counter is low-stakes. Only
`src/encoder.c` includes `core/hal_irq.h` and `epic_tick.h` (the glitch
gate's timebase); the public `encoder.h` stays dependency-free (only
`<stdint.h>`), matching `epic-debounce`'s header/.c split.

## Footprint

Cross-compiled cleanly with MPLAB XC8 v3.10 -O2 for every PIC16 device
(873A/874A/876A/877A) and the PIC18 SPP devices (4455/4550). `encoder_t`
is 17 B per instance on both families (tight XC8 packing:
1+1+4+1+2+4+2+2), within the plan's 16-18 B estimate and the same order
as `epic-pid`'s 21-byte `pid_t`:

| Target      | Program space                | Data space           |
|-------------|------------------------------|----------------------|
| PIC16F877A  | 3005 words (36.7% of 8 KW)   | 153 B (41.6% of 368 B) |
| PIC16F873A  | 2675 words (65.3% of 4 KW)   | 146 B (76.0% of 192 B) |
| PIC18F4550  | 4960 B    (15.1% of 32 KB)   | 290 B (14.2% of 2 KB)  |

The per-instance `encoder_t` cost is 17 B; the rest of data space is
`epic-tick`'s timebase + HAL state. (The PIC18 2455/2550 do not build, for
a pre-existing `pic18fxx5x-hal` reason unrelated to this module:
`pic18fxx5x_spp.c` is compiled unconditionally but references SPP symbols
only defined under `PIC18FXX5X_FAMILY_HAS_SPP` (4455/4550).
`epic-tick`'s 18F2455 build fails identically, confirming it is not a
regression from this work.)

## What is deliberately out of scope

- **Velocity/RPM** composes at the call site (two `encoder_get_position()`
  reads across a known `epic-tick` interval), exactly like `epic-adcfilter`
  composes with anything wanting filtered ADC. Nothing about it belongs
  inside `encoder_update`.
- **Z/index-pulse homing** is a different problem (single-edge-per-revolution
  capture, not quadrature) and would use a spare INTx pin on PIC18, not
  RB<7:4>. Separate module if a concrete application needs it.
- **A third encoder, or one on PORTA/C/D** is not possible on this hardware
  (mismatch-style IOC is PORTB-only on both families). A hard ceiling, not
  a roadmap item.
- **Sub-millisecond glitch filtering** would need a free-running Timer1
  microsecond capture instead of `epic-tick`'s 1 ms; meaningfully more
  complexity, not built until a concrete encoder's bounce actually demands it.
