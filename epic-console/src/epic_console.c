/* Line-based serial command dispatcher over epic-serial. */

#include "epic_console.h"
#include "epic_serial.h"

#include <stddef.h>
#include <string.h>

/**
 * @brief Write a NUL-terminated string through epic-serial.
 *
 * @param s the string to write
 */
static void epic_console_write_str(const char *s)
{
    (void)epic_serial_write((const uint8_t *)s, (int)strlen(s));
}

/**
 * @brief Tokenize a line in place into argv.
 *
 * Whitespace-delimited tokens are NUL-terminated in line and their
 * start pointers collected in argv, up to EPIC_CONSOLE_MAX_ARGS.
 *
 * @param line the NUL-terminated line to tokenize (modified in place)
 * @param argv array receiving the token start pointers
 * @return the number of tokens found
 */
static uint8_t epic_console_tokenize(char *line, char **argv)
{
    uint8_t argc = 0u;
    char *p = line;

    while (*p != '\0' && argc < EPIC_CONSOLE_MAX_ARGS) {
        while (*p == ' ' || *p == '\t') {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
        }

        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
    }

    while (*p != '\0') {
        if (*p == ' ' || *p == '\t') {
            *p = '\0';
        }
        p++;
    }

    return argc;
}

/**
 * @brief Tokenize the current line and dispatch it to the command table.
 *
 * Matches argv[0] against the table; an empty line is cleared without
 * dispatch. The line buffer is reset after dispatch.
 *
 * @param con the console whose line to dispatch
 */
static void epic_console_dispatch(epic_console_t *con)
{
    uint8_t argc;

    con->line[con->line_len] = '\0';
    argc = epic_console_tokenize(con->line, con->argv);
    if (argc == 0u) {
        con->line_len = 0u;
        return;
    }

    for (uint8_t i = 0; i < con->table_len; i++) {
        if (strcmp(con->argv[0], con->table[i].name) == 0) {
            if (con->table[i].handler != NULL) {
                con->table[i].handler(argc, con->argv, con->ctx);
            }
            break;
        }
    }

    con->line_len = 0u;
}

/**
 * @brief Initialize a console instance with a caller-owned command table.
 *
 * Stores the table, context, and line state. See epic_console.h for the
 * full contract.
 *
 * @param con the console instance to initialize
 * @param table the caller-owned command table
 * @param table_len number of rows in table
 * @param ctx opaque context passed to every handler
 */
void epic_console_init(epic_console_t *con, const epic_console_cmd_t *table,
                       uint8_t table_len, void *ctx)
{
    con->table = table;
    con->table_len = table_len;
    con->ctx = ctx;
    con->line_len = 0u;
    con->last_was_cr = false;
    if (EPIC_CONSOLE_LINE_MAX > 0u) {
        con->line[0] = '\0';
    }
}

/**
 * @brief Drain serial input, echo/edit it, and dispatch complete lines.
 *
 * See epic_console.h for the full contract.
 *
 * @param con the console instance to poll
 */
void epic_console_poll(epic_console_t *con)
{
    uint8_t ch;

    while (epic_serial_available() > 0) {
        if (epic_serial_read(&ch, 1) != 1) {
            break;
        }

        if (ch == '\r' || ch == '\n') {
            if (ch == '\n' && con->last_was_cr) {
                con->last_was_cr = false;
                continue;
            }

            epic_console_write_str("\r\n");
            epic_console_dispatch(con);
            con->last_was_cr = (ch == '\r');
            continue;
        }

        con->last_was_cr = false;

        if (ch == '\b' || ch == 0x7Fu) {
            if (con->line_len > 0u) {
                con->line_len--;
                con->line[con->line_len] = '\0';
                epic_console_write_str("\b \b");
            }
            continue;
        }

        if (con->line_len < (uint8_t)(EPIC_CONSOLE_LINE_MAX - 1u)) {
            con->line[con->line_len++] = (char)ch;
            con->line[con->line_len] = '\0';
            (void)epic_serial_write(&ch, 1);
        }
    }
}

/**
 * @brief Print one "name - help" line per command-table row.
 *
 * See epic_console.h for the full contract.
 *
 * @param con the console whose table to print
 */
void epic_console_print_help(const epic_console_t *con)
{
    for (uint8_t i = 0; i < con->table_len; i++) {
        epic_console_write_str(con->table[i].name);
        epic_console_write_str(" - ");
        epic_console_write_str(con->table[i].help != NULL ? con->table[i].help : "");
        epic_console_write_str("\r\n");
    }
}
