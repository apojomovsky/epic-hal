/**
 * @file    test_epic_usb.c
 * @brief   Host tests against epic_usb_host_stub.c: the public API's
 *          behavioral contract only (ring fill/drain, overflow-drop,
 *          connected() transitions). Does not exercise epic_usb.c or
 *          M-Stack, that boundary is real, not a shortcut.
 */

#include "epic_usb.h"
#include "epic_usb_test_support.h"

#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static void reset(void)
{
    epic_usb_init();
    epic_usb_test_reset_sent();
}

static void test_initial_state(void)
{
    reset();
    CHECK(!epic_usb_connected(), "init: not connected");
    CHECK(epic_usb_available() == 0u, "init: RX empty");
    CHECK(epic_usb_test_sent_len() == 0u, "init: nothing sent");
}

static void test_write_holds_until_connected(void)
{
    reset();
    const uint8_t data[] = "hello";
    size_t n = epic_usb_write(data, sizeof(data) - 1u);
    CHECK(n == sizeof(data) - 1u, "write: enqueues full length regardless of connection");
    CHECK(epic_usb_test_sent_len() == 0u, "write while disconnected: nothing sent yet");

    epic_usb_test_set_dtr(true);
    CHECK(epic_usb_connected(), "connected: DTR true after set_dtr(true)");
    CHECK(epic_usb_test_sent_len() == sizeof(data) - 1u,
          "connected: queued write drains immediately");
    CHECK(memcmp(epic_usb_test_sent_data(), data, sizeof(data) - 1u) == 0,
          "connected: sent bytes match what was written");
}

static void test_write_after_connect_drains_on_write(void)
{
    reset();
    epic_usb_test_set_dtr(true);
    const uint8_t data[] = "world";
    epic_usb_write(data, sizeof(data) - 1u);
    CHECK(epic_usb_test_sent_len() == sizeof(data) - 1u,
          "write while already connected: drains without an extra service() call");
}

static void test_disconnect_stops_draining(void)
{
    reset();
    epic_usb_test_set_dtr(true);
    epic_usb_test_set_dtr(false);
    epic_usb_test_reset_sent();

    const uint8_t data[] = "xyz";
    epic_usb_write(data, sizeof(data) - 1u);
    CHECK(epic_usb_test_sent_len() == 0u, "disconnected: write does not drain");

    epic_usb_service();
    CHECK(epic_usb_test_sent_len() == 0u, "disconnected: service() does not drain either");
}

static void test_rx_round_trip(void)
{
    reset();
    const uint8_t data[] = "abc";
    size_t n = epic_usb_test_inject_rx(data, sizeof(data) - 1u);
    CHECK(n == sizeof(data) - 1u, "inject_rx: all bytes accepted (ring has room)");
    CHECK(epic_usb_available() == sizeof(data) - 1u, "available() reflects injected bytes");

    uint8_t buf[8] = {0};
    size_t got = epic_usb_read(buf, sizeof(buf));
    CHECK(got == sizeof(data) - 1u, "read: returns exactly what was injected");
    CHECK(memcmp(buf, data, sizeof(data) - 1u) == 0, "read: bytes match");
    CHECK(epic_usb_available() == 0u, "read: RX ring now empty");
}

static void test_rx_read_is_nonblocking_and_partial(void)
{
    reset();
    uint8_t buf[4];
    CHECK(epic_usb_read(buf, sizeof(buf)) == 0u, "read on empty RX ring returns 0, does not block");

    const uint8_t data[] = "abcdef";
    epic_usb_test_inject_rx(data, sizeof(data) - 1u);
    size_t got = epic_usb_read(buf, 3u);
    CHECK(got == 3u, "read: honors max, returns a short read");
    CHECK(epic_usb_available() == 3u, "read: remaining bytes still available");
}

static void test_rx_overflow_drops(void)
{
    reset();
    uint8_t big[EPIC_USB_RING_SZ + 16u];
    for (size_t i = 0; i < sizeof(big); i++) {
        big[i] = (uint8_t)i;
    }
    size_t n = epic_usb_test_inject_rx(big, sizeof(big));
    CHECK(n == EPIC_USB_RING_SZ, "inject_rx: capped at ring capacity, extra bytes dropped");
    CHECK(epic_usb_available() == EPIC_USB_RING_SZ, "available() caps at ring capacity too");
}

static void test_tx_overflow_short_write_when_disconnected(void)
{
    reset();
    uint8_t big[EPIC_USB_RING_SZ + 16u];
    memset(big, 0xAA, sizeof(big));
    /* Disconnected: nothing drains the TX ring, so write() can only enqueue
     * up to ring capacity before it has nowhere left to put more bytes. */
    size_t n = epic_usb_write(big, sizeof(big));
    CHECK(n == EPIC_USB_RING_SZ,
          "write while disconnected: short-completes at ring capacity, does not hang");
}

static void test_flush_drains_when_connected(void)
{
    reset();
    epic_usb_test_set_dtr(true);
    const uint8_t data[] = "flush-me";
    epic_usb_write(data, sizeof(data) - 1u);
    epic_usb_flush();
    CHECK(epic_usb_test_sent_len() == sizeof(data) - 1u, "flush: fully drains the TX ring");
}

static void test_two_independent_calls_to_reset(void)
{
    /* epic_usb_init() resets all state, including a previous connection. */
    reset();
    epic_usb_test_set_dtr(true);
    epic_usb_write((const uint8_t *)"x", 1u);
    epic_usb_init();
    CHECK(!epic_usb_connected(), "re-init: connection state cleared");
    CHECK(epic_usb_available() == 0u, "re-init: RX ring cleared");
}

int main(void)
{
    test_initial_state();
    test_write_holds_until_connected();
    test_write_after_connect_drains_on_write();
    test_disconnect_stops_draining();
    test_rx_round_trip();
    test_rx_read_is_nonblocking_and_partial();
    test_rx_overflow_drops();
    test_tx_overflow_short_write_when_disconnected();
    test_flush_drains_when_connected();
    test_two_independent_calls_to_reset();

    printf("test_epic_usb: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
