/* Cross-compile sizecheck for epic-console. */

#include "epic_console.h"

static void stub(uint8_t argc, char **argv, void *ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
}

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
