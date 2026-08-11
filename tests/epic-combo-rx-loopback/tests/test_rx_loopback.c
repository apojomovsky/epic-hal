/**
 * Host-sim test for the RX-loopback harness firmware
 * (combo_rx_loopback.c): drives the SAME firmware logic through the
 * host sim's USART RX-injection hook. MPLAB SIM cannot inject RX, so
 * the echo path never fires under mdb; the host sim CAN:
 * pic16f87xa_sim_drive_usart_rx() stores the
 * byte in RCREG, sets RCIF and fires the sim IRQ callback, which
 * epic_harness_init wires to epic_dispatch_all_irqs, the same
 * dispatch the real interrupt vector runs. The TX helper is the
 * build seam: this test provides its own rx_loopback_tx() byte
 * recorder, so every transmission (boot banner included) is captured
 * byte-exact.
 */

#include "core/epic_harness.h"
#include "epic_hal.h"
#include "pic16f87xa_sim.h"

#include <stdio.h>
#include <string.h>

/* The firmware's shared entry points (combo_rx_loopback.c). */
void rx_loopback_init(void);

static uint8_t g_tx[256];
static size_t  g_tx_len;

void rx_loopback_tx(uint8_t b)
{
    if (g_tx_len < sizeof(g_tx)) {
        g_tx[g_tx_len++] = b;
    }
}

static void tx_reset(void)
{
    g_tx_len = 0u;
}

static int g_fail = 0;

static void check_tx(const char *expected, const char *what)
{
    size_t n = strlen(expected);
    if (g_tx_len != n || memcmp(g_tx, expected, n) != 0) {
        printf("FAIL: %s (got %u bytes, want %u)\n", what,
               (unsigned)g_tx_len, (unsigned)n);
        g_fail++;
    }
}

/** Inject one line byte by byte through the sim hook (each injection
 *  dispatches synchronously); a couple of ticks between bytes mirrors
 *  wire timing. */
static void inject_line(const char *line)
{
    while (*line) {
        pic16f87xa_sim_drive_usart_rx((uint8_t)*line);
        line++;
        epic_harness_tick();
        epic_harness_tick();
    }
}

int main(void)
{
    epic_harness_init(100000UL);

    /* Boot banner: the firmware's own init (USART init + banner). */
    rx_loopback_init();
    check_tx("RXLOOP UP\r\n", "boot banner");
    tx_reset();

    /* GIE on, mirroring the firmware's main(). */
    EPIC_IRQ_Restore(1);

    /* Framing vectors: input line -> expected echo, per the protocol
     * documented in combo_rx_loopback.c's header. */
    static const struct {
        const char *in;
        const char *expect;
    } cases[] = {
        { "hello\r\n",                          "OK:hello\r\n" },
        { "x\r\n",                              "OK:x\r\n" },
        /* Empty line: "\r\n" is a well-formed (empty payload) frame. */
        { "\r\n",                               "OK:\r\n" },
        /* Lone CR inside a line is payload, only LF terminates. */
        { "q\rw\r\n",                           "OK:q\rw\r\n" },
        /* Bad terminator: LF without the CR. */
        { "abc\n",                              "ERR:abc\r\n" },
        /* Bare LF: empty payload, no CR. */
        { "\n",                                 "ERR:\r\n" },
        /* CR-only "terminator" is not a terminator: the frame only
         * ends at the LF, and the trailing byte is 'b', not CR. */
        { "a\rb\n",                             "ERR:a\rb\r\n" },
        /* Over-long: 40 payload bytes; the first 32 are echoed, the
         * rest discarded until the LF. */
        { "0123456789abcdef0123456789abcdef01234567\n",
          "ERR:0123456789abcdef0123456789abcdef\r\n" },
    };

    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char label[64];
        snprintf(label, sizeof(label), "vector %u", (unsigned)i);
        tx_reset();
        inject_line(cases[i].in);
        check_tx(cases[i].expect, label);
    }

    /* The line state resets after every frame: a fresh well-formed
     * line right after the over-long one still echoes OK. */
    tx_reset();
    inject_line("tail\r\n");
    check_tx("OK:tail\r\n", "state reset after overflow");

    if (g_fail == 0) {
        printf("rx_loopback host test: PASS\n");
    }
    return g_fail == 0 ? 0 : 1;
}
