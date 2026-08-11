/*
 * Host-only test double for epic_usb.h, deliberately not sharing
 * implementation with epic_usb.c: no faithful host simulation of a real
 * USB SIE exists, so this proves only the public API's behavioral
 * contract (ring fill/drain, overflow-drop, DTR-gated connected());
 * real enumeration is real-silicon only. The test_support.h hooks
 * (set_dtr/inject_rx/sent-log) simulate the host side.
 */

#include "epic_usb.h"
#include "epic_usb_test_support.h"

#define MASK              (EPIC_USB_RING_SZ - 1u)
#define SENT_LOG_SZ       256u

static uint8_t g_tx_buf[EPIC_USB_RING_SZ];
static uint8_t g_tx_head, g_tx_tail, g_tx_count;
static uint8_t g_rx_buf[EPIC_USB_RING_SZ];
static uint8_t g_rx_head, g_rx_tail, g_rx_count;
static bool    g_dtr;

static uint8_t g_sent_log[SENT_LOG_SZ];
static size_t  g_sent_len;

/**
 * @brief Initialize the stub's ring and connection state.
 *
 * Clears the TX/RX rings, the DTR flag, and the sent-byte log. See
 * epic_usb.h for the full public contract.
 */
void epic_usb_init(void)
{
    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
    g_dtr = false;
    g_sent_len = 0u;
}

/**
 * @brief Drain the TX ring into the sent-byte log while connected.
 *
 * Mirrors the real module's service(): when DTR is clear nothing drains,
 * matching the real module's gating on usb_is_configured().
 */
void epic_usb_service(void)
{
    if (!g_dtr) {
        return;                     /* real module gates draining on usb_is_configured() */
    }
    while (g_tx_count > 0u && g_sent_len < SENT_LOG_SZ) {
        g_sent_log[g_sent_len++] = g_tx_buf[g_tx_tail];
        g_tx_tail = (uint8_t)((g_tx_tail + 1u) & MASK);
        g_tx_count--;
    }
}

/**
 * @brief Enqueue len bytes for transmission.
 *
 * Unlike the real module, short-completes at ring capacity instead of
 * blocking (the stub has no service() loop to block inside). See
 * epic_usb.h for the full public contract.
 *
 * @param data bytes to transmit
 * @param len number of bytes to enqueue
 * @return the number of bytes actually enqueued
 */
size_t epic_usb_write(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_tx_count < EPIC_USB_RING_SZ) {
        g_tx_buf[g_tx_head] = data[n++];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
    }
    epic_usb_service();
    return n;
}

/**
 * @brief Pull up to max received bytes from the RX ring.
 *
 * Non-blocking. See epic_usb.h for the full public contract.
 *
 * @param buf destination buffer
 * @param max maximum number of bytes to read
 * @return the number of bytes actually read (0 if nothing received)
 */
size_t epic_usb_read(uint8_t *buf, size_t max)
{
    size_t n = 0;
    while (n < max && g_rx_count > 0u) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (uint8_t)((g_rx_tail + 1u) & MASK);
        g_rx_count--;
    }
    return n;
}

/**
 * @brief Report the number of bytes available to read from the RX ring.
 *
 * @return the number of received bytes buffered
 */
size_t epic_usb_available(void)
{
    return (size_t)g_rx_count;
}

/**
 * @brief Block until every enqueued TX byte has been transmitted.
 *
 * In the stub this is a single service() pass; see epic_usb.h for the
 * full public contract.
 */
void epic_usb_flush(void)
{
    epic_usb_service();
}

/**
 * @brief Report whether the host has the CDC port open.
 *
 * @return true when DTR is asserted, false otherwise
 */
bool epic_usb_connected(void)
{
    return g_dtr;
}

/* Test-only driver hooks (epic_usb_test_support.h). */

/**
 * @brief Simulate the host asserting or clearing DTR.
 *
 * Setting true also runs one service() pass, so a write() enqueued while
 * disconnected starts draining immediately once "connected".
 *
 * @param on true to assert DTR, false to clear it
 */
void epic_usb_test_set_dtr(bool on)
{
    g_dtr = on;
    if (on) {
        epic_usb_service();
    }
}

/**
 * @brief Simulate data arriving from the host into the RX ring.
 *
 * @param data bytes to inject
 * @param len number of bytes to inject
 * @return the number of bytes actually injected (short if the RX ring
 *         lacks room for all of it)
 */
size_t epic_usb_test_inject_rx(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_rx_count < EPIC_USB_RING_SZ) {
        g_rx_buf[g_rx_head] = data[n++];
        g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
        g_rx_count++;
    }
    return n;
}

/**
 * @brief Report the number of bytes the stub has transmitted.
 *
 * @return the number of bytes in the sent-byte log
 */
size_t epic_usb_test_sent_len(void)
{
    return g_sent_len;
}

/**
 * @brief Return the stub's transmitted-byte log.
 *
 * @return pointer to the log, with epic_usb_test_sent_len() bytes valid
 */
const uint8_t *epic_usb_test_sent_data(void)
{
    return g_sent_log;
}

/**
 * @brief Clear the transmitted-byte log.
 *
 * Does not touch connection or ring state.
 */
void epic_usb_test_reset_sent(void)
{
    g_sent_len = 0u;
}
