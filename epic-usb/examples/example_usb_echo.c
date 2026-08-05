/**
 * @file    example_usb_echo.c
 * @brief   Minimal echo: read a byte from the host, write it back.
 *
 * @details
 *   Only calls epic_usb.h's public API, so this same source builds
 *   against either the host stub or the real target's epic_usb.c.
 *   PIC8_USB_EXAMPLE_HOST_ITERS bounds the host loop (there's no real
 *   host to connect, so connected() would otherwise never go true); on
 *   target the loop runs forever, as firmware does.
 */

#include "epic_usb.h"

#ifdef PIC8_USB_EXAMPLE_HOST_ITERS
#include "epic_usb_test_support.h"
#include <stdio.h>
#endif

int main(void)
{
    epic_usb_init();

#ifdef PIC8_USB_EXAMPLE_HOST_ITERS
    /* Host smoke run: simulate a host connecting and sending a few bytes. */
    epic_usb_test_set_dtr(true);
    const uint8_t sent[] = "ping";
    epic_usb_test_inject_rx(sent, sizeof(sent) - 1u);

    for (int i = 0; i < PIC8_USB_EXAMPLE_HOST_ITERS; i++) {
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
