/* Cross-compile sizecheck for epic-console. */

#include "epic_console.h"

/**
 * @brief No-op command handler for the sizecheck table.
 *
 * @param argc number of tokens in argv
 * @param argv token pointers (unused)
 * @param ctx opaque context (unused)
 */
static void stub(uint8_t argc, char **argv, void *ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
}

/**
 * @brief Cross-compile sizecheck entry point.
 *
 * Instantiates a console with a one-row table and prints its help.
 *
 * @return 0 (always)
 */
int main(void)
{
    static const epic_console_cmd_t table[] = {
        { "ping", stub, "ping" },
    };
    epic_console_t con;
    EPIC_CONSOLE_INIT(&con, table, 0);
    epic_console_print_help(&con);
    return 0;
}
