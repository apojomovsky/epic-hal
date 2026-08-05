/**
 * @file    epic_lcd_gpio8.c
 * @brief   8-bit parallel GPIO transport for epic_lcd. R/W tied low.
 */

#include "epic_lcd.h"
#include "epic_hal.h"

typedef struct {
    epic_lcd_gpio8_pins_t pins;
} gpio8_ctx_t;

static void gpio8_send(void *ctx, uint8_t rs, uint8_t byte)
{
    gpio8_ctx_t *g = (gpio8_ctx_t *)ctx;

    EPIC_GPIO_WritePin(g->pins.rs_port, g->pins.rs_pin,
                      rs ? GPIO_PIN_SET : GPIO_PIN_RESET);

    EPIC_GPIO_WritePin(g->pins.db0_port, g->pins.db0_pin,
                      (byte & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db1_port, g->pins.db1_pin,
                      (byte & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db2_port, g->pins.db2_pin,
                      (byte & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db3_port, g->pins.db3_pin,
                      (byte & 0x08u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db4_port, g->pins.db4_pin,
                      (byte & 0x10u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db5_port, g->pins.db5_pin,
                      (byte & 0x20u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db6_port, g->pins.db6_pin,
                      (byte & 0x40u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g->pins.db7_port, g->pins.db7_pin,
                      (byte & 0x80u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_SET);
    EPIC_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_RESET);
}

static void gpio8_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    if (us >= 1000u) {
        epic_tick_delay_ms(us / 1000u);
    }
}

static void gpio8_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    epic_tick_delay_ms(ms);
}

void epic_lcd_gpio8_init(epic_lcd_ops_t *ops, void **ctx,
                         const epic_lcd_gpio8_pins_t *pins)
{
    static gpio8_ctx_t g;
    g.pins = *pins;

    EPIC_GPIO_Init(g.pins.rs_port,  g.pins.rs_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.e_port,   g.pins.e_pin,   GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db0_port, g.pins.db0_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db1_port, g.pins.db1_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db2_port, g.pins.db2_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db3_port, g.pins.db3_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db4_port, g.pins.db4_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db5_port, g.pins.db5_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db6_port, g.pins.db6_pin,  GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(g.pins.db7_port, g.pins.db7_pin,  GPIO_MODE_OUTPUT);

    EPIC_GPIO_WritePin(g.pins.e_port, g.pins.e_pin, GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(g.pins.rs_port, g.pins.rs_pin, GPIO_PIN_RESET);

    ops->send     = gpio8_send;
    ops->delay_us = gpio8_delay_us;
    ops->delay_ms = gpio8_delay_ms;
    *ctx = &g;
}
