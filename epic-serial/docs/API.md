# `epic-serial` API reference

Authoritative declarations: [`include/epic_serial.h`](../include/epic_serial.h).
Override the ring size with `-DEPIC_SERIAL_RING_SZ=64` (power of two) before
including the header.

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
XC8 `printf` retarget: emit one char through the TX ring. Define this and
XC8's `<stdio.h>` `printf` family streams over the UART. On the host build
libc `printf` does not call `putch`, so this is target-firmware use.

## Usage

```c
epic_serial_init(FOSC_HZ, 9600);                 /* once */
epic_serial_write((const uint8_t *)"hello\r\n", 7);

/* non-blocking receive */
uint8_t buf[16];
int n = epic_serial_read(buf, sizeof(buf));
if (n) { /* got n bytes */ }

/* on target, printf just works: */
printf("x=%u\n", x);   /* streams over UART via putch */
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
| `putch` | XC8 `printf` retarget |
