# `epic-bus` architecture

The I2C/SPI "MEM" register-access idiom, STM32Cube's `HAL_I2C_Mem_Read`/
`Mem_Write` and the SPI register-transaction pattern, on the HAL's MSSP/SSP
driver.

## What it is

`epic-bus` gives sensor code two calls instead of twenty: "write a register
address, then write N bytes" and "write a register address, then read N
bytes," for both I2C and SPI. One family-agnostic source builds against
`pic16f87xa-hal` or `pic18fxx5x-hal`; the transaction logic is host-testable
through an injectable ops seam.

## The transaction shapes

**I2C `mem_write(dev, reg, data, n)`:** START, `(dev<<1)|W` (ACK), reg (ACK),
data[0..n-1] (ACK each), STOP. Returns `n`, or `-1` if the device NACKs the
address or any byte.

**I2C `mem_read(dev, reg, buf, n)`:** START, `(dev<<1)|W` (ACK), reg (ACK),
REPEATED-START, `(dev<<1)|R` (ACK), read n-1 bytes with ACK, read the last
with NACK, STOP. Returns `n`, or `-1` on address/reg NACK.

**SPI `mem_write(reg, data, n)`:** CS low, exchange(reg), exchange(data[0..n-1]),
CS high.

**SPI `mem_read(reg, buf, n)`:** CS low, exchange(reg), exchange(0)×n
(capturing MISO), CS high.

## The ops seam (and why)

The HAL's SSP driver is **register-level**: it exposes `Start`/`Stop`/
`RepeatedStart`/`WriteByte`/`ReceiveEnable`/`AcknowledgeEnable`/
`AcknowledgeStatus` but NOT the two things a clean MEM transaction needs.
the **ACKDT** bit (to NACK the last read byte) and a **wait-for-idle** poll
(the control functions return immediately). epic-bus's default I2C ops add
those: a read-modify-write of `SSPCON2<ACKDT>` (0=ACK, 1=NACK) and a poll of
SSPIF (`EPIC_IRQ_GetFlag`/`ClearFlag` on the SSP IRQ) before each step.

The host sim has **no SSP slave model** (it never raises SSPIF for bus
operations), so the default ops would hang there. `epic_bus_set_i2c_ops` /
`epic_bus_set_spi_ops` inject an alternate ops table, the host test wires in
a mock MEM device (a register map + transaction state machine) and exercises
the family-neutral transaction LOGIC without hardware. On a real target the
default (HAL) ops are used.

## The one family branch

`SSPCON2`, `ACKDT`, and `ACKSTAT` have the same bit names on both families;
the register address differs (PIC16 bank-1 `0x91`, PIC18 `0xFC5`) and so does
the access idiom (PIC16 `PIC8_REG8 =`, PIC18 `epic_sfr_read8`/`write8`, XC8
can't lower `|=` on a volatile cast lvalue at a runtime SFR address) and the
SSP IRQ number. These are `#if`-gated on the family device define the build
passes; everything else is family-neutral through `epic_hal.h`. The SPI CS
uses `EPIC_GPIO_WritePin` (the HAL's GPIO abstraction), no raw PORTx pokes.

## Testing

- **Host**: a mock I2C + SPI MEM device (injected via the ops seam) verifies
  the transaction shape and byte movement for read/write/read-back, plus the
  wrong-device NACK → `-1` path. Passes on both families.
- **Target**: the XC8 Makefiles build `example_bus_target.c`, which inits the
  I2C + SPI buses (proving the module links against the real HAL). It does
  NOT issue a transaction (the default ops' SSPIF wait would block without a
  device attached); real firmware calls `mem_read`/`mem_write` against an
  attached sensor after init.
