/*
 * lib/lcd target example: HD44780 "Hello, World!" banner plus a
 * seconds counter on line 2 of a 16x2 display, driven through the
 * SPI transport (74HC595 shift register). Wiring: SCK=RC3, SDO=RC5
 * (the 16F877A MSSP pins), latch CS=RB0; the 595 Q outputs follow
 * EPIC_LCD_SPI_LAYOUT_COMMON. Adapt CS to your board.
 */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "epic_tick.h"

#include <stdio.h>

/* The transport always runs the SSP at Fosc/64 (312.5 kHz at 20 MHz);
 * spi_hz is reserved and ignored. */
static const epic_lcd_spi_config_t LCD_SPI = {
    .cs_port = GPIOB, .cs_pin = GPIO_PIN_0,
    .fosc_hz = FOSC_HZ, .spi_hz = 0u,
};

/**
 * @brief Print a banner, then a seconds counter updated once per second.
 */
int main(void)
{
    epic_tick_init(FOSC_HZ);

    epic_lcd_ops_t ops;
    void *ops_ctx;
    epic_lcd_spi_init(&ops, &ops_ctx, &LCD_SPI,
                      &EPIC_LCD_SPI_LAYOUT_COMMON);

    epic_lcd_t lcd;
    epic_lcd_config_t cfg = { .cols = 16, .rows = 2, .row_addr = {0} };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);

    epic_lcd_print(&lcd, "Hello, World!");

    char line[17];
    uint32_t seconds = 0u;
    uint32_t last = epic_tick_get();
    for (;;)
    {
        if (epic_tick_elapsed_since(last) >= 1000u)
        {
            last = epic_tick_get();
            seconds++;
            (void)sprintf(line, "up %lu s", (unsigned long)seconds);
            epic_lcd_set_cursor(&lcd, 0, 1);
            epic_lcd_print(&lcd, line);
        }
    }
}
