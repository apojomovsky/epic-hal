/**
 * @file    epic_usb.h
 * @brief   USB CDC-ACM (virtual serial) device for PIC18F2455/2550/4455/4550,
 *          wrapping the vendored M-Stack USB device stack
 *          (third_party/m-stack, see epic-usb/docs/epic-usb-plan.md).
 *
 * @details
 *   Mirrors epic_serial.h's contract (non-blocking read/available, write
 *   blocks until enqueued). PIC18Fxx5x-only by hardware necessity, no
 *   USB peripheral on PIC16F87XA. epic_usb_service() must be called
 *   frequently (every main-loop iteration or a short epic-taskmgr task)
 *   to pump enumeration and drain/fill the ring buffers.
 */

#ifndef EPIC_USB_H
#define EPIC_USB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Ring-buffer size (power of two) for both TX and RX. Override by defining
 *  EPIC_USB_RING_SZ before including this header. Default is one full-speed
 *  bulk packet (64 bytes) -- the natural quantum for this endpoint size. */
#ifndef EPIC_USB_RING_SZ
#define EPIC_USB_RING_SZ 64u
#endif

/**
 * @brief  Initialize the USB peripheral and start enumeration. Does not
 *         block for enumeration to complete -- call epic_usb_service()
 *         afterwards to drive it, and check epic_usb_connected() before
 *         relying on the host being present.
 */
void epic_usb_init(void);

/**
 * @brief  Pump the USB stack: services control transfers, drains the TX
 *         ring into the IN endpoint when the host is ready, and fills the
 *         RX ring from the OUT endpoint. Call this often (low
 *         single-digit milliseconds) -- see "Servicing cadence" in the
 *         plan doc for the two supported ways to drive it.
 */
void epic_usb_service(void);

/**
 * @brief  Enqueue @p len bytes for transmission. Non-blocking with respect
 *         to the USB transfer itself, but blocks (calling
 *         epic_usb_service() internally) if the TX ring is full, so the
 *         whole buffer is always enqueued before this returns.
 * @return the number of bytes enqueued (always @p len unless @p len is 0).
 */
size_t epic_usb_write(const uint8_t *data, size_t len);

/**
 * @brief  Pull up to @p max received bytes from the RX ring. Non-blocking.
 * @return the number of bytes actually read (0 if nothing received).
 */
size_t epic_usb_read(uint8_t *buf, size_t max);

/**
 * @brief  Number of bytes available to read from the RX ring.
 */
size_t epic_usb_available(void);

/**
 * @brief  Block (servicing internally) until every enqueued TX byte has
 *         left the ring and the IN endpoint is no longer busy.
 */
void epic_usb_flush(void);

/**
 * @brief  True once the host has the CDC port open (DTR asserted via the
 *         CDC SET_CONTROL_LINE_STATE request) -- not merely once USB
 *         enumeration finished. A host can enumerate the device and never
 *         open a terminal; gate output on this, not on "did init return."
 */
bool epic_usb_connected(void);

#endif /* EPIC_USB_H */
