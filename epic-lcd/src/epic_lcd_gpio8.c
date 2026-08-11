/** 8-bit parallel GPIO transport for epic_lcd; R/W tied low. */

#include "epic_lcd_transport.h"
#include "epic_hal.h"
#include "epic_tick.h"

typedef struct {
    epic_lcd_gpio8_pins_t pins;
} gpio8_ctx_t;

/**
 * @brief Send one byte over the 8-bit parallel bus.
 *
 * Drives RS, writes all eight data lines, then pulses E (HD44780 8-bit
 * protocol).
 *
 * @param ctx  transport context (gpio8_ctx_t)
 * @param rs   register select: 0 = instruction, 1 = data
 * @param byte byte to send
 */
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

/**
 * @brief Delay a number of microseconds using epic-tick.
 *
 * Sub-ms delays are a best-effort busy wait: epic-tick's resolution is
 * 1 ms and the E-pulse setup/hold time is satisfied by the GPIO write
 * call overhead.
 *
 * @param ctx transport context (unused)
 * @param us  microseconds to wait
 */
static void gpio8_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    if (us >= 1000u) {
        epic_tick_delay_ms(us / 1000u);
    }
}

/**
 * @brief Delay a number of milliseconds using epic-tick.
 *
 * @param ctx transport context (unused)
 * @param ms  milliseconds to wait
 */
static void gpio8_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    epic_tick_delay_ms(ms);
}

/**
 * @brief Initialize the 8-bit parallel GPIO transport.
 *
 * Configures the RS/E/DB0-DB7 pins as outputs, asserts E low, and binds
 * the transport's send/delay ops into ops.
 *
 * @param ops   transport ops struct to fill in
 * @param ctx   receives the transport context pointer to pass to ops calls
 * @param pins  GPIO pin assignments for RS, E, and DB0-DB7
 */
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
