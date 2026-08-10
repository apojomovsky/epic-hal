# epic-mcp23x17

MCP23017 (I2C) / MCP23S17 (SPI) 16-bit remote I/O expander driver for
8-bit PICs (Microchip DS20001952), on top of the HAL via the
[epic-bus](../epic-bus) transport layer.

Both parts share one register map; only the transport differs. The
driver is family-agnostic: it rides on epic-bus's I2C "MEM" idiom for
the MCP23017 and builds the MCP23S17's control-byte SPI transactions
from epic-bus's raw SPI ops. The transport is injectable, so the host
tests run the full module + epic-bus + mock-device stack.

## Quick start

```c
#include "epic_mcp23x17.h"
#include "epic_bus.h"

/* MCP23017 on the I2C bus at address 0b0100000 (A2A1A0 low). */
epic_bus_i2c_init(FOSC_HZ, 100000UL);          /* I2C master, 100 kHz */
epic_mcp23x17_handle_t h;
EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_I2C, 0x20u);

/* GPA0-7 out, GPB all out; drive a pattern; read it back. */
EPIC_MCP23X17_SetDirectionAll(&h, 0x0000u);
EPIC_MCP23X17_WriteAll(&h, 0x00AAu);
uint16_t port;
EPIC_MCP23X17_ReadAll(&h, &port);
```

For the MCP23S17 (SPI): `epic_bus_spi_init(fosc, sclk, cs_port,
cs_pin)` then `EPIC_MCP23X17_Init(&h, EPIC_MCP23X17_BUS_SPI, a2a1a0)`.
Every accessor returns 0 on success or -1 when the device NACKs, so a
missing expander is surfaced, never swallowed.

## Host build

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/test_mcp23x17          # the mock-device unit tests
./build/example_mcp23x17       # host example (mock device)
```

Select the HAL family with `-DEPIC_FAMILY=PIC18` (default PIC16).
Real-target builds use the manifest + `scripts/epic_build.py`.

## Documentation

- `docs/ARCHITECTURE.md`: the register map, the transport seam, and
  the transaction shapes.
- `docs/API.md`: the full API reference.
- The datasheet: Microchip DS20001952 (16-Bit I/O Expander with
  Serial Interface), linked from the Microchip site.
