# epic-tick, a 1 ms timebase for 8-bit PICs

The STM32Cube `HAL_GetTick` / `HAL_Delay` equivalent: a monotonic millisecond
counter, a blocking delay, and a non-blocking elapsed-time helper, built on
the HAL's Timer2 (auto-reload, so the ISR just increments a counter).

- **One family-agnostic API** (`epic_tick_init` / `epic_tick_get` /
  `epic_tick_delay_ms` / `epic_tick_elapsed_since`), same `src/epic_tick.c`
  builds against `pic16f87xa-hal` or `pic18fxx5x-hal`.
- **Atomic 32-bit tick read** via double-read retry: disabling GIE is
  unreliable under MPLAB SIM (a latched request can still vector and can
  leave GIE cleared, killing the tick), so the read retries while two
  reads differ instead.
- **Works on the host simulator**, `epic_tick_delay_ms` pumps
  `epic_harness_tick()` so simulated time advances, and on real silicon.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
# PIC18 family instead:
cmake -B build18 -DEPIC_FAMILY=PIC18 && cmake --build build18
```

The host build compiles the [target example](examples/example_tick.c) as a
syntax/link gate; the example itself is a real-silicon program (an LED
blink, no stdout) and is not run here.

### Real target (XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-tick-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-tick-mplabx MCU=18F4550
```

## Use it

```c
#include "epic_tick.h"
epic_tick_init(FOSC_HZ);            /* once at startup */
epic_tick_delay_ms(100);            /* blocking 100 ms */
if (epic_tick_elapsed_since(t0) >= 50u) { /* timeout */ }
```

## License

MIT, see the [repo LICENSE](../LICENSE).
