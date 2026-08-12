/* Host tests for epic-console over injected serial RX bytes. */

#include "epic_console.h"
#include "epic_serial.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_RX(b)  pic18_sim_drive_usart_rx((uint8_t)(b))
  #define TEST_FOSC_HZ 48000000UL
#else
  #include "pic16f87xa_sim.h"
  #define SIM_RX(b)  pic16f87xa_sim_drive_usart_rx((uint8_t)(b))
  #define TEST_FOSC_HZ 20000000UL
#endif

#include <stdio.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { \
            g_pass++; \
        } else { \
            printf("FAIL: %s\n", msg); \
            g_fail++; \
        } \
    } while (0)

typedef struct {
    int call_count;
    uint8_t argc;
    char args[8][64];
    int help_count;
} epic_console_ctx_t;

/**
 * @brief Copy the dispatched tokens into the test context.
 */
static void copy_args(epic_console_ctx_t *ctx, uint8_t argc, char **argv)
{
    ctx->argc = argc;
    for (uint8_t i = 0; i < argc && i < 8u; i++) {
        strncpy(ctx->args[i], argv[i], sizeof(ctx->args[i]) - 1u);
        ctx->args[i][sizeof(ctx->args[i]) - 1u] = '\0';
    }
}

/**
 * @brief Handler that records each dispatch into the test context.
 */
static void cmd_capture(uint8_t argc, char **argv, void *ctx_)
{
    epic_console_ctx_t *ctx = (epic_console_ctx_t *)ctx_;
    ctx->call_count++;
    copy_args(ctx, argc, argv);
}

/**
 * @brief Zero the test context.
 */
static void reset_ctx(epic_console_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

/**
 * @brief Reinitialize the harness and serial layers for a test.
 */
static void reset_env(void)
{
    epic_harness_init(1000000UL);
    epic_serial_init(TEST_FOSC_HZ, 9600u);
}

/**
 * @brief Inject a string of RX bytes without polling.
 */
static void drive_input(const char *s)
{
    while (*s != '\0') {
        SIM_RX((uint8_t)*s++);
    }
}

/**
 * @brief Inject RX bytes one at a time, polling after each.
 */
static void drive_input_polled(epic_console_t *con, const char *s)
{
    while (*s != '\0') {
        SIM_RX((uint8_t)*s++);
        epic_console_poll(con);
    }
}

/**
 * @brief Drain the TX ring into out, capturing bytes.
 *
 * @param out buffer receiving the captured bytes
 * @param max capacity of out
 * @return the number of bytes captured
 */
static int drain_tx(char *out, int max)
{
    int n = 0;
    epic_harness_tick();
    while (epic_serial_tx_pending() > 0 && n < max) {
        epic_dispatch_all_irqs();
        out[n++] = (char)EPIC_REG8(PIC_REG_TXREG);
        epic_harness_tick();
    }
    return n;
}

/**
 * @brief Verify dispatch of a no-arg command line.
 */
static void test_no_arg_dispatch(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "status", cmd_capture, "show status" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("status\r");
    epic_console_poll(&con);

    CHECK(ctx.call_count == 1, "no-arg: handler called once");
    CHECK(ctx.argc == 1u, "no-arg: argc == 1");
    CHECK(strcmp(ctx.args[0], "status") == 0, "no-arg: argv[0] is command");
}

/**
 * @brief Verify multi-arg tokenization with extra whitespace.
 */
static void test_multi_arg_tokenization(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "set", cmd_capture, "set value" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("set   mode   12\r");
    epic_console_poll(&con);

    CHECK(ctx.call_count == 1, "argv: handler called");
    CHECK(ctx.argc == 3u, "argv: argc == 3");
    CHECK(strcmp(ctx.args[0], "set") == 0, "argv: argv0");
    CHECK(strcmp(ctx.args[1], "mode") == 0, "argv: argv1");
    CHECK(strcmp(ctx.args[2], "12") == 0, "argv: argv2");
}

/**
 * @brief Verify backspace edits the line and echoes the erase sequence.
 */
static void test_backspace_behavior(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    char tx[64] = {0};
    static const epic_console_cmd_t table[] = {
        { "ping", cmd_capture, "ping command" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("pign\b\bng\r");
    epic_console_poll(&con);
    int n = drain_tx(tx, (int)sizeof(tx));

    CHECK(ctx.call_count == 1, "backspace: handler called");
    CHECK(strcmp(ctx.args[0], "ping") == 0, "backspace: final token reflects edits");
    CHECK(n >= 8, "backspace: produced echo output");
    CHECK(strstr(tx, "\b \b") != NULL, "backspace: terminal erase sequence emitted");
}

/**
 * @brief Verify backspace on an empty line is a safe no-op.
 */
static void test_backspace_at_empty_line(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    char tx[32] = {0};
    static const epic_console_cmd_t table[] = {
        { "x", cmd_capture, "unused" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("\b\r");
    epic_console_poll(&con);
    int n = drain_tx(tx, (int)sizeof(tx));

    CHECK(ctx.call_count == 0, "empty backspace: no handler");
    CHECK(n == 2 && tx[0] == '\r' && tx[1] == '\n', "empty backspace: only terminator echoed");
}

/**
 * @brief Verify an overlong line truncates without crashing.
 */
static void test_overlong_line_truncates_without_crashing(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "abcdefghijklmnopqrstuvwxyz12345", cmd_capture, "long command" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input_polled(&con, "abcdefghijklmnopqrstuvwxyz12345ZZZZ\r");

    CHECK(ctx.call_count == 1, "overflow: handler still called for truncated line");
    CHECK(strlen(ctx.args[0]) == (size_t)(EPIC_CONSOLE_LINE_MAX - 1u),
          "overflow: line truncated to max payload");
    CHECK(strncmp(ctx.args[0], "abcdefghijklmnopqrstuvwxyz12345ZZZZ", strlen(ctx.args[0])) == 0,
          "overflow: line preserves the input prefix that fit");
}

/**
 * @brief Verify CRLF is a single line terminator.
 */
static void test_crlf_is_one_terminator(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "go", cmd_capture, "run" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("go\r\n");
    epic_console_poll(&con);

    CHECK(ctx.call_count == 1, "crlf: one dispatch only");
}

/**
 * @brief Verify an unknown command is ignored.
 */
static void test_unknown_command_ignored(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "known", cmd_capture, "known command" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    drive_input("unknown\r");
    epic_console_poll(&con);

    CHECK(ctx.call_count == 0, "unknown: no handler called");
}

/**
 * @brief Verify EPIC_CONSOLE_INIT computes the table length.
 */
static void test_init_macro_table_len(void)
{
    reset_env();

    epic_console_ctx_t ctx;
    epic_console_t con;
    static const epic_console_cmd_t table[] = {
        { "a", cmd_capture, "A" },
        { "b", cmd_capture, "B" },
        { "c", cmd_capture, "C" },
    };
    reset_ctx(&ctx);
    EPIC_CONSOLE_INIT(&con, table, &ctx);

    CHECK(con.table_len == 3u, "init macro: table_len matches array size");
}

/**
 * @brief Run all epic-console host tests.
 *
 * @return 0 when all tests pass, 1 otherwise
 */
int main(void)
{
    test_no_arg_dispatch();
    test_multi_arg_tokenization();
    test_backspace_behavior();
    test_backspace_at_empty_line();
    test_overlong_line_truncates_without_crashing();
    test_crlf_is_one_terminator();
    test_unknown_command_ignored();
    test_init_macro_table_len();

    printf("test_console: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
