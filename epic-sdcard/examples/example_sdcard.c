/*
 * epic-sdcard on-target demo: bring up the SD card over SPI, read block 0
 * (the first sector), and print its first 16 bytes over epic-serial.
 * PIC18Fxx5x only: a 512-byte block exceeds every PIC16F87XA variant's
 * total RAM, so this module exists only for this family.
 */

#include "epic_sdcard.h"
#include "epic_serial.h"
#include "epic_tick.h"
#include "epic_hal.h"

#include <stdio.h>

/**
 * @brief Read SD block 0 and print its first 16 bytes over epic-serial.
 */
int main(void)
{
    epic_serial_init(FOSC_HZ, 9600u);
    epic_tick_init(FOSC_HZ);     /* card timeouts run on the 1 ms tick */
    EPIC_IRQ_Restore(1);         /* serial RX/TX and the tick ISR need GIE on */

    epic_sdcard_pins_t pins = { .cs_port = GPIOC, .cs_pin = 6 };
    if (!epic_sdcard_init(&pins, FOSC_HZ)) {
        printf("epic-sdcard: init failed\r\n");
        for (;;) {
        }
    }

    printf("epic-sdcard: %lu blocks\r\n",
           (unsigned long)epic_sdcard_num_blocks());

    static uint8_t block[512];
    if (!epic_sdcard_read_block(0, block)) {
        printf("epic-sdcard: read_block(0) failed\r\n");
        for (;;) {
        }
    }

    printf("block 0, first 16 bytes:");
    for (int i = 0; i < 16; i++) {
        printf(" %02x", block[i]);
    }
    printf("\r\n");

    for (;;) {
    }
}
