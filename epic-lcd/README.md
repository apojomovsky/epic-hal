# epic-lcd, HD44780-compatible character LCD driver

A transport-agnostic HD44780/ST7066 character LCD driver with configurable
GPIO and SPI backends, built on the HAL.

- **Transport-agnostic core**: the HD44780 command logic (init, cursor,
  display control, custom characters, print) calls through an injectable
  `epic_lcd_ops_t` struct — the core never touches a GPIO pin or SPI
  peripheral directly.
- **Three shipped transports**: 4-bit GPIO (6 pins), 8-bit GPIO (10 pins),
  and SPI via 74HC595 shift register (3 wires + 1 latch pin).
- **Instance-based**: one `epic_lcd_t` per display, caller-owned storage.
  Multiple independent LCDs are supported.
- **Timed delays (no busy flag)**: R/W is tied low in all transports.
  Commands wait their datasheet-specified execution time — saves a pin,
  avoids read-timing complexity.
- **Host tests prove the command logic**: a mock transport records every
  instruction and data byte, verifying init sequence, DDRAM addressing,
  cursor control, and custom character generation against the HD44780
  spec — without real hardware.
- **Family-agnostic**: any family the HAL supports. The ops/config types
  (`epic_lcd.h`) have zero HAL dependency; transport-specific pin structs
  live in `epic_lcd_transport.h`.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), transport ops, 4-bit init handling,
  SPI/74HC595 protocol, host testing.
- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator (the test)

```sh
cmake -B build -S epic-lcd && cmake --build build
ctest --test-dir build --output-on-failure
```

### Real target (4-bit GPIO, XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
# MCU Makefile not wired up yet -- see example_lcd_hello.c for pin setup
```

## Use it

```c
#include "epic_lcd.h"
#include "epic_lcd_transport.h"

epic_lcd_ops_t ops;
void *ops_ctx;

epic_lcd_gpio4_pins_t pins = {
    .rs_port = GPIOA, .rs_pin  = GPIO_PIN_0,
    .e_port  = GPIOA, .e_pin   = GPIO_PIN_1,
    .db4_port = GPIOA, .db4_pin = GPIO_PIN_4,
    .db5_port = GPIOA, .db5_pin = GPIO_PIN_5,
    .db6_port = GPIOA, .db6_pin = GPIO_PIN_6,
    .db7_port = GPIOA, .db7_pin = GPIO_PIN_7,
};
epic_lcd_gpio4_init(&ops, &ops_ctx, &pins);

epic_lcd_t lcd;
epic_lcd_config_t cfg = { .cols = 16, .rows = 2, .row_addr = {0} };
epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);

epic_lcd_set_cursor(&lcd, 0, 0);
epic_lcd_print(&lcd, "Hello, World!");
```

## License

MIT, see the [repo LICENSE](../LICENSE).
