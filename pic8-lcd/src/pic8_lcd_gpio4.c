/**
 * @file    pic8_lcd_gpio4.c
 * @brief   4-bit parallel GPIO transport for pic8_lcd. R/W tied low.
 */

#include "pic8_lcd.h"
#include "pic8_hal.h"

typedef struct {
    pic8_lcd_gpio4_pins_t pins;
} gpio4_ctx_t;

static void gpio4_send(void *ctx, uint8_t rs, uint8_t byte)
{
    gpio4_ctx_t *g = (gpio4_ctx_t *)ctx;

    EPIC_GPIO_WritePin(g->pins.rs_port, g->pins.rs_pin,
                      rs ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* High nibble first (HD44780 4-bit protocol) */
    uint8_t hi = (uint8_t)(byte >> 4u);
    EPIC_GPIO_WritePin(g->pins.db4_port, g->pins.db4_pin,
                      (hi & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db5_port, g->pins.db5_pin,
                      (hi & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db6_port, g->pins.db6_pin,
                      (hi & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db7_port, g->pins.db7_pin,
                      (hi & 0x08u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* E pulse: high then low */
    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_SET);
    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_RESET);

    /* Low nibble */
    uint8_t lo = (uint8_t)(byte & 0x0Fu);
    EPIC_GPIO_WritePin(g->pins.db4_port, g->pins.db4_pin,
                      (lo & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db5_port, g->pins.db5_pin,
                      (lo & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db6_port, g->pins.db6_pin,
                      (lo & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db7_port, g->pins.db7_pin,
                      (lo & 0x08u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_SET);
    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_RESET);
}

/* On target, use pic8-tick for ms delays and a busy-wait for us.
 * On host (when built via the HAL's host sim), EPIC_GPIO_WritePin is
 * a no-op and delays are irrelevant -- the mock transport is used instead. */

static void gpio4_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    /* Best-effort busy wait. pic8-tick's resolution is 1ms; for sub-ms
     * delays we approximate. For most HD44780 commands this is fine --
     * the timing is a minimum, not exact. */
    if (us >= 1000u) {
        pic8_tick_delay_ms(us / 1000u);
    }
    /* Sub-ms: no precise timer available on 8-bit PIC without Timer
     * intervention. The E-pulse setup/hold time is already satisfied by
     * the EPIC_GPIO_WritePin call overhead (several us at 20-48 MHz). */
}

static void gpio4_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    pic8_tick_delay_ms(ms);
}

/* No special cold-start handling here: nibble-splitting 0x38 three times
 * (as pic8_lcd_init does before switching modes) reproduces the HD44780's
 * 4-bit init preamble (0x3, 0x3, 0x3, then 0x28 for 4-bit/2-line) on its
 * own, since the LCD only reads the first nibble of each 0x3X send while
 * still in 8-bit mode. */

void pic8_lcd_gpio4_init(pic8_lcd_ops_t *ops, void **ctx,
                         const pic8_lcd_gpio4_pins_t *pins)
{
    static gpio4_ctx_t g;
    g.pins = *pins;

    EPIC_GPIO_Init(g.pins.rs_port,  g.pins.rs_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.e_port,   g.pins.e_pin,   GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db4_port, g.pins.db4_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db5_port, g.pins.db5_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db6_port, g.pins.db6_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db7_port, g.pins.db7_pin,  GPIO_MODE_OUTPUT);

    EPIC_GPIO_WritePin(g.pins.e_port, g.pins.e_pin, GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g.pins.rs_port, g.pins.rs_pin, GPIO_PIN_RESET);

    ops->send     = gpio4_send;
    ops->delay_us = gpio4_delay_us;
    ops->delay_ms = gpio4_delay_ms;
    *ctx = &g;
}
