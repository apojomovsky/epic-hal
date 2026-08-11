# epic-mcp23x17 API

All functions take the expander handle first and return the
transport's status: `> 0` bytes transferred on success (1 for a
single register, 2 for a 16-bit composite), `-1` on a device NACK.

## Types

- `epic_mcp23x17_port_t`: `EPIC_MCP23X17_PORTA`, `EPIC_MCP23X17_PORTB`.
- `epic_mcp23x17_bus_t`: `EPIC_MCP23X17_BUS_I2C` (MCP23017),
  `EPIC_MCP23X17_BUS_SPI` (MCP23S17).
- `epic_mcp23x17_transport_t`: the injectable register transport
  (read_reg / write_reg / ctx). NULL on the handle selects the
  built-in epic-bus transport for the handle's bus.
- `epic_mcp23x17_handle_t`: the caller-owned handle.

## GPIO-mimic layer (HAL-shaped per-pin convenience)

- `MCP23X17_PIN_0..7` / `MCP23X17_PIN_All` : pin masks, values match
  the HAL's `GPIO_PIN_*` so the swap between the MCU's pins and the
  expander is a prefix change.
- `epic_mcp23x17_pin_state_t`: `MCP23X17_PIN_RESET` / `SET` (0/1,
  matching `GPIO_PinState`).
- `epic_mcp23x17_mode_t`: `MCP23X17_MODE_OUTPUT`, `MODE_INPUT`,
  `MODE_INPUT_PULLUP`.
- `EPIC_MCP23X17_GPIO_Init(h, port, pins, mode)` : sets the direction
  (+ pull-ups for INPUT_PULLUP) for the masked pins only; pins
  outside the mask keep their configuration.
- `EPIC_MCP23X17_GPIO_WritePin(h, port, pins, state)` : masked write;
  a read-modify-write of the output latch (two bus transactions, see
  ARCHITECTURE.md).
- `EPIC_MCP23X17_GPIO_TogglePin(h, port, pins)` : masked toggle, same
  RMW.
- `EPIC_MCP23X17_GPIO_ReadPin(h, port, pin)` : returns
  `MCP23X17_PIN_SET`/`RESET` (1/0) or -1 on a NACK.

## Lifecycle

- `EPIC_MCP23X17_Init(h, bus, dev)` : bind to the built-in transport.
  `dev` is the 7-bit I2C address (0b0100A2A1A0) or the SPI A2A1A0
  value (0-7).
- `EPIC_MCP23X17_InitTransport(h, t)` : bind to a custom transport.

## Per-port access

- `SetDirection/GetDirection(h, port, value)` : IODIR; 1 = input.
- `SetInputPolarity/GetInputPolarity(h, port, value)` : IPOL; 1 =
  inverted.
- `SetPullUps/GetPullUps(h, port, value)` : GPPU; 1 = 100k pull-up
  on input pins.
- `WritePort(h, port, value)` : GPIO write (updates the output
  latch).
- `ReadPort(h, port, *value)` : GPIO read (the pins).
- `ReadOutputLatch(h, port, *value)` : OLAT read.

## 16-bit composites (low byte = PORTA)

- `SetDirectionAll(h, uint16_t)`, `GetDirectionAll(h, *uint16_t)`.
- `WriteAll(h, uint16_t)`, `ReadAll(h, *uint16_t)`.
- `SetPullUpsAll(h, uint16_t)`.

## IOCON

- `SetConfig(h, uint8_t)` / `GetConfig(h, *uint8_t)` : BANK, MIRROR,
  SEQOP, DISSLW, HAEN (SPI only), ODR, INTPOL as documented in
  DS20001952E Register 3-5.

## Interrupts

- `SetInterruptEnable(h, port, mask)` : GPINTEN.
- `SetInterruptDefault(h, port, value)` : DEFVAL.
- `SetInterruptControl(h, port, mask)` : INTCON (1 = compare to
  DEFVAL, 0 = pin change).
- `ReadInterruptFlag(h, port, *flags)` : INTF (read-only; does not
  clear).
- `ReadInterruptCapture(h, port, *capture)` : INTCAP (the port value
  captured at the interrupt). The part clears a pending interrupt on
  a GPIO or INTCAP read; the module adds no hidden reads.
