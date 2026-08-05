# pic8-modbus, a Modbus RTU slave for 8-bit PICs

A Modbus RTU slave (server) built on `pic8-serial` (the UART), `pic8-tick`
(the T3.5 inter-frame silence timing), and the HAL's GPIO (an optional
RS-485 driver-enable pin).

- **RTU slave, core function codes**: 01/02/03/04/05/06/15/16 (read/write
  coils, discrete inputs, holding and input registers). No ASCII, no TCP,
  no master role, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for what's
  deliberately out of scope.
- **Plain-array register map**: `pic8_modbus_slave_map_t` points straight at
  caller-owned coil/discrete-input/holding-register/input-register arrays,
  no callback indirection, no dynamic allocation.
- **Silence-delimited framing, polled**: `pic8_modbus_slave_poll()` drains
  new UART bytes and, once `pic8-tick` shows T3.5 has elapsed since the last
  byte, validates (length, CRC-16) and dispatches the frame in one call. Call
  it every main-loop iteration, or wire it as a `pic8-taskmgr` task.
- **Family-agnostic**: one `src/pic8_modbus.c` builds against
  `pic16f87xa-hal` or `pic18fxx5x-hal` (no `#if` at all, GPIO is already
  neutral through `pic8_hal.h`), runs on the host sim and real silicon.
- **Optional RS-485 direction control**: `pic8_modbus_slave_set_rs485_dir_pin`
  asserts a driver-enable pin for the duration of each response, held until
  `pic8_serial_flush()` confirms the last bit has actually left the pin.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), the framing algorithm, CRC-16
  choice, the RS-485 timing, and the known timing-resolution caveat above
  19200 baud.
- [API reference](docs/API.md), per-function semantics + usage.
- [Implementation plan](../docs/pic8-modbus-plan.md), scope, external
  library survey, and the design decisions made before writing code.

## Quick start

### Host simulator (the test)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # example_modbus: fails=0
cmake -B build18 -DEPIC_FAMILY=PIC18 && ctest --test-dir build18
```

### Real target (XC8), a 4-holding-register RTU slave

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-modbus-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-modbus-mplabx MCU=18F4550
```

## Use it

```c
#include "pic8_modbus.h"
#include "pic8_tick.h"

static uint16_t holding_regs[4];

pic8_tick_init(FOSC_HZ);                       /* once, before the slave init */

static const pic8_modbus_slave_map_t map = {
    .holding_regs     = holding_regs,
    .num_holding_regs = 4,
    /* .coils, .discrete_inputs, .input_regs left NULL/0: not exposed */
};
pic8_modbus_slave_init(FOSC_HZ, 9600u, /*slave_addr=*/0x11u, &map);

/* optional: RS-485 transceiver DE/RE tied together on PORTB pin 0 */
pic8_modbus_slave_set_rs485_dir_pin(/*port=*/1u, /*pin=*/0u);

for (;;) {
    pic8_modbus_slave_poll();   /* call every loop iteration */
}
```

## License

MIT, see the [repo LICENSE](../LICENSE).
