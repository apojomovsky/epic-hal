# pic8-debounce, vendor-agnostic digital-input debouncer

A reusable, instantiable debouncer for one digital input: given a raw,
possibly-bouncy pin read, decide when the *stable* state has actually changed
and emit a press/release edge event. Multiple instances, each independent
plain data, cover multiple inputs.

- **Vendor-agnostic**: the caller supplies a `debounce_read_fn` callback
  returning `true` = active. The core never sees a HAL type, equally useful
  over a GPIO pin, an I2C-expander bit, or a mock in a test.
- **Depends on `pic8-tick`** for its timebase (`pic8_tick_get` /
  `pic8_tick_elapsed_since`), so the host test suite exercises real timing
  semantics, not a mock clock.
- **One implementation**, `src/debounce.c` compiles unchanged for host, PIC16,
  and PIC18. No per-family backend, no inline asm. Host tests prove the shipped
  code directly.
- **Poll-driven**: call `debounce_poll()` once per scheduler tick or loop
  iteration. No interrupt-on-change wiring needed.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), the algorithm, the read-callback
  design, why pic8-tick directly, poll vs interrupt, why not pic8-fsm.
- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 6 test cases, all pass
./build/example_debounce_hal                  # two buttons on RA0/RA1, LED on RB0
```

### Real target (XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-debounce-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-debounce-mplabx MCU=18F4550
```

## Use it

```c
#include "debounce.h"
static bool read_btn(void *ctx) {
    (void)ctx;
    return EPIC_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
}
debounce_t btn;
debounce_init(&btn, read_btn, NULL, 20);   /* 20 ms window */

/* per tick: */
debounce_event_t ev = debounce_poll(&btn);
if (ev == DEBOUNCE_EVENT_PRESSED)  { /* ... */ }
if (ev == DEBOUNCE_EVENT_RELEASED) { /* ... */ }
```

## License

MIT, see the [repo LICENSE](../LICENSE).
