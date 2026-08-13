/*
 * epic-console target example: a line command dispatcher over the
 * 115200-baud UART. "help" prints the command table, "led on|off"
 * drives the LED on GPIOB0, and "status" reports the LED state and a
 * status counter. Typed lines are echoed and edited by epic-console
 * and dispatched on Enter.
 */

#include "epic_console.h"
#include "epic_serial.h"
#include "epic_hal.h"                /* EPIC_GPIO_Init / EPIC_GPIO_WritePin */

#include <stdio.h>
#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define BAUD     115200UL
#define LED_PORT GPIOB
#define LED_PIN  GPIO_PIN_0

typedef struct {
    uint8_t          led_on;
    uint8_t          status_count;
    epic_console_t  *console;
} app_ctx_t;

/** @brief Turn the demo LED on or off. */
static void cmd_led(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'n' && argv[1][2] == '\0') {
        ctx->led_on = 1u;
        EPIC_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        printf("led -> on\r\n");
    } else if (argc >= 2u && argv[1][0] == 'o' && argv[1][1] == 'f' &&
               argv[1][2] == 'f' && argv[1][3] == '\0') {
        ctx->led_on = 0u;
        EPIC_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        printf("led -> off\r\n");
    } else {
        printf("usage: led on|off\r\n");
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
    printf("status: led=%u count=%u\r\n", ctx->led_on, ctx->status_count);
}

/** @brief Print the command table help. */
static void cmd_help(uint8_t argc, char **argv, void *ctx_)
{
    app_ctx_t *ctx = (app_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    epic_console_print_help(ctx->console);
}

/** @brief Line command dispatcher: help/led on/off/status over serial. */
int main(void)
{
    epic_serial_init(FOSC_HZ, BAUD);
    EPIC_GPIO_Init(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    static const epic_console_cmd_t table[] = {
        { "led",    cmd_led,    "led on|off" },
        { "status", cmd_status, "show led state and status count" },
        { "help",   cmd_help,   "list commands" },
    };

    static app_ctx_t ctx;
    static epic_console_t con;
    ctx.led_on = 0u;
    ctx.status_count = 0u;
    EPIC_CONSOLE_INIT(&con, table, &ctx);
    ctx.console = &con;

    EPIC_IRQ_Restore(1);             /* UART RX/TX ISRs */
    printf("epic-console example: type \"help\"\r\n");

    for (;;) {
        epic_console_poll(&con);
    }
}
