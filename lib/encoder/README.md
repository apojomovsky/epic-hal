# epic-encoder, interrupt-driven x4 quadrature decoder for 8-bit PICs

A vendor-agnostic decoder for a standard 2-channel (A/B) incremental
quadrature encoder, x4 resolution (counts every edge of both channels),
instantiable (one `epic_encoder_t` per A/B pair), composable with `epic-pid`:
feed `epic_encoder_get_position()` into `epic_pid_update()`'s `measurement`
argument every control cycle.

Neither PIC16F87XA nor PIC18F2455/2550/4455/4550 has any encoder-aware
peripheral (no QEI mode), so the decode is software, interrupt-driven,
using the one thing both families do have: the RB<7:4> interrupt-on-change
source. Wire channel A and B to two of RB4-RB7; any edge on either fires
one shared interrupt; decode with a software Gray-code transition table.

- **One family-agnostic API** (`epic_encoder_init` / `epic_encoder_reset` /
  `epic_encoder_update` / `epic_encoder_get_position` / `epic_encoder_get_error_count` /
  `epic_encoder_get_glitch_count`), same `src/encoder.c` builds against
  `pic16f87xa-hal` or `pic18fxx5x-hal` (`-DEPIC_FAMILY=PIC18`).
- **Push, not poll**: an edge-triggered decoder, called from the
  application's RB-change callback, so it does not miss counts the way a
  poll-driven design would.
- **Two diagnostic counters**: `error_count` (impossible Gray transitions,
  always on) and `glitch_count` (edges rejected by the optional per-instance
  minimum-interval gate), kept separate so each tells a distinct story.
- **Atomic reads**: `epic_encoder_get_position()` wraps the 32-bit read in
  `EPIC_IRQ_Disable`/`Restore` (the ISR writes it asynchronously), mirroring
  `epic_tick_get()`.
- **No `epic-math` link**: only integer add/compare/shift, no multiply.
- **Works on the host simulator and real silicon**; the host test suite
  tests the exact code that ships on-target.

## Documentation

- [API reference](docs/API.md): per-function semantics, the port-byte
  bit-position wiring convention, the at-most-two-encoders-per-port /
  PORTB-only ceiling, the 1 ms glitch-gate resolution, direction-swap
  via pin order.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # test_encoder: 52 checks
# PIC18 family instead:
cmake -B build18 -DEPIC_FAMILY=PIC18 && cmake --build build18
ctest --test-dir build18 --output-on-failure
```

### Real target (XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-encoder-mplabx MCU=16F877A   # also 873A / 874A / 876A
make -C mcu/pic18fxx5x-encoder-mplabx MCU=18F4550   # also 4455
make size
```

The mcu build is a cross-compile sanity check + footprint report
(`epic_encoder_t` is 17 B per instance on both families); correctness is fully
covered on host by `tests/test_encoder.c` (one `encoder.c`, no
per-family variant).

## Use it

```c
#include "encoder.h"
#include "epic_tick.h"
#include "peripherals/hal_gpio.h"   /* EPIC_GPIO_RegisterChangeCallback */
#include "core/hal_irq.h"

static epic_encoder_t enc;

static void on_rb_change(uint8_t portb) { epic_encoder_update(&enc, portb); }

void app_init(void) {
    epic_tick_init(FOSC_HZ);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_MODE_INPUT);
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);
    epic_encoder_init(&enc, 4, 5, 5 /* ms gate */, EPIC_GPIO_ReadPort(GPIOB));
    EPIC_IRQ_Enable(PIC16_IRQ_RB);   /* PIC18_IRQ_RB on the PIC18 family */
    EPIC_IRQ_Restore(1);
}

/* Each control cycle: */
int32_t meas = epic_encoder_get_position(&enc);
int16_t out  = epic_pid_update(&pid, setpoint, (int16_t)meas);
```

See `examples/example_encoder.c` for the complete, target-runnable
wiring (A/B on RB4/RB5, position logged over serial).

## Constraints

- **PORTB only, at most two encoders per port**: mismatch-style IOC exists
  only on PORTB on both families, and RB<7:4> is 4 pins (one A/B pair uses
  2). A hard ceiling, not a tunable.
- **1 ms glitch-gate resolution**: the optional gate uses `epic-tick`'s
  1 ms timebase, adequate for mechanical detent bounce, not sub-ms
  electrical noise.
- **Swap `pin_a`/`pin_b` to invert direction**: the count sign is fixed
  by the shipped `QUAD_TABLE`; invert by swapping the two pin arguments at
  `epic_encoder_init`, not by a flag.
- **Never re-read PORTB in the RB-change callback**: use the byte the
  handler read before clearing RBIF; a later read may not reflect the byte
  the mismatch comparator actually cleared against.
