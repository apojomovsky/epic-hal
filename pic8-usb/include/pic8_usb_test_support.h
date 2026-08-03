/**
 * @file    pic8_usb_test_support.h
 * @brief   Host-stub-only driver hooks for exercising pic8_usb.h's public
 *          API contract without real USB hardware.
 *
 * @details
 *   Implemented only by pic8_usb_host_stub.c, not the real-target
 *   pic8_usb.c: there is no faithful host simulation of the real USB SIE,
 *   so this exists as a separate test-only surface. Only test/example
 *   tooling includes it; target firmware never does.
 */

#ifndef PIC8_USB_TEST_SUPPORT_H
#define PIC8_USB_TEST_SUPPORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** Simulate the host asserting/clearing DTR (CDC SET_CONTROL_LINE_STATE).
 *  Setting true also runs one service() pass, so a write() enqueued while
 *  disconnected starts draining immediately once "connected". */
void pic8_usb_test_set_dtr(bool on);

/** Simulate data arriving from the host into the RX ring, as if it had
 *  come from the OUT endpoint. Returns the number of bytes actually
 *  injected (short if the RX ring doesn't have room for all of it). */
size_t pic8_usb_test_inject_rx(const uint8_t *data, size_t len);

/** Number of bytes the stub has "transmitted" (moved out of the TX ring
 *  while connected) since the last pic8_usb_test_reset_sent(). */
size_t pic8_usb_test_sent_len(void);

/** Pointer to the stub's transmitted-byte log (pic8_usb_test_sent_len()
 *  bytes valid). */
const uint8_t *pic8_usb_test_sent_data(void);

/** Clear the transmitted-byte log without touching connection/ring state. */
void pic8_usb_test_reset_sent(void);

#endif /* PIC8_USB_TEST_SUPPORT_H */
