# epic-serial, interrupt-driven ring-buffered UART

The non-blocking serial layer STM32Cube's `HAL_UART_Transmit_DMA`/
`Receive_DMA`/`_IT` give, for 8-bit PICs, built on the HAL's USART driver.

- **Ring-buffered IT TX/RX**: `write` enqueues and the TX ISR drains in the
  background; received bytes land in an RX ring and `read` pulls them without
  blocking. The main loop is never stuck in a byte loop.
- **Non-variadic formatting (the `put_*` API)**: one function per value type
  (`put_str`, `put_u16`, `put_hex8`, ...), no format string. The same
  eight functions serve XC8 and epic-cc, because epic-cc's freestanding
  libc has no `stdio` and a stackless machine cannot read varargs. `putch`
  stays for XC8's `printf` retarget. The docs teach the `put_*` API, never
  `printf`; the declarations and the examples migrate with epic-hal#88
  (until then the shipped surface is the raw byte I/O plus `putch`, see
  docs/API.md).
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
ctest --test-dir build --output-on-failure   # serial_stress: randomized TX/RX round trips
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

/* formatting, once epic-hal#88 ships the put_* functions: no printf
 * needed, same code on XC8 and epic-cc */
epic_serial_put_str("x=");
epic_serial_put_u16(x);
epic_serial_put_str("\r\n");
```

## License

MIT, see the [repo LICENSE](../LICENSE).
