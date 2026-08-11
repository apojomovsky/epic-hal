# epic-serial, interrupt-driven ring-buffered UART + printf

The non-blocking serial layer STM32Cube's `HAL_UART_Transmit_DMA`/
`Receive_DMA`/`_IT` give, for 8-bit PICs, built on the HAL's USART driver.

- **Ring-buffered IT TX/RX**: `write` enqueues and the TX ISR drains in the
  background; received bytes land in an RX ring and `read` pulls them without
  blocking. The main loop is never stuck in a byte loop.
- **`printf` retarget**: `putch` routes XC8's `printf` to the TX ring, so
  `printf("x=%u\n", x)` streams over the UART on target.
- **Family-agnostic**: one `src/epic_serial.c` builds against
  `pic16f87xa-hal` or `pic18fxx5x-hal` (one `#if` branch for the
  family-specific USART handle/IRQ/TXREG-write); runs on the host sim and real
  silicon. Configurable ring size (`-DEPIC_SERIAL_RING_SZ=64`).

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator (the test)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # example_serial: rx=2 tx=2
cmake -B build18 -DEPIC_FAMILY=PIC18 && ctest --test-dir build18
```

### Real target (XC8), banner + echo demo

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-serial-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-serial-mplabx MCU=18F4550
```

## Use it

```c
#include "epic_serial.h"
epic_serial_init(FOSC_HZ, 9600);            /* once */
epic_serial_write((const uint8_t *)"hi\r\n", 4);
int n = epic_serial_read(buf, sizeof(buf));  /* non-blocking RX */
printf("x=%u\n", x);                          /* target: via putch */
```

## License

MIT, see the [repo LICENSE](../LICENSE).
