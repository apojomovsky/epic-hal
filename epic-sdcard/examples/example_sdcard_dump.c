/**
 * @file    example_sdcard_dump.c
 * @brief   Read block 0 and print its first 16 bytes over epic-serial.
 *          Real-target only: host tests exercise mmc.c/crc.c directly
 *          against a mock (tests/test_epic_sdcard.c) instead of this
 *          wrapper, since epic_sdcard.c depends on real HAL/SPI.
 */

#include "epic_sdcard.h"
#include "epic_serial.h"

#include <stdio.h>

int main(void)
{
    epic_serial_init(48000000UL, 9600UL);

    epic_sdcard_pins_t pins = { .cs_port = GPIOC, .cs_pin = 6 };
    if (!epic_sdcard_init(&pins, 48000000UL)) {
        printf("epic-sdcard: init failed\r\n");
        for (;;) {
        }
    }

    printf("epic-sdcard: %lu blocks\r\n", (unsigned long)epic_sdcard_num_blocks());

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
    return 0;
}
