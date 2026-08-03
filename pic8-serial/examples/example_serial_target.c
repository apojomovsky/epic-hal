/**
 * @file    example_serial_target.c
 * @brief   pic8-serial on-target demo: send a banner, then loop forever
 *          echoing received bytes back to TX. Connect a serial terminal
 *          at the configured baud to see it.
 */

#include "pic8_serial.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

int main(void)
{
    pic8_harness_init(0UL);
    pic8_serial_init(FOSC_HZ, 9600u);

    static const uint8_t banner[] = "pic8-serial ready\r\n";
    pic8_serial_write(banner, (int)sizeof(banner) - 1);
    pic8_serial_flush();

    uint8_t buf[8];
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        int n = pic8_serial_read(buf, (int)sizeof(buf));
        if (n > 0) {
            pic8_serial_write(buf, n);
        }
        pic8_harness_tick();
    }
    return 0;
}
