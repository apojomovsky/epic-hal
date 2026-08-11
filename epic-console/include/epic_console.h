/*
 * Line-based serial command dispatcher over epic-serial: buffers one
 * editable input line, tokenizes it in place, and dispatches it through
 * a caller-owned command table (one static const epic_console_cmd_t[],
 * no hidden registration or global tokenizer state).
 */

#ifndef EPIC_CONSOLE_H
#define EPIC_CONSOLE_H

#include <stdint.h>
#include <stdbool.h>

/* Maximum buffered line length including the terminating NUL. Override
 * by defining EPIC_CONSOLE_LINE_MAX before including the header. */
#ifndef EPIC_CONSOLE_LINE_MAX
#define EPIC_CONSOLE_LINE_MAX 32u
#endif

/* Maximum number of in-place whitespace-delimited tokens placed in
 * argv[] during dispatch. Override by defining EPIC_CONSOLE_MAX_ARGS
 * before including the header. */
#ifndef EPIC_CONSOLE_MAX_ARGS
#define EPIC_CONSOLE_MAX_ARGS 8u
#endif

/* Command callback signature. argc is the number of tokens in argv (for
 * a matched non-empty line, argc >= 1 and argv[0] is the command name
 * itself); argv points into the console's line buffer; ctx is the
 * opaque caller-owned context passed at init time. */
typedef void (*epic_console_cmd_fn)(uint8_t argc, char **argv, void *ctx);

/* One row in a command-dispatch table. */
typedef struct {
    const char          *name;    /* Command name matched against argv[0]. */
    epic_console_cmd_fn  handler; /* Callback to run on a match.           */
    const char          *help;    /* One-line help text for print_help().  */
} epic_console_cmd_t;

/* One console instance: command table, opaque context, editable line
 * buffer, and CR/LF state. Multiple instances are independent, and the
 * line buffer lives inside the instance, not in module-global storage. */
typedef struct {
    const epic_console_cmd_t *table;       /* Command table declared by the caller. */
    uint8_t                   table_len;   /* Number of rows in table.               */
    void                     *ctx;         /* Opaque pointer passed to handlers.     */
    char                      line[EPIC_CONSOLE_LINE_MAX]; /* Editable line buffer.  */
    uint8_t                   line_len;    /* Bytes currently buffered in line.      */
    bool                      last_was_cr; /* CR/LF coalescing flag.                 */
} epic_console_t;

/* Initialize a console instance with a caller-owned command table
 * (typically a static const array). */
void epic_console_init(epic_console_t *con, const epic_console_cmd_t *table,
                       uint8_t table_len, void *ctx);

/* Convenience wrapper over epic_console_init that computes the table
 * length via sizeof(table)/sizeof(table[0]) at the call site. The table
 * must be the actual array here, not a decayed pointer. */
#define EPIC_CONSOLE_INIT(con, table, ctx) \
    epic_console_init((con), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), (ctx))

/* Drain all bytes currently available from epic-serial, echo/edit them,
 * and dispatch any complete lines found during that drain. */
void epic_console_poll(epic_console_t *con);

/* Print one "name - help" line per command-table row through
 * epic-serial. Not auto-bound to any command name: a caller wanting a
 * help command wires this function to its own "help" table row. */
void epic_console_print_help(const epic_console_t *con);

#endif /* EPIC_CONSOLE_H */
