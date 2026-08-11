# epic-mcp23x17 architecture

The MCP23017 (I2C) and MCP23S17 (SPI) share one register map
(DS20001952E section 3.5); only the serial transport differs. The
driver encodes the register map and the two transaction shapes, and
rides on epic-bus for both buses.

## Register map (IOCON.BANK=0, sequential)

Each 8-bit port has its own register; the pair is base + port
(PORTA=0, PORTB=1):

| Base | Register | Semantics |
|---|---|---|
| 0x00 | IODIR | 1 = input (reset default 0xFF) |
| 0x02 | IPOL | 1 = inverted input polarity |
| 0x04 | GPINTEN | 1 = interrupt-on-change enabled per pin |
| 0x06 | DEFVAL | compare value for INTCON=1 pins |
| 0x08 | INTCON | 1 = compare to DEFVAL, 0 = on pin change |
| 0x0A | IOCON | BANK MIRROR SEQOP DISSLW HAEN ODR INTPOL |
| 0x0B | GPPU | 1 = 100k pull-up on input pins |
| 0x0D | INTF | read-only pending interrupt flags |
| 0x0F | INTCAP | read-only captured port value at the interrupt |
| 0x11 | GPIO | read pins; write updates the output latch |
| 0x13 | OLAT | read the output latch |

A write to GPIO mirrors into OLAT (the part's behavior); reading GPIO
or INTCAP clears a pending interrupt (the part's semantics, the
module never adds hidden reads).

## The transport seam

The handle carries a `epic_mcp23x17_transport_t` (read_reg/write_reg
callbacks + a context). NULL means "use the built-in epic-bus
transport" for the handle's bus:

- **I2C (MCP23017)**: `epic_bus_i2c_mem_read/write(dev, reg, ...)`.
  The 7-bit device address is `0b0100A2A1A0`; the transaction shapes
  are the epic-bus MEM idiom (START, addr|W, reg, data, STOP; the
  read adds REPEATED-START + addr|R + ACK/NACK reads).
- **SPI (MCP23S17)**: the control-byte idiom, built from epic-bus's
  raw SPI ops (`epic_bus_get_spi_ops()`): CS low, exchange(control),
  exchange(reg), exchange(data...), CS high. The control byte is
  `0b0100_0A2A1A0_RW`; the handle's `dev` carries the A2A1A0 bits.
  epic-bus's SPI "MEM" idiom sends the register as the first byte, so
  it cannot express the expander's control byte; the raw ops getters
  (added 2026-08-12) let this module build the real framing.

The host tests inject a mock MCP23017/23S17 (a register file + the
I2C/SPI framing) at the epic-bus ops seam, so the full module +
epic-bus + mock-device stack runs host-side; the same seam that
epic-bus's own gate uses under MPLAB SIM.

## The GPIO-mimic layer and the RMW window

The GPIO-mimic calls (`EPIC_MCP23X17_GPIO_WritePin`/`TogglePin`)
present the HAL's per-pin shape, but the expander's GPIO register is
a whole byte, so a masked write is a read-modify-write of the output
latch: read OLAT, modify, write GPIO. That is two bus transactions
with a window between them, and a concurrent writer to the same port
inside that window can be lost. The whole-port `WritePort`/`WriteAll`
remain single-transaction and atomic; use them when two contexts
touch the same port. The MCU's own GPIO pin write is one (mostly
atomic) instruction, so this is the one place the mimic is genuinely
weaker than the native API; it is the caller's call to make, hence
the documentation here rather than a hidden serialization layer.

## Return values

Every accessor returns the transport's status: the byte count
transferred on success (1 for a single register, 2 for a 16-bit
composite) or -1 when the device NACKs the address or register. A
missing expander is therefore a -1, never silent.

## Limits

MPLAB SIM has no I2C/SPI slave to inject (the MSSP data path is
unmodeled), so the mdb gate proves link + init only; the register and
transaction semantics are covered by the host tests. The interrupt
input (INTA/INTB wired to a PIC pin) is a silicon-only path, see the
plan's follow-up.
