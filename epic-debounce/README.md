# epic-debounce, vendor-agnostic digital-input debouncer

A reusable, instantiable debouncer for one digital input: given a raw,
possibly-bouncy pin read, decide when the *stable* state has actually changed
and emit a press/release edge event. Multiple instances, each independent
plain data, cover multiple inputs.

- **Vendor-agnostic**: the caller supplies an `epic_debounce_read_fn` callback
  returning `true` = active. The core never sees a HAL type, equally useful
  over a GPIO pin, an I2C-expander bit, or a mock in a test.
- **Depends on `epic-tick`** for its timebase (`epic_tick_get` /
  `epic_tick_elapsed_since`), so the host test suite exercises real timing
  semantics, not a mock clock.
- **One implementation**, `src/debounce.c` compiles unchanged for host, PIC16,
  and PIC18. No per-family backend, no inline asm. Host tests prove the shipped
  code directly.
- **Poll-driven**: call `epic_debounce_poll()` once per scheduler tick or loop
  iteration. No interrupt-on-change wiring needed.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 6 test cases, all pass
```

The module's example (`examples/example_debounce.c`) is target-only: two
debounced buttons driving two LEDs via press/release events, built for
real parts through the manifest's example slot.

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
epic_debounce_t btn;
epic_debounce_init(&btn, read_btn, NULL, 20);   /* 20 ms window */

/* per tick: */
epic_debounce_event_t ev = epic_debounce_poll(&btn);
if (ev == DEBOUNCE_EVENT_PRESSED)  { /* ... */ }
if (ev == DEBOUNCE_EVENT_RELEASED) { /* ... */ }
```

## License

MIT, see the [repo LICENSE](../LICENSE).
