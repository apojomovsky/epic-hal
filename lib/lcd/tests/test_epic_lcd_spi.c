/* Host test for the SPI transport: the real src/epic_lcd_spi.c
 * against the mock_hal SSP/GPIO recorder. Decodes the recorded 595
 * bytes back into (rs, byte) sends and checks the exact HD44780
 * stream the core emits, plus SSP/CS setup and delay mapping. */

#include "epic_lcd.h"
#include "epic_lcd_transport.h"
#include "mock_hal.h"

#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static const epic_lcd_spi_config_t CFG = {
    .cs_port = GPIOB, .cs_pin = GPIO_PIN_0,
    .fosc_hz = 20000000u, .spi_hz = 0u,
};

static epic_lcd_ops_t g_ops;
static void *g_ctx;

/** @brief Init the transport on a clean recorder. */
static void setup(void)
{
    mock_hal_reset();
    epic_lcd_spi_init(&g_ops, &g_ctx, &CFG, &EPIC_LCD_SPI_LAYOUT_COMMON);
}

/** @brief Verify SSP master Fosc/64 setup and idle CS. */
static void test_init(void)
{
    setup();
    CHECK(mock_hal_ssp_init_len() == 1u, "spi init: one SSP init");
    CHECK(mock_hal_ssp_mode() == SSP_MODE_SPI_MASTER_FOSC_64, "spi init: Fosc/64 master");
    CHECK(mock_hal_init_len() == 1u, "spi init: CS configured");
    const mock_gpio_init_t *in = mock_hal_init(0u);
    CHECK(in != NULL && in->port == GPIOB && in->pins == GPIO_PIN_0 &&
          in->mode == GPIO_MODE_OUTPUT, "spi init: CS output");
    CHECK(mock_hal_write_len() == 1u, "spi init: one idle write");
    const mock_gpio_write_t *w = mock_hal_write(0u);
    CHECK(w != NULL && w->port == GPIOB && w->pins == GPIO_PIN_0 &&
          w->state == GPIO_PIN_RESET, "spi init: CS idle low");
}

/* Bit positions from EPIC_LCD_SPI_LAYOUT_COMMON. */
#define RS_BIT 4u
#define E_BIT  5u

/** @brief Check one recorded nibble transfer against (rs, nibble). */
static void check_nibble(unsigned s, unsigned g, uint8_t rs, uint8_t nibble)
{
    uint8_t want = 0u;
    if (rs)
    {
        want |= (uint8_t)(1u << RS_BIT);
    }
    if (nibble & 0x01u) want |= (uint8_t)(1u << 0u);
    if (nibble & 0x02u) want |= (uint8_t)(1u << 1u);
    if (nibble & 0x04u) want |= (uint8_t)(1u << 2u);
    if (nibble & 0x08u) want |= (uint8_t)(1u << 3u);
    CHECK(mock_hal_ssp(s) == want, "spi send: setup byte");
    CHECK(mock_hal_ssp(s + 1u) == (uint8_t)(want | (1u << E_BIT)), "spi send: strobe byte");
    CHECK(mock_hal_ssp(s + 2u) == want, "spi send: release byte");
    for (unsigned k = 0; k < 3u; k++)
    {
        const mock_gpio_write_t *hi = mock_hal_write(g + k * 2u);
        const mock_gpio_write_t *lo = mock_hal_write(g + k * 2u + 1u);
        CHECK(hi != NULL && hi->port == GPIOB && hi->pins == GPIO_PIN_0 &&
              hi->state == GPIO_PIN_SET, "spi send: latch high");
        CHECK(lo != NULL && lo->port == GPIOB && lo->pins == GPIO_PIN_0 &&
              lo->state == GPIO_PIN_RESET, "spi send: latch low");
    }
}

/** @brief Verify direct sends frame each nibble with an E strobe. */
static void test_send_direct(void)
{
    setup();
    unsigned s = mock_hal_ssp_len();
    unsigned g = mock_hal_write_len();
    g_ops.send(g_ctx, 0u, 0xA5u);
    g_ops.send(g_ctx, 1u, 0x00u);
    CHECK(mock_hal_ssp_len() - s == 12u, "spi send: 6 SSP bytes per LCD byte");
    CHECK(mock_hal_write_len() - g == 24u, "spi send: 12 latch writes per LCD byte");
    check_nibble(s, g, 0u, 0x0Au);
    check_nibble(s + 3u, g + 6u, 0u, 0x05u);
    check_nibble(s + 6u, g + 12u, 1u, 0x00u);
    check_nibble(s + 9u, g + 18u, 1u, 0x00u);
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

/** @brief Decode one recorded LCD byte from six SSP bytes. */
static void check_byte(unsigned s, uint8_t rs, uint8_t byte)
{
    uint8_t hi = (uint8_t)(byte >> 4u);
    uint8_t lo = (uint8_t)(byte & 0x0Fu);
    for (unsigned n = 0; n < 2u; n++)
    {
        uint8_t nibble = n == 0u ? hi : lo;
        uint8_t b0 = mock_hal_ssp(s + n * 3u);
        uint8_t b1 = mock_hal_ssp(s + n * 3u + 1u);
        uint8_t b2 = mock_hal_ssp(s + n * 3u + 2u);
        uint8_t want = 0u;
        if (rs)
        {
            want |= (uint8_t)(1u << RS_BIT);
        }
        if (nibble & 0x01u) want |= (uint8_t)(1u << 0u);
        if (nibble & 0x02u) want |= (uint8_t)(1u << 1u);
        if (nibble & 0x04u) want |= (uint8_t)(1u << 2u);
        if (nibble & 0x08u) want |= (uint8_t)(1u << 3u);
        CHECK(b0 == want && b2 == want, "spi e2e: setup/release bytes");
        CHECK(b1 == (uint8_t)(want | (1u << E_BIT)), "spi e2e: strobe byte");
        CHECK((b0 & 0xC0u) == 0u, "spi e2e: unused 595 outputs low");
    }
}

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

    CHECK(mock_hal_ssp_len() == EXPECT_LEN * 6u, "spi e2e: SSP byte count");
    for (unsigned i = 0; i < EXPECT_LEN; i++)
    {
        check_byte(i * 6u, EXPECT[i].rs, EXPECT[i].byte);
    }
    /* One CS latch strobe (high then low) per shifted byte. */
    CHECK(mock_hal_write_len() == 1u + EXPECT_LEN * 12u, "spi e2e: latch count");

    CHECK(mock_hal_tick_len() == 3u, "spi e2e: three tick delays");
    CHECK(mock_hal_tick(0u) == 50u, "spi e2e: power-on 50 ms");
    CHECK(mock_hal_tick(1u) == 4u, "spi e2e: 4500 us becomes 4 ms");
    CHECK(mock_hal_tick(2u) == 1u, "spi e2e: 1600 us becomes 1 ms");
}

/** @brief Run all SPI transport tests and report pass/fail counts. */
int main(void)
{
    test_init();
    test_send_direct();
    test_end_to_end();
    printf("spi: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
