/*
 * USB CDC-ACM (virtual serial) device for PIC18Fxx5x, wrapping the
 * vendored M-Stack stack (third_party/m-stack). PIC18Fxx5x-only by
 * hardware necessity; call epic_usb_service() often to pump it.
 */

#ifndef EPIC_USB_H
#define EPIC_USB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Ring-buffer size (power of two) for both TX and RX. Override by defining
 * EPIC_USB_RING_SZ before including this header. Default is one full-speed
 * bulk packet (64 bytes), the natural quantum for this endpoint size. */
#ifndef EPIC_USB_RING_SZ
#define EPIC_USB_RING_SZ 64u
#endif

/* Initialize the USB peripheral and start enumeration. Does not block
 * for enumeration: call epic_usb_service() afterwards to drive it, and
 * check epic_usb_connected() before relying on the host being present. */
void epic_usb_init(void);

/* Pump the USB stack: control transfers, TX ring into the IN endpoint,
 * OUT endpoint into the RX ring. Call often (low single-digit ms). */
void epic_usb_service(void);

/* Enqueue len bytes for transmission. Non-blocking for the USB transfer
 * itself, but blocks (servicing internally) while the TX ring is full,
 * so the whole buffer is always enqueued before returning. Returns the
 * number of bytes enqueued (len unless len is 0). */
size_t epic_usb_write(const uint8_t *data, size_t len);

/* Pull up to max received bytes from the RX ring. Non-blocking. Returns
 * the number of bytes actually read (0 if nothing received). */
size_t epic_usb_read(uint8_t *buf, size_t max);

/* Number of bytes available to read from the RX ring. */
size_t epic_usb_available(void);

/* Block (servicing internally) until every enqueued TX byte has left
 * the ring and the IN endpoint is no longer busy. */
void epic_usb_flush(void);

/* True once the host has the CDC port open (DTR asserted via the CDC
 * SET_CONTROL_LINE_STATE request), not merely once enumeration finished:
 * a host can enumerate and never open a terminal, so gate output on this. */
bool epic_usb_connected(void);

#endif /* EPIC_USB_H */
