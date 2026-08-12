# `epic-encoder` API reference

Per-function semantics, the port-byte/bit-position wiring convention, and
the hard constraints (`at most two encoders per port`, `PORTB only`, the
glitch-gate's 1 ms resolution). The design rationale lives in the
module's [README](../README.md).

## Headers and linkage

```c
#include "encoder.h"      /* public API, dependency-free (only <stdint.h>) */
```

`encoder.h` is the only header a consumer includes. It is dependency-free
(only `<stdint.h>`), matching `epic-debounce`'s header/.c split: the
`epic_tick.h` timebase and the family-neutral `core/hal_irq.h` are included
only by `src/encoder.c`, so a unit including `encoder.h` does not drag in
the HAL or the timebase.

Build (host): `cmake -B build && cmake --build build` (default PIC16;
`-DEPIC_FAMILY=PIC18` selects the PIC18 family). The CMake build links
`epic-tick` (the glitch-gate timebase) and the chosen HAL family
(`epic_encoder_get_position`'s atomic read is the one HAL call this module
makes), and `epic-pid` for the `example_encoder_pid_loop` example. Real
targets use the XC8 Makefiles under `mcu/`.

## The wiring convention: one port byte, two bit positions

An `epic_encoder_t` does not hold a port or a HAL pin handle, it holds two **bit
positions** (0-7) within the port byte the RB-change callback receives.
The HAL's `RB_IRQHandler` reads PORTB into a byte and forwards it to the
callback registered via `EPIC_GPIO_RegisterChangeCallback`; the application
callback fans that one byte out to every `epic_encoder_t` it owns by calling
`epic_encoder_update(&enc, portb)`, and `epic_encoder_update` extracts this
instance's 2-bit state as `((portb >> pin_a) & 1) << 1 | ((portb >> pin_b) & 1)`.

Wire channel A and B to two of RB4-RB7 (the RB<7:4> interrupt-on-change
pins) and pass those bit positions to `epic_encoder_init`. For two encoders
sharing the port, use disjoint pin pairs (e.g. A on RB4/RB5, B on RB6/RB7).

```c
/* Application-side wiring (see examples/example_encoder_hal.c). */
static void on_rb_change(uint8_t portb) {
    epic_encoder_update(&enc_a, portb);   /* pins 4,5 */
    epic_encoder_update(&enc_b, portb);   /* pins 6,7 */
}
EPIC_GPIO_RegisterChangeCallback(on_rb_change);
EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
              GPIO_MODE_INPUT);
uint8_t start = EPIC_GPIO_ReadPort(GPIOB);
epic_encoder_init(&enc_a, 4, 5, 0, start);
epic_encoder_init(&enc_b, 6, 7, 0, start);
/* On a real target also: EPIC_IRQ_Enable(<family>_IRQ_RB); EPIC_IRQ_Restore(1); */
```

## Types

```c
typedef struct {
    uint8_t           pin_a;                /* bit position 0-7 of channel A */
    uint8_t           pin_b;                /* bit position 0-7 of channel B */
    volatile int32_t  position;             /* x4 count; written only from ISR context */
    uint8_t           last_state;           /* 2-bit (a<<1|b), previous sample */
    uint16_t          min_edge_interval_ms; /* 0 = glitch gate disabled */
    uint32_t          last_edge_tick;       /* epic_tick_get() at last ACCEPTED edge;
                                               meaningless when the gate is disabled */
    volatile uint16_t error_count;          /* impossible Gray transitions */
    volatile uint16_t glitch_count;         /* edges rejected by the gate */
} epic_encoder_t;
```

`epic_encoder_t` is 17 B on-target (XC8 tight packing) on both PIC16 and PIC18.
Caller-owned storage, one per A/B pair.

## Functions

### `epic_encoder_init`

```c
void epic_encoder_init(epic_encoder_t *enc, uint8_t pin_a, uint8_t pin_b,
                  uint16_t min_edge_interval_ms, uint8_t port_value);
```

Stores the pin positions and the gate interval, zeroes `position`,
`error_count`, `glitch_count`, seeds `last_edge_tick = epic_tick_get()`,
and seeds `last_state` from the given `port_value` (the same byte the
RB-change callback would receive, or a one-off manual read at boot before
interrupts are enabled). Seeding `last_state` from the real port is what
keeps the very first real edge from being misjudged against a state the
hardware was never in (or spuriously counted as an impossible transition).

`epic_tick` must be initialized (`epic_tick_init`) before `epic_encoder_init`,
because `epic_encoder_init` calls `epic_tick_get()` to seed `last_edge_tick`
(even when the gate is disabled, the field is seeded; it is simply unused
until the gate is armed).

**Direction:** the count sign for a given physical rotation is fixed by
the shipped Gray-code `QUAD_TABLE`. If your wiring
produces the opposite sign (turns clockwise but the count goes down),
**swap `pin_a` and `pin_b`** in this call to invert direction. Do not add a
`direction_invert` flag, swapping two constructor arguments already
solves it.

### `epic_encoder_reset`

```c
void epic_encoder_reset(epic_encoder_t *enc, uint8_t port_value);
```

Re-syncs `last_state` from `port_value` and zeroes `position`,
`error_count`, `glitch_count`, and `last_edge_tick`. Gains
(`pin_a` / `pin_b` / `min_edge_interval_ms`) are unchanged. For recovering
after a fault (e.g. a missed-edge storm) without re-wiring the instance.
The next `epic_encoder_update` for the next real edge is then decoded against
the freshly-read state, not spuriously as an impossible transition.

### `epic_encoder_update`

```c
void epic_encoder_update(epic_encoder_t *enc, uint8_t port_value);
```

Call from the application's `EPIC_GPIO_RegisterChangeCallback` handler,
once per registered instance, passing the same received port byte. It
extracts this instance's 2-bit state, no-ops if it equals `last_state`
(another instance's pins changed on the same byte, not an error), then
(if the gate is armed) drops a too-soon edge as a glitch without
advancing `last_state`, else looks up the Gray-code step and applies it.
An impossible transition (both bits flipped at once, `QUAD_TABLE` returns
0 for a genuine state change) increments `error_count` and leaves
`position` unchanged (the step is 0), but still advances `last_state` so
the decoder resyncs to the new sample.

### `epic_encoder_get_position`

```c
int32_t epic_encoder_get_position(const epic_encoder_t *enc);
```

The accumulated x4 quadrature count, read atomically (interrupts disabled
around the 32-bit read so an ISR update cannot tear it mid-read on an
8-bit core). Mirrors `epic_tick_get()`'s exact pattern.

### `epic_encoder_get_error_count` / `epic_encoder_get_glitch_count`

```c
uint16_t epic_encoder_get_error_count(const epic_encoder_t *enc);
uint16_t epic_encoder_get_glitch_count(const epic_encoder_t *enc);
```

The two diagnostic counters, each read atomically (a 16-bit read has the
same tear risk in principle on an 8-bit core, wrapped the same way for
consistency even though a torn diagnostic counter is low-stakes). A
climbing `error_count` means the software isn't keeping up or samples are
being corrupted; a climbing `glitch_count` means the gate is doing its job
filtering real mechanical bounce.

## Hard constraints

### At most two encoders per port, PORTB only

RB<7:4> is 4 pins; one A/B pair uses 2, so at most two independent
`epic_encoder_t` instances can share one port's IOC. This is a real hardware
ceiling on both families: mismatch-style interrupt-on-change exists **only**
on PORTB (confirmed via the IRQ enums, `PIC16_IRQ_RB` / `PIC18_IRQ_RB` are
the only IOC sources; nothing analogous for PORTA/C/D). A third encoder,
or one on another port, is not possible on this hardware. This is a hard
constraint, not a tunable, and not a roadmap item.

### The glitch gate's 1 ms resolution

`min_edge_interval_ms` is measured on `epic-tick`'s 1 ms timebase. That
is coarse relative to true electrical bounce timescales (sub-ms) but
adequate for mechanical detent bounce, which typically resolves within a
few ms. Do not treat the gate as a precision sub-ms filter; if a concrete
encoder's bounce characteristics demand sub-millisecond filtering, that
that needs a free-running Timer1 microsecond capture, a separate piece of work.
The gate is per-instance and
optional (`min_edge_interval_ms == 0` disables it), so a clean Hall-effect
channel is not held to a timing gate a bouncy mechanical one needs.

## What composes at the call site (and so is not in this module)

- **Velocity/RPM**: two `epic_encoder_get_position()` reads across a known
  `epic_tick_elapsed_since()` interval, exactly like `epic-adcfilter`
  composes with anything wanting filtered ADC. Nothing about it belongs
  inside `epic_encoder_update`.
- **A PID loop**: feed `epic_encoder_get_position()` straight into
  `epic_pid_update()`'s `measurement` each control cycle
  (`examples/example_encoder_pid_loop.c`).
