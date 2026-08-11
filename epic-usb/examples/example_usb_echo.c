/*
 * Minimal USB echo: read a byte from the host, write it back. Uses only
 * the public epic_usb.h API, so it builds against either the host stub
 * or the real target. EPIC_USB_EXAMPLE_HOST_ITERS bounds the host loop
 * (no real host to connect, so connected() would never go true); on
 * target the loop runs forever.
 */

#include "epic_usb.h"

#ifdef EPIC_USB_EXAMPLE_HOST_ITERS
#include "epic_usb_test_support.h"
#include <stdio.h>
#endif

int main(void)
{
    epic_usb_init();

#ifdef EPIC_USB_EXAMPLE_HOST_ITERS
    /* Host smoke run: simulate a host connecting and sending a few bytes. */
    epic_usb_test_set_dtr(true);
    const uint8_t sent[] = "ping";
    epic_usb_test_inject_rx(sent, sizeof(sent) - 1u);

    for (int i = 0; i < EPIC_USB_EXAMPLE_HOST_ITERS; i++) {
        epic_usb_service();
        uint8_t b;
        while (epic_usb_connected() && epic_usb_read(&b, 1) == 1) {
            epic_usb_write(&b, 1);
        }
    }
    epic_usb_flush();

    printf("example_usb_echo (host smoke run): echoed %u bytes\n",
           (unsigned)epic_usb_test_sent_len());
    return (epic_usb_test_sent_len() == sizeof(sent) - 1u) ? 0 : 1;
#else
    for (;;) {
        epic_usb_service();
        uint8_t b;
        if (epic_usb_connected() && epic_usb_read(&b, 1) == 1) {
            epic_usb_write(&b, 1);
        }
    }
    return 0;
#endif
}
