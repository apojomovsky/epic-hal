# `epic-modbus` API reference

Authoritative declarations: [`include/epic_modbus.h`](../include/epic_modbus.h).
Override the frame buffer size with `-DEPIC_MODBUS_MAX_ADU=128` (bytes)
before including the header, if a larger register map needs bigger
read/write-multiple responses than the 64-byte default allows.

### `epic_modbus_slave_map_t`

```c
typedef struct {
    uint8_t        *coils;              /* bit-packed, read/write        */
    uint16_t        num_coils;
    const uint8_t  *discrete_inputs;    /* bit-packed, read-only         */
    uint16_t        num_discrete_inputs;
    uint16_t       *holding_regs;       /* read/write                    */
    uint16_t        num_holding_regs;
    const uint16_t *input_regs;         /* read-only                     */
    uint16_t        num_input_regs;
} epic_modbus_slave_map_t;
```

The slave's register map, caller-owned storage. Coils/discrete inputs are
bit-packed (LSB of `array[0]` is address 0, Modbus's own within-byte bit
order); holding/input registers are one `uint16_t` per address. Leave an
array `NULL` (count 0) to not expose that table at all, requests against it
get exception `ILLEGAL_DATA_ADDRESS` (0x02).

### `void epic_modbus_slave_init(uint32_t fosc_hz, uint32_t baud, uint8_t slave_addr, const epic_modbus_slave_map_t *map)`
Initialize the slave: configures the UART (calls `epic_serial_init`
internally, so the baud used for framing and the baud used for T3.5 timing
math can never drift apart) and precomputes the T3.5 inter-frame timeout
for `baud`. Call once at startup, **after** `epic_tick_init`. `slave_addr`
is 1..247 (0 is reserved for broadcast, don't pass it here). `map` is
stored by pointer, not copied, keep it alive for the program's lifetime.

### `void epic_modbus_slave_set_rs485_dir_pin(uint8_t port, uint8_t pin)`
Optional. Configures a GPIO pin as an RS-485 driver-enable, asserted for the
duration of each response transmission (raised before `epic_serial_write`,
held through `epic_serial_flush`, then dropped). `port` is a HAL GPIO port
index (`GPIOA`=0, `GPIOB`=1, ...), `pin` a bit index 0..7. If never called,
responses go out through `epic_serial_write` untouched.

### `void epic_modbus_slave_poll(void)`
Call every main-loop iteration (or wire as a `epic-taskmgr` task). Drains
newly received UART bytes into the internal frame buffer; once
`epic-tick` shows the RTU T3.5 silence has elapsed since the last received
byte, validates the frame (length, CRC-16, address match or broadcast) and,
if valid, dispatches it and transmits the response (broadcast requests
never get a response). The frame buffer resets after every dispatch
attempt, valid or not, so the next byte always starts a fresh frame.

## Function codes implemented

| FC | Name | Notes |
|----|------|-------|
| 0x01 | Read Coils | qty 1..2000 (buffer-limited in practice, see below) |
| 0x02 | Read Discrete Inputs | same shape as 0x01 |
| 0x03 | Read Holding Registers | qty 1..125 (buffer-limited) |
| 0x04 | Read Input Registers | same shape as 0x03 |
| 0x05 | Write Single Coil | value must be `0xFF00` (ON) or `0x0000` (OFF) |
| 0x06 | Write Single Register | any 16-bit value |
| 0x0F | Write Multiple Coils | qty 1..1968, byte count must match qty |
| 0x10 | Write Multiple Registers | qty 1..123, byte count must match qty |

Any other function code gets exception `ILLEGAL_FUNCTION` (0x01). Any
in-range request whose address/quantity falls outside the configured table
gets `ILLEGAL_DATA_ADDRESS` (0x02). An out-of-range quantity, a bad
single-coil value, or a byte-count/quantity mismatch gets
`ILLEGAL_DATA_VALUE` (0x03). A request whose read response wouldn't fit in
`EPIC_MODBUS_MAX_ADU` also gets `ILLEGAL_DATA_VALUE`, a practical
buffer-size constraint (at the 64-byte default: up to ~29 holding/input
registers, or ~475 coils/discrete inputs, per single read).

## Usage

```c
#include "epic_modbus.h"
#include "epic_tick.h"

static uint16_t holding_regs[4];
static uint8_t  coils[1];   /* 8 coils */

epic_tick_init(FOSC_HZ);

static const epic_modbus_slave_map_t map = {
    .coils               = coils,
    .num_coils           = 8,
    .holding_regs        = holding_regs,
    .num_holding_regs    = 4,
};
epic_modbus_slave_init(FOSC_HZ, 9600u, 0x11u, &map);
epic_modbus_slave_set_rs485_dir_pin(1u /* GPIOB */, 0u /* RB0 */);

for (;;) {
    epic_modbus_slave_poll();
}
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `epic_modbus_slave_init` | configure UART + T3.5 timing, store the register map |
| `epic_modbus_slave_set_rs485_dir_pin` | optional RS-485 driver-enable pin |
| `epic_modbus_slave_poll` | drain RX, detect T3.5 silence, dispatch + respond |
