# epic-modbus, a Modbus RTU slave for 8-bit PICs

A Modbus RTU slave (server) built on `epic-serial` (the UART), `epic-tick`
(the T3.5 inter-frame silence timing), and the HAL's GPIO (an optional
RS-485 driver-enable pin).

- **RTU slave, core function codes**: 01/02/03/04/05/06/15/16 (read/write
  coils, discrete inputs, holding and input registers). No ASCII, no TCP,
  no master role.
- **Plain-array register map**: `epic_modbus_slave_map_t` points straight at
  caller-owned coil/discrete-input/holding-register/input-register arrays,
  no callback indirection, no dynamic allocation.
- **Silence-delimited framing, polled**: `epic_modbus_slave_poll()` drains
  new UART bytes and, once `epic-tick` shows T3.5 has elapsed since the last
  byte, validates (length, CRC-16) and dispatches the frame in one call. Call
  it every main-loop iteration, or wire it as a `epic-taskmgr` task.
- **Family-agnostic**: one `src/epic_modbus.c` builds against
  `pic16f87xa-hal` or `pic18fxx5x-hal` (no `#if` at all, GPIO is already
  neutral through `epic_hal.h`), runs on the host sim and real silicon.
- **Optional RS-485 direction control**: `epic_modbus_slave_set_rs485_dir_pin`
  asserts a driver-enable pin for the duration of each response, held until
  `epic_serial_flush()` confirms the last bit has actually left the pin.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.
- [Implementation plan](../docs/epic-modbus-plan.md), scope, external
  library survey, and the design decisions made before writing code.

### T3.5 caveat

Above 19200 baud the spec fixes T3.5 at 1.75 ms and `epic-tick`'s 1 ms
granularity rounds it up to ~2 ms: framing stays correct, silence
detection just fires later (latency, not a correctness bug).
PIC16F873A/874A (192 B RAM) cannot link this module at default ring
sizes; realistic targets are 16F876A/877A and the PIC18Fxx5x family.

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
#include "epic_modbus.h"
#include "epic_tick.h"

static uint16_t holding_regs[4];

epic_tick_init(FOSC_HZ);                       /* once, before the slave init */

static const epic_modbus_slave_map_t map = {
    .holding_regs     = holding_regs,
    .num_holding_regs = 4,
    /* .coils, .discrete_inputs, .input_regs left NULL/0: not exposed */
};
epic_modbus_slave_init(FOSC_HZ, 9600u, /*slave_addr=*/0x11u, &map);

/* optional: RS-485 transceiver DE/RE tied together on PORTB pin 0 */
epic_modbus_slave_set_rs485_dir_pin(/*port=*/1u, /*pin=*/0u);

for (;;) {
    epic_modbus_slave_poll();   /* call every loop iteration */
}
```

## License

MIT, see the [repo LICENSE](../LICENSE).
