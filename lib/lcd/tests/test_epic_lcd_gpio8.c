/* Host test for the 8-bit GPIO transport: the real
 * src/epic_lcd_gpio8.c against the mock_hal GPIO recorder. Decodes the
 * recorded pin writes back into (rs, byte) sends and checks the exact
 * HD44780 stream the core emits, plus init pin setup and delay mapping. */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "mock_hal.h"

#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static const epic_lcd_gpio8_pins_t PINS = {
    .rs_port = GPIOB, .rs_pin = GPIO_PIN_0,
    .e_port  = GPIOB, .e_pin  = GPIO_PIN_1,
    .db0_port = GPIOD, .db0_pin = GPIO_PIN_0,
    .db1_port = GPIOD, .db1_pin = GPIO_PIN_1,
    .db2_port = GPIOD, .db2_pin = GPIO_PIN_2,
    .db3_port = GPIOD, .db3_pin = GPIO_PIN_3,
    .db4_port = GPIOD, .db4_pin = GPIO_PIN_4,
    .db5_port = GPIOD, .db5_pin = GPIO_PIN_5,
    .db6_port = GPIOD, .db6_pin = GPIO_PIN_6,
    .db7_port = GPIOD, .db7_pin = GPIO_PIN_7,
};

static epic_lcd_ops_t g_ops;
static void *g_ctx;

/** @brief Init the transport on a clean recorder. */
static void setup(void)
{
    mock_hal_reset();
    epic_lcd_gpio8_init(&g_ops, &g_ctx, &PINS);
}

/** @brief Verify init configures all ten pins as outputs, E then RS low. */
static void test_init(void)
{
    setup();
    CHECK(mock_hal_init_len() == 10u, "gpio8 init: ten pins configured");
    for (unsigned i = 0; i < mock_hal_init_len(); i++)
    {
        const mock_gpio_init_t *e = mock_hal_init(i);
        CHECK(e != NULL && e->mode == GPIO_MODE_OUTPUT, "gpio8 init: output mode");
    }
    CHECK(mock_hal_write_len() == 2u, "gpio8 init: two idle writes");
    const mock_gpio_write_t *e0 = mock_hal_write(0u);
    const mock_gpio_write_t *e1 = mock_hal_write(1u);
    CHECK(e0 != NULL && e0->port == GPIOB && e0->pins == GPIO_PIN_1 &&
          e0->state == GPIO_PIN_RESET, "gpio8 init: E driven low");
    CHECK(e1 != NULL && e1->port == GPIOB && e1->pins == GPIO_PIN_0 &&
          e1->state == GPIO_PIN_RESET, "gpio8 init: RS driven low");
}

/** @brief Check one recorded send group against (rs, byte). */
static void check_send(unsigned base, uint8_t rs, uint8_t byte)
{
    static const uint16_t db[8] = {
        GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
        GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    };
    const mock_gpio_write_t *w = mock_hal_write(base);
    CHECK(w != NULL && w->port == GPIOB && w->pins == GPIO_PIN_0 &&
          w->state == (rs ? GPIO_PIN_SET : GPIO_PIN_RESET),
          "gpio8 send: RS first");
    for (unsigned b = 0; b < 8u; b++)
    {
        w = mock_hal_write(base + 1u + b);
        GPIO_PinState want = (byte & (uint8_t)(1u << b)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        CHECK(w != NULL && w->port == GPIOD && w->pins == db[b] && w->state == want,
              "gpio8 send: data bit");
    }
    w = mock_hal_write(base + 9u);
    CHECK(w != NULL && w->port == GPIOB && w->pins == GPIO_PIN_1 &&
          w->state == GPIO_PIN_SET, "gpio8 send: E pulse high");
    w = mock_hal_write(base + 10u);
    CHECK(w != NULL && w->port == GPIOB && w->pins == GPIO_PIN_1 &&
          w->state == GPIO_PIN_RESET, "gpio8 send: E pulse low");
}

/** @brief Verify direct sends drive RS, all data bits, and one E pulse. */
static void test_send_direct(void)
{
    setup();
    unsigned base = mock_hal_write_len();
    g_ops.send(g_ctx, 0u, 0xA5u);
    g_ops.send(g_ctx, 1u, 0x00u);
    CHECK(mock_hal_write_len() - base == 22u, "gpio8 send: 11 writes per byte");
    check_send(base, 0u, 0xA5u);
    check_send(base + 11u, 1u, 0x00u);
}

typedef struct {
    uint8_t rs;
    uint8_t byte;
} expect_t;

/* Same driver script as the target sim gate: init plus cursor, text,
 * scroll, and display-control calls on a 16x2 display. */
static const expect_t EXPECT[] = {
    { 0u, 0x38u }, { 0u, 0x38u }, { 0u, 0x38u }, { 0u, 0x08u },
    { 0u, 0x01u }, { 0u, 0x06u }, { 0u, 0x0Cu }, { 0u, 0x80u },
    { 1u, 0x48u }, { 1u, 0x69u }, { 1u, 0x21u }, { 0u, 0xC0u },
    { 1u, 0x41u }, { 0u, 0x18u }, { 0u, 0x08u }, { 0u, 0x0Cu },
};
#define EXPECT_LEN (sizeof(EXPECT) / sizeof(EXPECT[0]))

/** @brief Verify the full driver script emits the exact byte stream. */
static void test_end_to_end(void)
{
    setup();
    epic_lcd_t lcd;
    epic_lcd_config_t cfg = { .cols = 16u, .rows = 2u, .row_addr = {0u} };
    epic_lcd_init(&lcd, &g_ops, g_ctx, &cfg);
    epic_lcd_set_cursor(&lcd, 0u, 0u);
    epic_lcd_print(&lcd, "Hi!");
    epic_lcd_set_cursor(&lcd, 0u, 1u);
    epic_lcd_write_char(&lcd, 'A');
    epic_lcd_scroll_left(&lcd);
    epic_lcd_display_on(&lcd, false);
    epic_lcd_display_on(&lcd, true);

    /* Two idle writes from init, then 11 recorded writes per send. */
    CHECK(mock_hal_write_len() == 2u + EXPECT_LEN * 11u, "gpio8 e2e: write count");
    for (unsigned i = 0; i < EXPECT_LEN; i++)
    {
        unsigned base = 2u + i * 11u;
        const mock_gpio_write_t *rs = mock_hal_write(base);
        CHECK(rs != NULL && rs->state == (EXPECT[i].rs ? GPIO_PIN_SET : GPIO_PIN_RESET),
              "gpio8 e2e: RS matches");
        uint8_t byte = 0u;
        for (unsigned b = 0; b < 8u; b++)
        {
            const mock_gpio_write_t *w = mock_hal_write(base + 1u + b);
            if (w != NULL && w->state == GPIO_PIN_SET)
            {
                byte |= (uint8_t)(1u << b);
            }
        }
        CHECK(byte == EXPECT[i].byte, "gpio8 e2e: byte matches");
        const mock_gpio_write_t *e_hi = mock_hal_write(base + 9u);
        const mock_gpio_write_t *e_lo = mock_hal_write(base + 10u);
        CHECK(e_hi != NULL && e_hi->state == GPIO_PIN_SET &&
              e_lo != NULL && e_lo->state == GPIO_PIN_RESET, "gpio8 e2e: E pulsed");
    }

    /* Transport delay mapping: 50 ms init, 4500 us -> 4 ms,
     * 1600 us clear -> 1 ms, sub-ms waits are no-ops. */
    CHECK(mock_hal_tick_len() == 3u, "gpio8 e2e: three tick delays");
    CHECK(mock_hal_tick(0u) == 50u, "gpio8 e2e: power-on 50 ms");
    CHECK(mock_hal_tick(1u) == 4u, "gpio8 e2e: 4500 us becomes 4 ms");
    CHECK(mock_hal_tick(2u) == 1u, "gpio8 e2e: 1600 us becomes 1 ms");
}

/** @brief Run all gpio8 transport tests and report pass/fail counts. */
int main(void)
{
    test_init();
    test_send_direct();
    test_end_to_end();
    printf("gpio8: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
