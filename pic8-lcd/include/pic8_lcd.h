/**
 * @file    pic8_lcd.h
 * @brief   HD44780-compatible character LCD driver (16x2, 20x4, etc.) with
 *          configurable transport (4-bit GPIO, 8-bit GPIO, SPI via 74HC595).
 *
 * @details
 *   Core HD44780 logic is transport-agnostic, calling through
 *   `pic8_lcd_ops_t` (also mockable for host tests). Instance-based, one
 *   `pic8_lcd_t` per display. R/W is tied low in every shipped transport,
 *   so commands wait a fixed datasheet delay instead of polling the busy
 *   flag.
 */

#ifndef PIC8_LCD_H
#define PIC8_LCD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Transport ops (injected by the caller at init time) ---- */

/**
 * @brief  LCD transport operations: send a byte and wait for command
 *         execution. 4-bit nibble splitting, E pulsing, or SPI framing
 *         happens inside the transport, invisible to the core.
 */
typedef struct {
    /** Send a byte. rs=0 for instruction register, rs=1 for data register. */
    void (*send)(void *ctx, uint8_t rs, uint8_t byte);

    /** Delay for at least `us` microseconds. */
    void (*delay_us)(void *ctx, uint32_t us);

    /** Delay for at least `ms` milliseconds. */
    void (*delay_ms)(void *ctx, uint32_t ms);
} pic8_lcd_ops_t;

/* ---- LCD instance ---- */

/** Row-address table: DDRAM base address per logical row. Standard HD44780
 *  layout: row 0=0x00, row 1=0x40, row 2=0x14/0x10, row 3=0x54/0x50
 *  (20-col/16-col). PIC8_LCD_MAX_ROWS caps the table size. */
#define PIC8_LCD_MAX_ROWS 4u

/** LCD configuration, passed at init time. */
typedef struct {
    uint8_t cols;  /**< columns per row (e.g. 16 or 20) */
    uint8_t rows;  /**< number of rows (e.g. 2 or 4)   */
    /** DDRAM base address per row; if row_addr[0]==0 at init, defaults to
     *  the standard HD44780 layout (see PIC8_LCD_MAX_ROWS comment above). */
    uint8_t row_addr[PIC8_LCD_MAX_ROWS];
} pic8_lcd_config_t;

/** LCD instance. Caller-owned storage; one per display. */
typedef struct {
    const pic8_lcd_ops_t *ops;
    void                 *ops_ctx;
    uint8_t               cols;
    uint8_t               rows;
    uint8_t               row_addr[PIC8_LCD_MAX_ROWS];
    uint8_t               display_ctrl;  /**< cached Display/Cursor/Blink bits */
    uint8_t               entry_mode;    /**< cached I/D and S bits            */
} pic8_lcd_t;

/* ---- Lifecycle ---- */

/**
 * @brief  Run the full HD44780 init sequence and apply the caller's
 *         display/cursor/blink and entry-mode defaults. Call once before
 *         any other function; ops/ops_ctx must outlive the lcd instance.
 */
void pic8_lcd_init(pic8_lcd_t *lcd, const pic8_lcd_ops_t *ops, void *ops_ctx,
                   const pic8_lcd_config_t *config);

/* ---- High-level API ---- */

/** Clear the entire display and return the cursor to row 0, col 0.
 *  Takes ~1.53 ms to execute (the driver waits internally). */
void pic8_lcd_clear(pic8_lcd_t *lcd);

/** Return the cursor to row 0, col 0. Display contents are not changed.
 *  Takes ~1.53 ms. */
void pic8_lcd_home(pic8_lcd_t *lcd);

/** Move the cursor to (col, row). Row 0 is the top row. */
void pic8_lcd_set_cursor(pic8_lcd_t *lcd, uint8_t col, uint8_t row);

/** Write one character at the current cursor position and advance. */
void pic8_lcd_write_char(pic8_lcd_t *lcd, char c);

/** Write `len` characters from `str` at the current cursor position. */
void pic8_lcd_write(pic8_lcd_t *lcd, const char *str, size_t len);

/** Write a null-terminated string at the current cursor position. */
void pic8_lcd_print(pic8_lcd_t *lcd, const char *str);

/* ---- Display / cursor control ---- */

/** Turn the entire display on or off. Cursor and blink settings are
 *  preserved; nothing is cleared. */
void pic8_lcd_display_on(pic8_lcd_t *lcd, bool on);

/** Show or hide the underscore cursor at the current position. */
void pic8_lcd_cursor_on(pic8_lcd_t *lcd, bool on);

/** Enable or disable cursor-position blinking (the solid block). */
void pic8_lcd_cursor_blink(pic8_lcd_t *lcd, bool on);

/* ---- Scrolling ---- */

/** Shift the entire display one position to the left (cursor doesn't move). */
void pic8_lcd_scroll_left(pic8_lcd_t *lcd);

/** Shift the entire display one position to the right. */
void pic8_lcd_scroll_right(pic8_lcd_t *lcd);

/* ---- Custom characters ---- */

/**
 * @brief  Define a custom character in CGRAM slot @p slot (0-7).
 * @param  slot   CGRAM slot (0-7, mapped to character codes 0x00-0x07).
 * @param  glyph  8 bytes, one per row, bottom 5 bits are the pixel row
 *                (bit 4 = leftmost pixel, bit 0 = rightmost).
 */
void pic8_lcd_create_char(pic8_lcd_t *lcd, uint8_t slot, const uint8_t glyph[8]);

/* ---- Low-level access ---- */

/** Send a raw instruction byte. For advanced use or commands not covered
 *  by the high-level API. */
void pic8_lcd_command(pic8_lcd_t *lcd, uint8_t cmd);

/** Send a raw data byte. Writes to DDRAM/CGRAM at the current address. */
void pic8_lcd_data(pic8_lcd_t *lcd, uint8_t data);

/* ---- GPIO 4-bit transport ---- */

/* Transport pin structs are declared in pic8_lcd_transport.h (which
 * includes pic8_hal.h). They are not needed by the host test build,
 * which uses only the ops/config types above. */

/* ---- GPIO 8-bit transport ---- */

/* See pic8_lcd_transport.h. */

/* ---- SPI transport (74HC595) ---- */

/* See pic8_lcd_transport.h. */

#endif /* PIC8_LCD_H */
