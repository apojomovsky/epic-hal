/*
 * epic-serial target demo: 115200 8N1 echo with a banner.
 * Demonstrates the interrupt-driven ring-buffered UART: the put_* family
 * formats through the TX ring on XC8 and epic-cc alike, and received
 * bytes round-trip through the RX ring back to the TX ring.
 */

#include <stdint.h>

#include "epic_serial.h"
#include "core/hal_irq.h"   /* EPIC_IRQ_Restore: enable GIE for the USART */

/* Defined by the build; the fallback keeps the file parseable standalone. */
#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/**
 * @brief 115200 echo demo: printf banner, then loop echoing RX to TX.
 */
int main(void)
{
    epic_serial_init(FOSC_HZ, 115200u);
    EPIC_IRQ_Restore(1);    /* arm the USART RX interrupt (init enables RCIE) */

    /* The banner streams through the put_* family into the TX ring. */
    epic_serial_put_str("epic-serial ready at 115200 8N1\r\n");
    epic_serial_flush();    /* wait for the banner to leave the TX ring */

    /* Ring round-trip: drain whatever arrived into RX and echo it back
     * through the TX ring, byte for byte. */
    uint8_t buf[16];
    for (;;) {
        if (epic_serial_available() > 0) {
            int n = epic_serial_read(buf, (int)sizeof(buf));
            if (n > 0) {
                epic_serial_write(buf, n);
            }
        }
    }
}
