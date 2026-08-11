#include "epic_lcd.h"
#include "epic_lcd_mock_transport.h"

#include <stdio.h>
#include <string.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static epic_lcd_t lcd;
static epic_lcd_ops_t ops;
static void *ops_ctx;

/** @brief Reset the mock, bind ops, and init a fresh 16x2 LCD. */
static void setup(void)
{
    mock_reset();
    mock_ops_init(&ops, &ops_ctx);

    epic_lcd_config_t cfg = {
        .cols = 16,
        .rows = 2,
        .row_addr = {0},
    };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);
}

/** @brief Verify the HD44780 init sequence: 3x Function Set, then Display Off, Clear, Entry Mode, Display On. */
static void test_init_sequence(void)
{
    setup();
    /* Init sends: Function Set x3, Display OFF, Clear, Entry Mode, Display ON */
    CHECK(mock_log_len() >= 6u, "init: at least 6 commands sent");

    /* First three should be Function Set (rs=0, byte with bit 5 set) */
    for (uint16_t i = 0; i < 3u; i++) {
        const mock_entry_t *e = mock_log_entry(i);
        CHECK(e != NULL, "init: entry exists");
        CHECK(e->rs == 0u, "init: first commands are instructions");
        CHECK((e->byte & 0x20u) != 0u, "init: Function Set bit (DB5) set");
    }

    /* Find Clear Display (0x01) and Entry Mode Set */
    bool found_clear = false, found_entry = false;
    for (uint16_t i = 0; i < mock_log_len(); i++) {
        const mock_entry_t *e = mock_log_entry(i);
        if (e->rs == 0u && e->byte == 0x01u) found_clear = true;
        if (e->rs == 0u && (e->byte & 0x04u) && (e->byte & 0x02u)) found_entry = true;
    }
    CHECK(found_clear, "init: Clear Display found");
    CHECK(found_entry, "init: Entry Mode Set found");
}

/** @brief Verify clear sends 0x01 and waits the long delay. */
static void test_clear_command(void)
{
    setup();
    mock_reset();
    epic_lcd_clear(&lcd);
    CHECK(mock_log_len() == 1u, "clear: one command sent");
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "clear: instruction register");
    CHECK(e->byte == 0x01u, "clear: Clear Display opcode");
}

/** @brief Verify home sends 0x02 and waits the long delay. */
static void test_home_command(void)
{
    setup();
    mock_reset();
    epic_lcd_home(&lcd);
    CHECK(mock_log_len() == 1u, "home: one command sent");
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "home: instruction register");
    CHECK(e->byte == 0x02u, "home: Return Home opcode");
}

/** @brief Verify set_cursor on row 0 sends the row-0 DDRAM address. */
static void test_set_cursor_row0(void)
{
    setup();
    mock_reset();
    epic_lcd_set_cursor(&lcd, 5, 0);
    CHECK(mock_log_len() == 1u, "cursor row0: one command");
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "cursor row0: instruction");
    CHECK(e->byte == (0x80u | 0x05u), "cursor row0: DDRAM addr 0x05");
}

/** @brief Verify set_cursor on row 1 sends the row-1 DDRAM address. */
static void test_set_cursor_row1(void)
{
    setup();
    mock_reset();
    epic_lcd_set_cursor(&lcd, 0, 1);
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->byte == (0x80u | 0x40u), "cursor row1: DDRAM addr 0x40");
}

/** @brief Verify write_char sends the character as a data byte. */
static void test_write_char(void)
{
    setup();
    mock_reset();
    epic_lcd_write_char(&lcd, 'A');
    CHECK(mock_log_len() == 1u, "write_char: one byte sent");
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 1u, "write_char: data register");
    CHECK(e->byte == (uint8_t)'A', "write_char: correct character");
}

/** @brief Verify print sends each NUL-terminated character as a data byte. */
static void test_print_string(void)
{
    setup();
    mock_reset();
    epic_lcd_print(&lcd, "Hi");
    CHECK(mock_log_len() == 2u, "print: two bytes for 'Hi'");
    CHECK(mock_log_entry(0)->rs == 1u && mock_log_entry(0)->byte == 'H',
          "print: first char is 'H'");
    CHECK(mock_log_entry(1)->rs == 1u && mock_log_entry(1)->byte == 'i',
          "print: second char is 'i'");
}

/** @brief Verify display_on toggles the D bit of the Display Control command. */
static void test_display_on_off(void)
{
    setup();
    mock_reset();
    epic_lcd_display_on(&lcd, false);
    CHECK(mock_log_len() == 1u, "display off: one command");
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "display off: instruction");
    CHECK((e->byte & 0x08u) != 0u, "display off: Display Ctrl bit set");
    CHECK((e->byte & 0x04u) == 0u, "display off: D bit clear");

    mock_reset();
    epic_lcd_display_on(&lcd, true);
    e = mock_log_entry(0);
    CHECK((e->byte & 0x04u) != 0u, "display on: D bit set");
}

/** @brief Verify cursor_on toggles the C bit of the Display Control command. */
static void test_cursor_on_off(void)
{
    setup();
    mock_reset();
    epic_lcd_cursor_on(&lcd, true);
    const mock_entry_t *e = mock_log_entry(0);
    CHECK((e->byte & 0x02u) != 0u, "cursor on: C bit set");

    mock_reset();
    epic_lcd_cursor_on(&lcd, false);
    e = mock_log_entry(0);
    CHECK((e->byte & 0x02u) == 0u, "cursor off: C bit clear");
    /* Display should still be on */
    CHECK((e->byte & 0x04u) != 0u, "cursor off: display still on");
}

/** @brief Verify cursor_blink toggles the B bit of the Display Control command. */
static void test_cursor_blink(void)
{
    setup();
    mock_reset();
    epic_lcd_cursor_blink(&lcd, true);
    const mock_entry_t *e = mock_log_entry(0);
    CHECK((e->byte & 0x01u) != 0u, "blink on: B bit set");

    mock_reset();
    epic_lcd_cursor_blink(&lcd, false);
    e = mock_log_entry(0);
    CHECK((e->byte & 0x01u) == 0u, "blink off: B bit clear");
}

/** @brief Verify scroll_left/right send the cursor-shift display commands. */
static void test_scroll(void)
{
    setup();
    mock_reset();
    epic_lcd_scroll_left(&lcd);
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "scroll left: instruction");
    CHECK((e->byte & 0x10u) != 0u, "scroll left: Cursor Shift bit");
    CHECK((e->byte & 0x08u) != 0u, "scroll left: S/C bit = display shift");
    CHECK((e->byte & 0x04u) == 0u, "scroll left: R/L bit = left");

    mock_reset();
    epic_lcd_scroll_right(&lcd);
    e = mock_log_entry(0);
    CHECK((e->byte & 0x04u) != 0u, "scroll right: R/L bit = right");
}

/** @brief Verify create_char writes the glyph into CGRAM and returns to DDRAM. */
static void test_create_char(void)
{
    setup();
    mock_reset();
    const uint8_t heart[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
    epic_lcd_create_char(&lcd, 0, heart);

    /* CGRAM addr set (slot 0 = addr 0x40), then 8 data bytes, then DDRAM return */
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->rs == 0u, "create_char: CGRAM addr is instruction");
    CHECK((e->byte & 0x40u) != 0u, "create_char: Set CGRAM Addr bit");

    for (uint8_t i = 0; i < 8u; i++) {
        e = mock_log_entry((uint16_t)(i + 1u));
        CHECK(e->rs == 1u, "create_char: data byte");
        CHECK(e->byte == (heart[i] & 0x1Fu), "create_char: glyph row matches");
    }

    /* Last entry: return to DDRAM */
    e = mock_log_entry(9u);
    CHECK(e->rs == 0u, "create_char: DDRAM return is instruction");
    CHECK((e->byte & 0x80u) != 0u, "create_char: Set DDRAM Addr bit");
}

/** @brief Verify create_char ignores slots above 7. */
static void test_create_char_slot_clamp(void)
{
    setup();
    mock_reset();
    const uint8_t dummy[8] = {0};
    epic_lcd_create_char(&lcd, 8, dummy);  /* slot > 7, should be ignored */
    CHECK(mock_log_len() == 0u, "create_char slot 8: no commands sent");
}

/** @brief Verify a custom row-address table is honored by set_cursor. */
static void test_custom_config_row_addr(void)
{
    mock_reset();
    mock_ops_init(&ops, &ops_ctx);

    epic_lcd_config_t cfg = {
        .cols = 20,
        .rows = 4,
        .row_addr = {0x00, 0x40, 0x14, 0x54},
    };
    epic_lcd_init(&lcd, &ops, ops_ctx, &cfg);

    mock_reset();
    epic_lcd_set_cursor(&lcd, 0, 2);
    const mock_entry_t *e = mock_log_entry(0);
    CHECK(e->byte == (0x80u | 0x14u), "row2: DDRAM addr 0x14");

    mock_reset();
    epic_lcd_set_cursor(&lcd, 0, 3);
    e = mock_log_entry(0);
    CHECK(e->byte == (0x80u | 0x54u), "row3: DDRAM addr 0x54");
}

/** @brief Run all epic-lcd tests and report pass/fail counts. */
int main(void)
{
    test_init_sequence();
    test_clear_command();
    test_home_command();
    test_set_cursor_row0();
    test_set_cursor_row1();
    test_write_char();
    test_print_string();
    test_display_on_off();
    test_cursor_on_off();
    test_cursor_blink();
    test_scroll();
    test_create_char();
    test_create_char_slot_clamp();
    test_custom_config_row_addr();

    printf("test_epic_lcd: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
