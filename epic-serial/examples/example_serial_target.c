/*
 * epic-serial on-target demo: send a banner, then loop forever echoing
 * received bytes back to TX. Connect a serial terminal at the
 * configured baud to see it.
 */

#include "epic_serial.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/**
 * @brief Run the on-target serial echo demo.
 *
 * @return 0 when the demo exits (never on target)
 */
int main(void)
{
    epic_harness_init(0UL);
    epic_serial_init(FOSC_HZ, 9600u);

    static const uint8_t banner[] = "epic-serial ready\r\n";
    epic_serial_write(banner, (int)sizeof(banner) - 1);
    epic_serial_flush();

    uint8_t buf[8];
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        int n = epic_serial_read(buf, (int)sizeof(buf));
        if (n > 0) {
            epic_serial_write(buf, n);
        }
        epic_harness_tick();
    }
    return 0;
}
