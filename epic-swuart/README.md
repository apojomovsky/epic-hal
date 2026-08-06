# epic-swuart, bit-banged full-duplex UART

Software UART on any GPIO pin: a second serial channel (or a third,
fourth, up to `EPIC_SWUART_MAX_CHANNELS`) when the one hardware EUSART
is already spoken for.

- **Any GPIO pin, any family**: built entirely on the fixed-contract HAL
  (`EPIC_GPIO_*`, `EPIC_TIMER1_*`, `EPIC_IRQ_Restore`), so
  `src/epic_swuart.c` is one file, zero `#ifdef`.
- **Two channels active at once** (configurable via
  `EPIC_SWUART_MAX_CHANNELS`), one shared Timer1 tick drives every
  channel's TX and RX state machine.
- **9600 baud, 8N1**, non-blocking ring-buffered read/write, same shape
  as `epic-serial`'s.
- **Owns Timer1** for as long as any channel is active, the same way
  `epic_tick` owns Timer2. Do not also drive Timer1 directly from
  application code while a channel is initialised.

## Documentation

- [Architecture](docs/ARCHITECTURE.md): the shared-tick design, why
  Timer1, the oversample-factor probe result.
- [API reference](docs/API.md): per-function semantics + usage.

## Quick start

### Host simulator (the tests)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
cmake -B build18 -DEPIC_FAMILY=PIC18 && cmake --build build18 && ctest --test-dir build18
cmake -B build193x -DEPIC_FAMILY=PIC16F193X && cmake --build build193x && ctest --test-dir build193x
```

### Real target (XC8), loopback demo

```sh
export PATH=$PATH:/opt/microchip/xc8/v4.00/bin
python3 scripts/epic_build.py build --module epic-swuart --mcu 16F877A --run
```

## Use it

```c
#include "epic_swuart.h"

EPIC_SWUART_HandleTypeDef h;
EPIC_SWUART_Init(&h, GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2, FOSC_HZ, 9600u);
EPIC_SWUART_Write(&h, (const uint8_t *)"hi\r\n", 4);
uint8_t buf[8];
int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));  /* non-blocking RX */
```

## License

MIT, see the [repo LICENSE](../LICENSE).
