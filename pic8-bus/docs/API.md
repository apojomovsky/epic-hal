# `pic8-bus` API reference

Authoritative declarations: [`include/pic8_bus.h`](../include/pic8_bus.h).

## I2C

### `void pic8_bus_i2c_init(uint32_t fosc_hz, uint32_t fscl_hz)`
Configure the MSSP as an I2C master at `fscl_hz` (e.g. 100000) from
`fosc_hz`, via `SSP_ComputeSSPADD`. Selects the default (HAL) I2C ops. Call
once before `mem_read`/`mem_write`.

### `int pic8_bus_i2c_mem_write(uint8_t dev, uint8_t reg, const uint8_t *data, int n)`
Write `n` bytes to register `reg` on 7-bit-address device `dev`. Transaction:
START, (dev<<1)|W, reg, data..., STOP. Returns `n` (all ACKed) or `-1` (the
device NACKed its address or a byte).

### `int pic8_bus_i2c_mem_read(uint8_t dev, uint8_t reg, uint8_t *buf, int n)`
Read `n` bytes from register `reg` on 7-bit-address device `dev`. Transaction:
START, (dev<<1)|W, reg, REPEATED-START, (dev<<1)|R, read n-1 with ACK, read
last with NACK, STOP. Returns `n` or `-1` (address/reg NACK).

### `void pic8_bus_set_i2c_ops(const pic8_bus_i2c_ops_t *ops)`
Inject a custom I2C ops table (`NULL` restores the default HAL ops). The ops
are `start`, `repeated_start`, `stop`, `write_byte` (returns 1=ACK/0=NACK),
`read_byte(int ack)` (ack=1 sends ACK, 0 sends NACK). Used by the host test
to wire in a mock MEM device.

## SPI

### `void pic8_bus_spi_init(uint32_t fosc_hz, uint32_t f_sclk_hz, uint8_t cs_port, uint8_t cs_pin)`
Configure the MSSP as an SPI master, picking the standard divider (Fosc/4,
/16, /64) closest to `f_sclk_hz` (0 = fastest, Fosc/4). `cs_port`/`cs_pin`
(GPIO port enum 0=A..4=E, pin bit index 0..7) is the chip-select, driven by
`EPIC_GPIO_WritePin` (asserted low, idle high). Selects the default (HAL) SPI
ops.

### `int pic8_bus_spi_mem_write(uint8_t reg, const uint8_t *data, int n)`
Write `n` bytes to register `reg`: CS low, exchange(reg), exchange(data...),
CS high. Returns `n`.

### `int pic8_bus_spi_mem_read(uint8_t reg, uint8_t *buf, int n)`
Read `n` bytes from register `reg`: CS low, exchange(reg), exchange(0)×n
(capturing MISO), CS high. Returns `n`.

### `void pic8_bus_set_spi_ops(const pic8_bus_spi_ops_t *ops)`
Inject a custom SPI ops table (`NULL` restores the default). The ops are
`select`, `deselect`, `exchange(uint8_t)` (write MOSI, return MISO).

## Usage

```c
/* I2C sensor: read 3 bytes from reg 0x10 of device 0x50 */
pic8_bus_i2c_init(FOSC_HZ, 100000);
uint8_t who[3];
if (pic8_bus_i2c_mem_read(0x50, 0x10, who, 3) == 3) { /* got them */ }

/* SPI sensor: write 2 bytes to reg 0x20 */
pic8_bus_spi_init(FOSC_HZ, 0, GPIOB, 0);   /* CS on PB0 */
uint8_t cfg[2] = { 0x01, 0x02 };
pic8_bus_spi_mem_write(0x20, cfg, 2);
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `pic8_bus_i2c_init` | I2C master init + default ops |
| `pic8_bus_i2c_mem_read` | I2C read N bytes from a register |
| `pic8_bus_i2c_mem_write` | I2C write N bytes to a register |
| `pic8_bus_spi_init` | SPI master init + GPIO CS + default ops |
| `pic8_bus_spi_mem_read` | SPI read N bytes from a register |
| `pic8_bus_spi_mem_write` | SPI write N bytes to a register |
| `pic8_bus_set_i2c_ops` / `set_spi_ops` | inject alternate/mock ops |
