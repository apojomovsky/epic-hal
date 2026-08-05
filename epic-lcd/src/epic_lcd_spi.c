/**
 * @file    epic_lcd_spi.c
 * @brief   SPI transport for epic_lcd via 74HC595 shift register.
 *
 *          A layout struct maps each 74HC595 Q output to an LCD signal
 *          (RS, E, DB4-DB7). Uses the HAL SSP driver directly, not
 *          epic-bus, since the 595 is a raw shift register, not a
 *          register-addressed device.
 */

#include "epic_lcd.h"
#include "epic_hal.h"
#include "epic_tick.h"

typedef struct {
    const epic_lcd_spi_layout_t *layout;
    GPIO_TypeDef                 cs_port;
    uint16_t                     cs_pin;
} spi_ctx_t;

static uint8_t make_595_byte(const epic_lcd_spi_layout_t *l,
                             uint8_t rs, uint8_t nibble, bool e_asserted)
{
    uint8_t b = 0;

    if (rs)          b |= (uint8_t)(1u << l->rs_bit);
    if (e_asserted)  b |= (uint8_t)(1u << l->e_bit);

    if (nibble & 0x01u) b |= (uint8_t)(1u << l->db4_bit);
    if (nibble & 0x02u) b |= (uint8_t)(1u << l->db5_bit);
    if (nibble & 0x04u) b |= (uint8_t)(1u << l->db6_bit);
    if (nibble & 0x08u) b |= (uint8_t)(1u << l->db7_bit);

    return b;
}

static void spi_latch(spi_ctx_t *s)
{
    EPIC_GPIO_WritePin(s->cs_port, s->cs_pin, GPIO_PIN_SET);
    EPIC_GPIO_WritePin(s->cs_port, s->cs_pin, GPIO_PIN_RESET);
}

static void spi_send_nibble(spi_ctx_t *s, uint8_t rs, uint8_t nibble)
{
    const epic_lcd_spi_layout_t *l = s->layout;
    uint8_t base = make_595_byte(l, rs, nibble, false);

    /* E=0: data setup */
    EPIC_SSP_WriteByte(base);
    spi_latch(s);

    /* E=1: write strobe */
    EPIC_SSP_WriteByte((uint8_t)(base | (1u << l->e_bit)));
    spi_latch(s);

    /* E=0: end of strobe */
    EPIC_SSP_WriteByte(base);
    spi_latch(s);
}

static void spi_send(void *ctx, uint8_t rs, uint8_t byte)
{
    spi_ctx_t *s = (spi_ctx_t *)ctx;
    spi_send_nibble(s, rs, (uint8_t)(byte >> 4u));
    spi_send_nibble(s, rs, (uint8_t)(byte & 0x0Fu));
}

static void spi_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    if (us >= 1000u) {
        epic_tick_delay_ms(us / 1000u);
    }
}

static void spi_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    epic_tick_delay_ms(ms);
}

const epic_lcd_spi_layout_t EPIC_LCD_SPI_LAYOUT_COMMON = {
    .db4_bit = 0,
    .db5_bit = 1,
    .db6_bit = 2,
    .db7_bit = 3,
    .rs_bit  = 4,
    .e_bit   = 5,
    .rw_bit  = 6,
};

void epic_lcd_spi_init(epic_lcd_ops_t *ops, void **ctx,
                       const epic_lcd_spi_config_t *config,
                       const epic_lcd_spi_layout_t *layout)
{
    static spi_ctx_t s;
    s.layout  = layout;
    s.cs_port = config->cs_port;
    s.cs_pin  = config->cs_pin;

    SSP_HandleTypeDef h = SSP_HANDLE_DEFAULT;
    h.Mode = SSP_MODE_SPI_MASTER_FOSC_64;
    EPIC_SSP_Init(&h);

    EPIC_GPIO_Init(s.cs_port, s.cs_pin, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(s.cs_port, s.cs_pin, GPIO_PIN_RESET);

    ops->send     = spi_send;
    ops->delay_us = spi_delay_us;
    ops->delay_ms = spi_delay_ms;
    *ctx = &s;
}
