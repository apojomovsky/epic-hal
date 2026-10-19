/*
 * lib/lcd target example: HD44780 "Hello, World!" banner plus a
 * seconds counter on line 2 of a 16x2 display, driven through the
 * 8-bit GPIO transport. Pin map: RS=RB0, E=RB1, DB0..DB7=RD0..RD7
 * (16F877A); adapt to your board.
 */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "epic_tick.h"

#include <stdio.h>

static const epic_lcd_gpio8_pins_t LCD_PINS = {
    .rs_port  = GPIOB, .rs_pin  = GPIO_PIN_0,
    .e_port   = GPIOB, .e_pin   = GPIO_PIN_1,
    .db0_port = GPIOD, .db0_pin = GPIO_PIN_0,
    .db1_port = GPIOD, .db1_pin = GPIO_PIN_1,
    .db2_port = GPIOD, .db2_pin = GPIO_PIN_2,
    .db3_port = GPIOD, .db3_pin = GPIO_PIN_3,
    .db4_port = GPIOD, .db4_pin = GPIO_PIN_4,
    .db5_port = GPIOD, .db5_pin = GPIO_PIN_5,
    .db6_port = GPIOD, .db6_pin = GPIO_PIN_6,
    .db7_port = GPIOD, .db7_pin = GPIO_PIN_7,
};

/**
 * @brief Print a banner, then a seconds counter updated once per second.
 */
int main(void)
{
    epic_tick_init(FOSC_HZ);

    epic_lcd_ops_t ops;
    void *ops_ctx;
    epic_lcd_gpio8_init(&ops, &ops_ctx, &LCD_PINS);

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
