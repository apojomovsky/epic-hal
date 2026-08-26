# `epic-serial` API reference

Authoritative declarations: [`include/epic_serial.h`](../include/epic_serial.h).
Override the ring size with `-DEPIC_SERIAL_RING_SZ=64` (power of two) before
including the header.

> **Status (epic-hal#91, 2026-08-26):** the `put_*` functions below are the
> decided formatting surface for every toolchain. The declarations land with
> epic-hal#88 (the serial conformance cluster), which implements them and
> migrates the examples; until then the shipped API is the raw byte I/O plus
> `putch` documented in the header.

## Raw byte I/O

### `void epic_serial_init(uint32_t fosc_hz, uint32_t baud)`
Configure async 8N1 at `baud`, start interrupt-driven RX (always on), and arm
IT TX (drains on the first `write`). The USART handle is static (the driver
stores its pointer). Call once at startup.

### `int epic_serial_write(const uint8_t *data, int len)`
Enqueue `len` bytes for background TX. Non-blocking per byte; if the TX ring
fills it blocks until space frees (so the whole buffer is eventually sent).
Enables TXIE to kick the TX ISR. Returns `len`.

### `int epic_serial_read(uint8_t *buf, int max)`
Pull up to `max` received bytes from the RX ring. Non-blocking. Returns the
count read (0 if nothing received).

### `int epic_serial_available(void)`
Bytes available to read from the RX ring (single-byte atomic read).

### `int epic_serial_tx_pending(void)`
Bytes still in the TX ring (not yet loaded into TXREG). 0 means the ring is
empty; the last byte may still be shifting out, use `flush` to wait for that.

### `void epic_serial_flush(void)`
Block until the TX ring is empty AND the shift register has drained. Use
before sleep/reboot or to pace output.

### `void putch(char c)`
Emit one char through the TX ring. On XC8 this is the `printf` retarget:
define it and XC8's `<stdio.h>` `printf` family streams over the UART. On
epic-cc there is no `printf`, so `putch` is just the one-char write; use the
`put_*` functions for formatting. On the host build libc `printf` does not
call `putch`, so this is target-firmware use.

## Formatting (the `put_*` API)

One function per value type, no format string and no variadic arguments, so
the same calls compile under XC8 and epic-cc (epic-hal#91). The decimal
functions emit no leading zeros; the hex functions emit a fixed width with
uppercase digits. All emit through `epic_serial_write`, so they are
non-blocking like any other TX. The decimal conversion shares one static
scratch buffer, so the `put_*` functions are mutually exclusive under
interrupts: do not call one from an ISR while the main loop is mid-format.
Width and padding are deliberately not provided; compose with `put_str` and
`put_char` when a column layout is needed.

### `void epic_serial_put_char(char c)`
Emit one char.

### `void epic_serial_put_str(const char *s)`
Emit the NUL-terminated string `s` (via `strlen`; pass a non-NULL pointer).

### `void epic_serial_put_u16(uint16_t v)`
Emit `v` in decimal, no leading zeros.

### `void epic_serial_put_u32(uint32_t v)`
Emit `v` in decimal, no leading zeros.

### `void epic_serial_put_i16(int16_t v)`
Emit `v` in decimal with a `-` prefix when negative.

### `void epic_serial_put_i32(int32_t v)`
Emit `v` in decimal with a `-` prefix when negative.

### `void epic_serial_put_hex8(uint8_t v)`
Emit `v` as two uppercase hex digits.

### `void epic_serial_put_hex16(uint16_t v)`
Emit `v` as four uppercase hex digits.

## Usage

```c
epic_serial_init(FOSC_HZ, 9600);                 /* once */
epic_serial_write((const uint8_t *)"hello\r\n", 7);

/* non-blocking receive */
uint8_t buf[16];
int n = epic_serial_read(buf, sizeof(buf));
if (n) { /* got n bytes */ }

/* formatting: same code on XC8 and epic-cc */
epic_serial_put_str("count=");
epic_serial_put_u16(count);
epic_serial_put_str(", status=");
epic_serial_put_hex8(status);
epic_serial_put_str("\r\n");
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `epic_serial_init` | start IT UART (RX on, TX armed) |
| `epic_serial_write` | non-blocking background TX (blocks only if ring full) |
| `epic_serial_read` | non-blocking RX pull |
| `epic_serial_available` | RX ring count |
| `epic_serial_tx_pending` | TX ring count |
| `epic_serial_flush` | wait for TX fully drained |
| `putch` | XC8 `printf` retarget, one-char write |
| `epic_serial_put_char` | emit one char |
| `epic_serial_put_str` | emit a NUL-terminated string |
| `epic_serial_put_u16` / `put_u32` | decimal, no leading zeros |
| `epic_serial_put_i16` / `put_i32` | signed decimal with `-` |
| `epic_serial_put_hex8` / `put_hex16` | fixed-width uppercase hex |
