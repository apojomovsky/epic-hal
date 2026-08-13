/*
 * epic-usb on-target demo: enumerate as a USB CDC-ACM virtual serial
 * port, then echo everything the host sends back to it. PIC18Fxx5x
 * only: the 8-bit PICs with a USB SIE are all in this family.
 */

#include "epic_usb.h"

/**
 * @brief Run the USB CDC-ACM echo demo.
 */
int main(void)
{
    epic_usb_init();

    uint8_t buf[64];
    for (;;) {
        epic_usb_service();
        if (epic_usb_connected()) {
            size_t n = epic_usb_read(buf, sizeof(buf));
            if (n > 0) {
                epic_usb_write(buf, n);
            }
        }
    }
}
