/** "Hello, World!" on both lines of a 16x2 LCD via the 4-bit GPIO
 *  transport. Real-target only (depends on HAL); adapt pins to your board. */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "epic_hal.h"
#include "epic_tick.h"

/**
 * @brief Print "Hello, World!" on both lines of a 16x2 LCD, then blink
 *        the cursor forever.
 */
int main(void)
{
    epic_tick_init(FOSC_HZ);

    epic_lcd_ops_t ops;
    void *ops_ctx;

    epic_lcd_gpio4_pins_t pins = {
        .rs_port = GPIOA, .rs_pin  = GPIO_PIN_0,
        .e_port  = GPIOA, .e_pin   = GPIO_PIN_1,
        .db4_port = GPIOA, .db4_pin = GPIO_PIN_4,
        .db5_port = GPIOA, .db5_pin = GPIO_PIN_5,
        .db6_port = GPIOA, .db6_pin = GPIO_PIN_6,
        .db7_port = GPIOA, .db7_pin = GPIO_PIN_7,
    };
    epic_lcd_gpio4_init(&ops, &ops_ctx, &pins);

    epic_lcd_t lcd;
    epic_lcd_config_t cfg = { .cols = 16, .rows = 2, .row_addr = {0} };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);

    epic_lcd_set_cursor(&lcd, 0, 0);
    epic_lcd_print(&lcd, "Hello, World!");

    epic_lcd_set_cursor(&lcd, 0, 1);
    epic_lcd_print(&lcd, "epic-lcd ready");

    epic_lcd_cursor_blink(&lcd, true);

    for (;;) {
    }
    return 0;
}
