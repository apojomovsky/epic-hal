/*
 * epic-console target example: a line command dispatcher over the
 * 115200-baud UART. "help" prints the command table, "led on|off"
 * tracks LED state in memory and reports it, and "status" reports the
 * LED state and a status counter. Typed lines are echoed and edited
 * by epic-console and dispatched on Enter.
 */

#include "epic_console.h"
#include "epic_serial.h"
#include "core/hal_irq.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define BAUD     115200UL

typedef struct {
    uint8_t          led_on;
    uint8_t          status_count;
    epic_console_t  *console;
} app_ctx_t;

/** @brief Turn the demo LED state on or off. */
static void cmd_led(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'n' && argv[1][2] == '\0') {
        ctx->led_on = 1u;
        epic_serial_put_str("led -> on\r\n");
    } else if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'f' &&
               argv[1][2] == 'f' && argv[1][3] == '\0') {
        ctx->led_on = 0u;
        epic_serial_put_str("led -> off\r\n");
    } else {
        epic_serial_put_str("usage: led on|off\r\n");
    }
}

/** @brief Report the LED state and the status count. */
static void cmd_status(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    uint8_t c = ctx->status_count;
    c = (uint8_t)(c + 1u);
    ctx->status_count = c;
    epic_serial_put_str("status: led=");
    epic_serial_put_u16(ctx->led_on);
    epic_serial_put_str(" count=");
    epic_serial_put_u16(ctx->status_count);
    epic_serial_put_str("\r\n");
}

/** @brief Print the command table help. */
static void cmd_help(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    epic_console_print_help(ctx->console);
}

static epic_console_cmd_t table[3];

/**
 * @brief Console example: dispatch help/led/status over serial.
 */
int main(void)
{
    table[0].name = "led";    table[0].handler = cmd_led;    table[0].help = "led on|off";
    table[1].name = "status"; table[1].handler = cmd_status; table[1].help = "show led state and status count";
    table[2].name = "help";   table[2].handler = cmd_help;   table[2].help = "list commands";

    epic_serial_init(FOSC_HZ, BAUD);

    static app_ctx_t ctx;
    static epic_console_t con;
    ctx.led_on = 0u;
    ctx.status_count = 0u;
    EPIC_CONSOLE_INIT(&con, table, &ctx);
    ctx.console = &con;

    EPIC_IRQ_Restore(1);             /* UART RX/TX ISRs */
    epic_serial_put_str("epic-console example: type \"help\"\r\n");

    for (;;) {
        epic_console_poll(&con);
    }
}
