# pic8-tick, a 1 ms timebase for 8-bit PICs

The STM32Cube `HAL_GetTick` / `HAL_Delay` equivalent: a monotonic millisecond
counter, a blocking delay, and a non-blocking elapsed-time helper, built on
the HAL's Timer2 (auto-reload, so the ISR just increments a counter).

- **One family-agnostic API** (`pic8_tick_init` / `pic8_tick_get` /
  `pic8_tick_delay_ms` / `pic8_tick_elapsed_since`), same `src/pic8_tick.c`
  builds against `pic16f87xa-hal` or `pic18fxx5x-hal`.
- **Atomic 32-bit tick read** (interrupts disabled around the read).
- **Works on the host simulator**, `pic8_tick_delay_ms` pumps
  `pic8_harness_tick()` so simulated time advances, and on real silicon.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), Timer2 timebase, period math,
  atomicity, host vs target.
- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # example_tick: delay(10)->10 ms
# PIC18 family instead:
cmake -B build18 -DEPIC_FAMILY=PIC18 && ctest --test-dir build18
```

### Real target (XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-tick-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-tick-mplabx MCU=18F4550
```

## Use it

```c
#include "pic8_tick.h"
pic8_tick_init(FOSC_HZ);            /* once at startup */
pic8_tick_delay_ms(100);            /* blocking 100 ms */
if (pic8_tick_elapsed_since(t0) >= 50u) { /* timeout */ }
```

## License

MIT, see the [repo LICENSE](../LICENSE).
