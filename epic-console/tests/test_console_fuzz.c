/**
 * @file    test_console_fuzz.c
 * @brief   Host fuzz test for epic-console's line state machine:
 *          random byte streams fed through epic_console_poll must never
 *          wedge the parser, must keep the line buffer in bounds, must
 *          hit the unknown-command error path without calling any
 *          handler, and must recover to a sane state (a sentinel
 *          "ping\r" line always dispatches, exactly as many times as
 *          the line model says it should). The echo stream is verified
 *          byte-exact against a model of the documented edit behavior
 *          (echo accepted chars, "\r\n" on a terminator, "\b \b" on
 *          backspace, CRLF suppression, silent drop past
 *          EPIC_CONSOLE_LINE_MAX-1).
 *
 *          Deterministic: fixed-seed LCG. One RX byte is injected and
 *          one poll runs per step, and the TX ring is drained after
 *          every poll, so the host sim's single-pop-per-step TX limit
 *          can never be exceeded (see test_serial_stress.c's header
 *          note for the same constraint).
 */

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

static uint32_t g_seed = 0xC0A50001u;
static uint32_t rnd(void)
{
    g_seed = (1664525u * g_seed + 1013904223u);
    return g_seed;
}

/* Command handler capture context. */
static int g_calls = 0;
static char g_last_cmd[EPIC_CONSOLE_LINE_MAX];

static void cmd_ping(uint8_t argc, char **argv, void *ctx)
{
    (void)argc;
    (void)ctx;
    g_calls++;
    strncpy(g_last_cmd, argv[0], sizeof(g_last_cmd) - 1u);
    g_last_cmd[sizeof(g_last_cmd) - 1u] = '\0';
}

/* Drain the TX ring (echo stream), capturing bytes in order. The
 * sim's free-running Timer0 can pop a TX byte inside the tick, so
 * capture after each pending-count drop (same pattern as
 * test_serial_stress.c). */
static int drain_tx(char *out, int max)
{
    int n = 0;
    while (epic_serial_tx_pending() > 0 && n < max) {
        int before = epic_serial_tx_pending();
        epic_harness_tick();
        int mid = epic_serial_tx_pending();
        if (mid < before) {
            out[n++] = (char)EPIC_REG8(PIC_REG_TXREG);
        }
        epic_dispatch_all_irqs();
        int after = epic_serial_tx_pending();
        if (after < mid) {
            out[n++] = (char)EPIC_REG8(PIC_REG_TXREG);
        }
    }
    epic_harness_tick();
    return n;
}

/* Model of the console's documented edit behavior for one byte, plus
 * dispatch accounting: a terminator on a line that reads exactly
 * "ping" calls the handler. */
typedef struct {
    char     line[EPIC_CONSOLE_LINE_MAX];
    uint8_t  line_len;
    uint8_t  last_was_cr;
    char     echo[8];
    uint8_t  echo_len;
    int      dispatches;
} line_model_t;

static void model_feed(line_model_t *m, uint8_t ch)
{
    m->echo_len = 0u;

    if (ch == '\r' || ch == '\n') {
        if (ch == '\n' && m->last_was_cr) {
            m->last_was_cr = 0u;
            return;   /* CRLF: second byte swallowed silently */
        }
        m->echo[0] = '\r';
        m->echo[1] = '\n';
        m->echo_len = 2u;
        if (strcmp(m->line, "ping") == 0) {
            m->dispatches++;
        }
        m->line_len = 0u;              /* line dispatched (or dropped) */
        m->line[0] = '\0';
        m->last_was_cr = (ch == '\r');
        return;
    }

    m->last_was_cr = 0u;

    if (ch == '\b' || ch == 0x7Fu) {
        if (m->line_len > 0u) {
            m->line_len--;
            m->line[m->line_len] = '\0';
            m->echo[0] = '\b';
            m->echo[1] = ' ';
            m->echo[2] = '\b';
            m->echo_len = 3u;
        }
        return;
    }

    if (m->line_len < (uint8_t)(EPIC_CONSOLE_LINE_MAX - 1u)) {
        m->line[m->line_len++] = (char)ch;
        m->line[m->line_len] = '\0';
        m->echo[0] = (char)ch;
        m->echo_len = 1u;
    }
    /* past line capacity: dropped silently */
}

int main(void)
{
    epic_harness_init(4000000UL);
    epic_serial_init(TEST_FOSC_HZ, 9600u);

    static const epic_console_cmd_t table[] = {
        { "ping", cmd_ping, "ping command" },
    };
    epic_console_t con;
    EPIC_CONSOLE_INIT(&con, table, NULL);

    line_model_t m;
    memset(&m, 0, sizeof(m));
    g_calls = 0;

    for (int it = 0; it < 6000; it++) {
        /* One byte per step. Every 64 steps the sentinel "ping\r" is
         * injected; the step before it, a '\r' flush clears any
         * pending fuzz line so the sentinel starts from a clean
         * parser state (an unknown pending command hits the error
         * path: no handler runs). */
        int phase = it % 64;
        uint8_t b;
        int check_sentinel = 0;
        if (phase == 30) {
            b = (uint8_t)'\r';      /* flush pending fuzz line */
        } else if (phase == 31) {
            g_calls = 0;
            m.dispatches = 0;
            b = (uint8_t)'p';
        } else if (phase >= 32 && phase < 36) {
            static const char sent[] = "ing\r";
            uint8_t idx = (uint8_t)(phase - 32);
            if (idx < (uint8_t)(sizeof(sent) / sizeof(sent[0]))) {
                b = (uint8_t)sent[idx];
            } else {
                b = (uint8_t)'\r';
            }
            if (phase == 35) check_sentinel = 1;
        } else {
            /* Bias toward control bytes so the state-machine edges
             * (terminators, backspaces, CRLF pairs) are hit hard. */
            uint32_t r = rnd();
            if ((r & 3u) == 0u) {
                static const uint8_t ctl[] = { '\r', '\n', '\b', 0x7Fu, 'a', ' ', 'p' };
                b = ctl[rnd() % (sizeof(ctl) / sizeof(ctl[0]))];
            } else {
                b = (uint8_t)(r >> 8);
            }
        }

        SIM_RX(b);
        epic_console_poll(&con);

        /* Echo model + actual echo drain. */
        model_feed(&m, b);
        char echo[16];
        int n = drain_tx(echo, (int)sizeof(echo));
        CHECK(n == (int)m.echo_len, "echo length matches model");
        if (n == (int)m.echo_len) {
            CHECK(memcmp(echo, m.echo, m.echo_len) == 0, "echo bytes match model");
        }

        /* Parser state invariants. */
        CHECK(con.line_len < EPIC_CONSOLE_LINE_MAX, "line_len stays in bounds");
        CHECK(m.line_len == con.line_len, "model line length matches parser");

        /* Sentinel accounting: the model knows how many dispatches
         * occurred since the last checkpoint; the parser must agree
         * and the last dispatched command must be "ping". */
        if (check_sentinel) {
            CHECK(g_calls == m.dispatches && m.dispatches == 1,
                  "sentinel dispatched exactly once");
            CHECK(strcmp(g_last_cmd, "ping") == 0, "dispatched command is ping");
        }
    }

    printf("test_console_fuzz: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
