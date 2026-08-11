# epic-bus, I2C/SPI "MEM" register access for 8-bit PICs

The register-transaction idiom STM32Cube's `HAL_I2C_Mem_Read`/`Mem_Write` and
SPI sensor code use, on the HAL's MSSP/SSP driver, "write a register
address, then read/write N bytes" in one call, for both I2C and SPI.

- **One family-agnostic API** (`epic_bus_i2c_mem_read`/`write`,
  `epic_bus_spi_mem_read`/`write`), same `src/epic_bus.c` builds against
  `pic16f87xa-hal` or `pic18fxx5x-hal`.
- **Fills the HAL's gaps**: the SSP driver is register-level (no ACKDT
  setter, no wait-for-idle); epic-bus adds the NACK-the-last-byte and
  SSPIF-poll pieces with one small family branch.
- **Host-testable via an ops seam**: inject a mock MEM device
  (`epic_bus_set_i2c_ops`/`set_spi_ops`) to exercise the transaction logic.
  the host sim has no SSP slave model, so the default HAL ops are for target.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.

## Quick start

### Host simulator (the test, with a mock MEM device)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # example_bus: I2C+SPI MEM, fails=0
cmake -B build18 -DEPIC_FAMILY=PIC18 && ctest --test-dir build18
```

### Real target (XC8), bus init smoke

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-bus-mplabx MCU=16F877A
make -C mcu/pic18fxx5x-bus-mplabx MCU=18F4550
```

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

MIT, see the [repo LICENSE](../LICENSE).
