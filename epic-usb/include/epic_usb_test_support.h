/*
 * Host-stub-only driver hooks for exercising epic_usb.h's public API
 * contract without real USB hardware. Implemented only by
 * epic_usb_host_stub.c (no faithful host simulation of the real USB SIE
 * exists); only test/example tooling includes this.
 */

#ifndef EPIC_USB_TEST_SUPPORT_H
#define EPIC_USB_TEST_SUPPORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Simulate the host asserting or clearing DTR.
 *
 * Implements the CDC SET_CONTROL_LINE_STATE effect on the stub. Setting
 * true also runs one service() pass, so a write() enqueued while
 * disconnected starts draining immediately once "connected".
 *
 * @param on true to assert DTR, false to clear it
 */
void epic_usb_test_set_dtr(bool on);

/**
 * @brief Simulate data arriving from the host into the RX ring.
 *
 * Acts as if the bytes had come from the OUT endpoint.
 *
 * @param data bytes to inject
 * @param len number of bytes to inject
 * @return the number of bytes actually injected (short if the RX ring
 *         lacks room for all of it)
 */
size_t epic_usb_test_inject_rx(const uint8_t *data, size_t len);

/**
 * @brief Report the number of bytes the stub has transmitted.
 *
 * Counts bytes moved out of the TX ring while connected since the last
 * epic_usb_test_reset_sent().
 *
 * @return the number of transmitted bytes
 */
size_t epic_usb_test_sent_len(void);

/**
 * @brief Return the stub's transmitted-byte log.
 *
 * @return pointer to the log, with epic_usb_test_sent_len() bytes valid
 */
const uint8_t *epic_usb_test_sent_data(void);

/**
 * @brief Clear the transmitted-byte log.
 *
 * Does not touch connection or ring state.
 */
void epic_usb_test_reset_sent(void);

#endif /* EPIC_USB_TEST_SUPPORT_H */
