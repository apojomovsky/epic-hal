/**
 * HD44780-compatible character LCD driver with configurable transport
 * (4-bit/8-bit GPIO, SPI via 74HC595). Core logic calls through
 * epic_lcd_ops_t; R/W is tied low in every shipped transport, so commands
 * wait a fixed datasheet delay instead of polling the busy flag.
 */

#ifndef EPIC_LCD_H
#define EPIC_LCD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Transport ops: send a byte and wait for execution. Nibble splitting,
 * E pulsing, or SPI framing happens inside the transport, invisible to
 * the core.
 */
typedef struct {
    /** Send a byte: rs=0 selects the instruction register, rs=1 data. */
    void (*send)(void *ctx, uint8_t rs, uint8_t byte);

    void (*delay_us)(void *ctx, uint32_t us);
    void (*delay_ms)(void *ctx, uint32_t ms);
} epic_lcd_ops_t;

/** Row-address table: DDRAM base address per logical row. Standard HD44780
 *  layout: row 0=0x00, row 1=0x40, row 2=0x14/0x10, row 3=0x54/0x50
 *  (20-col/16-col). EPIC_LCD_MAX_ROWS caps the table size. */
#define EPIC_LCD_MAX_ROWS 4u

typedef struct {
    uint8_t cols;
    uint8_t rows;
    /** DDRAM base address per row; if row_addr[0]==0 at init, defaults to
     *  the standard HD44780 layout (see EPIC_LCD_MAX_ROWS comment above). */
    uint8_t row_addr[EPIC_LCD_MAX_ROWS];
} epic_lcd_config_t;

/** LCD instance. Caller-owned storage; one per display. */
typedef struct {
    const epic_lcd_ops_t *ops;
    void                 *ops_ctx;
    uint8_t               cols;
    uint8_t               rows;
    uint8_t               row_addr[EPIC_LCD_MAX_ROWS];
    uint8_t               display_ctrl;  /**< cached Display/Cursor/Blink bits */
    uint8_t               entry_mode;    /**< cached I/D and S bits            */
} epic_lcd_t;

/** Run the full HD44780 init sequence and apply the caller's
 *  display/cursor/blink and entry-mode defaults. Call once before any
 *  other function; ops/ops_ctx must outlive the lcd instance. */
void epic_lcd_init(epic_lcd_t *lcd, const epic_lcd_ops_t *ops, void *ops_ctx,
                   const epic_lcd_config_t *config);

/** Clear the entire display and return the cursor to row 0, col 0.
 *  Takes ~1.53 ms to execute (the driver waits internally). */
void epic_lcd_clear(epic_lcd_t *lcd);

/** Return the cursor to row 0, col 0. Display contents are not changed.
 *  Takes ~1.53 ms. */
void epic_lcd_home(epic_lcd_t *lcd);

/** Move the cursor to (col, row). Row 0 is the top row. */
void epic_lcd_set_cursor(epic_lcd_t *lcd, uint8_t col, uint8_t row);

void epic_lcd_write_char(epic_lcd_t *lcd, char c);
void epic_lcd_write(epic_lcd_t *lcd, const char *str, size_t len);
void epic_lcd_print(epic_lcd_t *lcd, const char *str);

/** Turn the entire display on or off. Cursor and blink settings are
 *  preserved; nothing is cleared. */
void epic_lcd_display_on(epic_lcd_t *lcd, bool on);

void epic_lcd_cursor_on(epic_lcd_t *lcd, bool on);
void epic_lcd_cursor_blink(epic_lcd_t *lcd, bool on);

/** Shift the entire display one position to the left (cursor doesn't move). */
void epic_lcd_scroll_left(epic_lcd_t *lcd);

void epic_lcd_scroll_right(epic_lcd_t *lcd);

/** Define a custom character in CGRAM slot @p slot (0-7, mapped to
 *  character codes 0x00-0x07). Glyph: 8 bytes, one per row, bottom 5
 *  bits are the pixel row (bit 4 = leftmost pixel, bit 0 = rightmost). */
void epic_lcd_create_char(epic_lcd_t *lcd, uint8_t slot, const uint8_t glyph[8]);

/** Send a raw instruction byte; for commands not covered by the API above. */
void epic_lcd_command(epic_lcd_t *lcd, uint8_t cmd);

/** Send a raw data byte. Writes to DDRAM/CGRAM at the current address. */
void epic_lcd_data(epic_lcd_t *lcd, uint8_t data);

/* GPIO/SPI transport pin structs and init functions live in
 * epic_lcd_transport.h, which includes epic_hal.h; that header is
 * excluded from the host test build. */

#endif /* EPIC_LCD_H */
