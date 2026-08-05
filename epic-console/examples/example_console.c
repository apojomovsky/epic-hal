/**
 * @file    example_console.c
 * @brief   Host-sim smoke for epic-console with a few injected commands.
 */

#include "epic_console.h"
#include "epic_serial.h"
#include "core/epic_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_RX(b)  pic18_sim_drive_usart_rx((uint8_t)(b))
  #define EXAMPLE_FOSC_HZ 48000000UL
#else
  #include "pic16f87xa_sim.h"
  #define SIM_RX(b)  pic16f87xa_sim_drive_usart_rx((uint8_t)(b))
  #define EXAMPLE_FOSC_HZ 20000000UL
#endif

#include <stdio.h>

typedef struct {
    uint8_t led_on;
    uint8_t status_count;
    epic_console_t *console;
} app_ctx_t;

static void cmd_led(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'n' && argv[1][2] == '\0') {
        ctx->led_on = 1u;
        epic_harness_log("led -> on\n");
    } else if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'f' && argv[1][2] == 'f' && argv[1][3] == '\0') {
        ctx->led_on = 0u;
        epic_harness_log("led -> off\n");
    }
}

static void cmd_status(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    ctx->status_count++;
    epic_harness_log("status: led=%u count=%u\n", ctx->led_on, ctx->status_count);
}

static void cmd_help(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    epic_console_print_help(ctx->console);
}

int main(void)
{
    epic_harness_init(1000000UL);
    epic_serial_init(EXAMPLE_FOSC_HZ, 9600u);

    app_ctx_t ctx = {0};
    static const epic_console_cmd_t table[] = {
        { "led",    cmd_led,    "led on|off" },
        { "status", cmd_status, "show current state" },
        { "help",   cmd_help,   "list commands" },
    };
    epic_console_t con;
    PIC8_CONSOLE_INIT(&con, table, &ctx);
    ctx.console = &con;

    const char *script = "status\rled on\rstatus\rhelp\r";
    while (*script != '\0') {
        SIM_RX((uint8_t)*script++);
    }
    epic_console_poll(&con);
    epic_serial_flush();

    return epic_harness_report(ctx.led_on == 1u && ctx.status_count == 2u);
}
