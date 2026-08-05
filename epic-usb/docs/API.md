# `epic-usb` API reference

Authoritative declarations: [`include/epic_usb.h`](../include/epic_usb.h).
Override the ring size with `-DPIC8_USB_RING_SZ=128` (power of two) before
including the header. Default is 64 (one full-speed bulk packet).

### `void epic_usb_init(void)`

Initialize the USB peripheral and start enumeration. Does not block for
enumeration to complete — call `epic_usb_service()` afterwards to drive it,
and check `epic_usb_connected()` before relying on the host being present.
Call once at startup.

### `void epic_usb_service(void)`

Pump the USB stack: services control transfers, drains the TX ring into the
IN endpoint when the host is ready, and fills the RX ring from the OUT
endpoint. Call this often (low single-digit milliseconds) — every main-loop
iteration, or from a short-period `epic-taskmgr` task. Nothing else in this
API does anything useful before `epic_usb_connected()` is true.

### `size_t epic_usb_write(const uint8_t *data, size_t len)`

Enqueue `len` bytes for transmission. Non-blocking with respect to the USB
transfer itself, but blocks (calling `epic_usb_service()` internally) if the
TX ring is full, so the whole buffer is always enqueued before this returns.
Returns the number of bytes enqueued (always `len` unless `len` is 0).

### `size_t epic_usb_read(uint8_t *buf, size_t max)`

Pull up to `max` received bytes from the RX ring. Non-blocking. Returns the
number of bytes actually read (0 if nothing received).

### `size_t epic_usb_available(void)`

Number of bytes available to read from the RX ring.

### `void epic_usb_flush(void)`

Block (servicing internally) until every enqueued TX byte has left the ring
and the IN endpoint is no longer busy. Use before sleep/reboot or to pace
output.

### `bool epic_usb_connected(void)`

True once the host has the CDC port open (DTR asserted via the CDC
`SET_CONTROL_LINE_STATE` request) — not merely once USB enumeration
finished. A host can enumerate the device and never open a terminal; gate
output on this, not on "did init return."

## Usage

```c
epic_usb_init();                                    /* once */

for (;;) {
    epic_usb_service();                              /* pump the stack */

    if (epic_usb_connected()) {                      /* host has the port open */
        uint8_t buf[64];
        size_t n = epic_usb_read(buf, sizeof(buf)); /* non-blocking RX */
        if (n) {
            epic_usb_write(buf, n);                  /* echo back */
        }
    }
}
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `epic_usb_init` | start USB peripheral, begin enumeration |
| `epic_usb_service` | pump the stack (call frequently) |
| `epic_usb_write` | non-blocking background TX (blocks only if ring full) |
| `epic_usb_read` | non-blocking RX pull |
| `epic_usb_available` | RX ring count |
| `epic_usb_flush` | wait for TX fully drained |
| `epic_usb_connected` | host has the CDC port open (DTR) |
