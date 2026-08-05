/**
 * @file    epic_console.c
 * @brief   Line-based serial command dispatcher over epic-serial.
 */

#include "epic_console.h"
#include "epic_serial.h"

#include <stddef.h>
#include <string.h>

static void console_write_str(const char *s)
{
    (void)epic_serial_write((const uint8_t *)s, (int)strlen(s));
}

static uint8_t console_tokenize(char *line, char **argv)
{
    uint8_t argc = 0u;
    char *p = line;

    while (*p != '\0' && argc < PIC8_CONSOLE_MAX_ARGS) {
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

static void console_dispatch(epic_console_t *con)
{
    char *argv[PIC8_CONSOLE_MAX_ARGS];
    uint8_t argc;

    con->line[con->line_len] = '\0';
    argc = console_tokenize(con->line, argv);
    if (argc == 0u) {
        con->line_len = 0u;
        return;
    }

    for (uint8_t i = 0; i < con->table_len; i++) {
        if (strcmp(argv[0], con->table[i].name) == 0) {
            if (con->table[i].handler != NULL) {
                con->table[i].handler(argc, argv, con->ctx);
            }
            break;
        }
    }

    con->line_len = 0u;
}

void epic_console_init(epic_console_t *con, const epic_console_cmd_t *table,
                       uint8_t table_len, void *ctx)
{
    con->table = table;
    con->table_len = table_len;
    con->ctx = ctx;
    con->line_len = 0u;
    con->last_was_cr = false;
    if (PIC8_CONSOLE_LINE_MAX > 0u) {
        con->line[0] = '\0';
    }
}

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

            console_write_str("\r\n");
            console_dispatch(con);
            con->last_was_cr = (ch == '\r');
            continue;
        }

        con->last_was_cr = false;

        if (ch == '\b' || ch == 0x7Fu) {
            if (con->line_len > 0u) {
                con->line_len--;
                con->line[con->line_len] = '\0';
                console_write_str("\b \b");
            }
            continue;
        }

        if (con->line_len < (uint8_t)(PIC8_CONSOLE_LINE_MAX - 1u)) {
            con->line[con->line_len++] = (char)ch;
            con->line[con->line_len] = '\0';
            (void)epic_serial_write(&ch, 1);
        }
    }
}

void epic_console_print_help(const epic_console_t *con)
{
    for (uint8_t i = 0; i < con->table_len; i++) {
        console_write_str(con->table[i].name);
        console_write_str(" - ");
        console_write_str(con->table[i].help != NULL ? con->table[i].help : "");
        console_write_str("\r\n");
    }
}
