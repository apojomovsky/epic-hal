# lib/bus, I2C/SPI "MEM" register access for 8-bit PICs

The register-transaction idiom STM32Cube's `HAL_I2C_Mem_Read`/`Mem_Write` and
SPI sensor code use, on the HAL's MSSP/SSP driver, "write a register
address, then read/write N bytes" in one call, for both I2C and SPI.

- **One family-agnostic API** (`epic_bus_i2c_mem_read`/`write`,
  `epic_bus_spi_mem_read`/`write`), same `src/epic_bus.c` builds against
  `hal/pic14/16f87xa` or `hal/pic18/18fxx5x`.
- **Fills the HAL's gaps**: the SSP driver is register-level (no ACKDT
  setter, no wait-for-idle); lib/bus adds the NACK-the-last-byte and
  SSPIF-poll pieces with one small family branch.
- **Host-testable via an ops seam**: inject a mock MEM device
  (`epic_bus_set_i2c_ops`/`set_spi_ops`) to exercise the transaction logic
  (`tests/test_bus_mem.c`, run by ctest). The host sim has no SSP slave
  model, so the default HAL ops are for target.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Example (real target, XC8)

The target-only example (`examples/example_bus.c`) round-trips a register
over I2C and SPI against the HAL ops; the manifest build compiles it:

```sh
make xc8-build MODULE=lib/bus MCU=16F877A
make xc8-build MODULE=lib/bus MCU=18F4550
```

### Host test (the mock-MEM test)

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

### MPLAB SIM gate

The same transaction logic also runs under MPLAB SIM, alongside real SSP
register traffic, in `tests/sim_bus.c`:

```sh
make mdb-test MODULE=lib/bus MCU=16F877A DEVICE=PIC16F877A
```

### epic-cc toggle gate (18F4550)

The epic-cc build for 18F4550 is a mem-dispatch probe
(`mcu/target_sizecheck_epiccc.c`): I2C and SPI register round-trips
through the ops seam against a RAM loopback device, toggling GPIOB
bit 0 while both verify. Its digital-mode write to ADCON1 is what lets
the gate read the pin back:

```sh
make mdb-epiccc MODULE=lib/bus MCU=18F4550 DEVICE=PIC18F4550 REG=PORTB BIT=0
```

The probe proves the mem read/write dispatch under epic-cc. It inits
both buses first (each init installs the default ops table), then
installs the loopback tables over them. No slave exists under the
simulators, so the mock stands in for the MEM device; a silent pin
means a NACK or a data mismatch.

## Use it

```c
#include "epic_bus.h"
epic_bus_i2c_init(FOSC_HZ, 100000);            /* I2C 100 kHz */
uint8_t id[3];
epic_bus_i2c_mem_read(0x50, 0x10, id, 3);      /* read 3 regs */

epic_bus_spi_init(FOSC_HZ, 0, 1, 0);           /* SPI, CS=PB0 */
epic_bus_spi_mem_write(0x20, cfg, 2);          /* write 2 regs */
```

## License

MIT, see the [repo LICENSE](../../LICENSE).
