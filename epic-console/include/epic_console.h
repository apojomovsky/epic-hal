/**
 * @file    epic_console.h
 * @brief   Line-based serial command dispatcher over epic-serial.
 *
 * @details
 *   Buffers one editable input line from `epic-serial`, tokenizes it in
 *   place, and dispatches it through a caller-owned command table. The whole
 *   command set is one `static const epic_console_cmd_t[]`, so there is no
 *   hidden registration, no auto-added commands, and no global tokenizer
 *   state.
 */

#ifndef PIC8_CONSOLE_H
#define PIC8_CONSOLE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Maximum buffered line length including the terminating `\0`.
 *         Override by defining `PIC8_CONSOLE_LINE_MAX` before including the
 *         header.
 */
#ifndef PIC8_CONSOLE_LINE_MAX
#define PIC8_CONSOLE_LINE_MAX 32u
#endif

/**
 * @brief  Maximum number of in-place whitespace-delimited tokens placed in
 *         `argv[]` during dispatch. Override by defining
 *         `PIC8_CONSOLE_MAX_ARGS` before including the header.
 */
#ifndef PIC8_CONSOLE_MAX_ARGS
#define PIC8_CONSOLE_MAX_ARGS 8u
#endif

/**
 * @brief  Command callback signature.
 *
 * @param  argc  Number of tokens in @p argv. For a matched non-empty line,
 *               `argc >= 1` and `argv[0]` is the command name itself.
 * @param  argv  In-place token array pointing into the console's line buffer.
 * @param  ctx   Opaque caller-owned context passed at init time.
 */
typedef void (*epic_console_cmd_fn)(uint8_t argc, char **argv, void *ctx);

/**
 * @brief  One row in a command-dispatch table.
 */
typedef struct {
    const char          *name;    /**< Command name matched against `argv[0]`. */
    epic_console_cmd_fn  handler; /**< Callback to run on a match.             */
    const char          *help;    /**< One-line help text for `print_help()`.  */
} epic_console_cmd_t;

/**
 * @brief  One console instance: command table, opaque context, editable line
 *         buffer, and CR/LF state.
 *
 * @note   Multiple instances are independent. The line buffer lives inside
 *         the instance, not in module-global storage.
 */
typedef struct {
    const epic_console_cmd_t *table;       /**< Command table declared by the caller. */
    uint8_t                   table_len;   /**< Number of rows in `table`.            */
    void                     *ctx;         /**< Opaque pointer passed to handlers.    */
    char                      line[PIC8_CONSOLE_LINE_MAX]; /**< Editable line buffer. */
    uint8_t                   line_len;    /**< Bytes currently buffered in `line`.   */
    bool                      last_was_cr; /**< CR/LF coalescing flag.                */
} epic_console_t;

/**
 * @brief  Initialize a console instance with a caller-owned command table.
 *
 * @param  con        Console instance to initialize.
 * @param  table      Command table; typically a `static const` array.
 * @param  table_len  Number of rows in @p table.
 * @param  ctx        Opaque caller-owned context passed to each handler.
 */
void epic_console_init(epic_console_t *con, const epic_console_cmd_t *table,
                       uint8_t table_len, void *ctx);

/**
 * @brief  Convenience wrapper over @ref epic_console_init that computes the
 *         table length via `sizeof(table) / sizeof(table[0])` at the call
 *         site.
 *
 * @warning `table` must be the actual array here, not a decayed pointer.
 */
#define PIC8_CONSOLE_INIT(con, table, ctx) \
    epic_console_init((con), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), (ctx))

/**
 * @brief  Drain all bytes currently available from `epic-serial`, echo/edit
 *         them, and dispatch any complete lines found during that drain.
 *
 * @param  con  Console instance to service.
 */
void epic_console_poll(epic_console_t *con);

/**
 * @brief  Print one `name - help` line per command-table row through
 *         `epic-serial`.
 *
 * @param  con  Console instance whose table should be listed.
 *
 * @note   This is not auto-bound to any command name. A caller wanting a
 *         `help` command wires this function to its own `"help"` table row.
 */
void epic_console_print_help(const epic_console_t *con);

#endif /* PIC8_CONSOLE_H */
