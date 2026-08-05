/**
 * @file    epic_usb_host_stub.c
 * @brief   Host-only test double for epic_usb.h. Deliberately does NOT
 *          share implementation with epic_usb.c.
 *
 * @details
 *   No faithful host simulation of a real USB SIE exists, so this only
 *   proves the public API's behavioral contract (ring fill/drain,
 *   overflow-drop, DTR-gated connected()), never real enumeration,
 *   that's real-silicon only. epic_usb_test_support.h's hooks
 *   (set_dtr/inject_rx/sent-log) simulate the host side.
 */

#include "epic_usb.h"
#include "epic_usb_test_support.h"

#define MASK              (PIC8_USB_RING_SZ - 1u)
#define SENT_LOG_SZ       256u

static uint8_t g_tx_buf[PIC8_USB_RING_SZ];
static uint8_t g_tx_head, g_tx_tail, g_tx_count;
static uint8_t g_rx_buf[PIC8_USB_RING_SZ];
static uint8_t g_rx_head, g_rx_tail, g_rx_count;
static bool    g_dtr;

static uint8_t g_sent_log[SENT_LOG_SZ];
static size_t  g_sent_len;

void epic_usb_init(void)
{
    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
    g_dtr = false;
    g_sent_len = 0u;
}

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

size_t epic_usb_write(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_tx_count < PIC8_USB_RING_SZ) {
        g_tx_buf[g_tx_head] = data[n++];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
    }
    epic_usb_service();
    return n;
}

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

size_t epic_usb_available(void)
{
    return (size_t)g_rx_count;
}

void epic_usb_flush(void)
{
    epic_usb_service();
}

bool epic_usb_connected(void)
{
    return g_dtr;
}

/* ---- test-only driver hooks (epic_usb_test_support.h) ---- */

void epic_usb_test_set_dtr(bool on)
{
    g_dtr = on;
    if (on) {
        epic_usb_service();
    }
}

size_t epic_usb_test_inject_rx(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_rx_count < PIC8_USB_RING_SZ) {
        g_rx_buf[g_rx_head] = data[n++];
        g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
        g_rx_count++;
    }
    return n;
}

size_t epic_usb_test_sent_len(void)
{
    return g_sent_len;
}

const uint8_t *epic_usb_test_sent_data(void)
{
    return g_sent_log;
}

void epic_usb_test_reset_sent(void)
{
    g_sent_len = 0u;
}
