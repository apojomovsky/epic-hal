/**
 * @file    sim_console.c
 * @brief   Bounded, self-reporting HARNESS=sim build for epic-console:
 *          the module's first real `mdb` gate (18F4550/MPLAB SIM).
 *
 * @details
 *   TX-side / framing / internal-state gate, the documented shape for
 *   RX-bound modules (same constraint as epic-swuart's sim build).
 *   MPLAB SIM cannot inject UART RX data into this program's run:
 *   firmware writes to RCREG are ignored, firmware writes to the RCIF
 *   flag are masked, and mdb's own `set PIR1` / `set RCREG` are
 *   ignored as well (all three verified by probe against this exact
 *   .hex: the simulator's EUSART model owns those registers). The one
 *   sanctioned injection path (SCL `stim` with `packetin(..., RCREG,
 *   true)`, baud-paced) is unusable here: the fixed sim-mdb-run.sh
 *   mdb script has no slot to load a stimulus file before `run` (its
 *   only hook, extra_mdb, runs after `halt`), and this mdb version's
 *   `stim` command fails to parse even the SCL guide's minimal
 *   testbench, crashing the simulator. So the gate exercises the
 *   module headlessly on its output side, mirroring the host tests
 *   where the platform allows:
 *
 *   (a) init contract: EPIC_CONSOLE_INIT wires table/table_len/ctx and
 *       leaves the line buffer empty (line_len == 0, line[0] == 0,
 *       last_was_cr == false).
 *   (b) help framing: epic_console_print_help emits exactly one
 *       "name - help\r\n" line per table row (including the NULL-help
 *       fallback) through the real epic-serial TX ring; the gate
 *       counts every byte that leaves the ring through the real TX
 *       ISR (epic_dispatch_all_irqs, the same pump the host harness
 *       uses), paced so each byte is shifted out before the next
 *       (polling TRMT, exactly like the harness's own polled TX), and
 *       compares the total against the documented framing. The
 *       uart1io capture in the mdb log shows the emitted bytes.
 *   (c) empty-table edge: a zero-row console prints nothing.
 *   (d) idle poll: epic_console_poll with an empty RX ring is a no-op
 *       (no echo, no dispatch, line buffer untouched).
 *
 *   The RX-bound parser contracts (known-command dispatch, unknown
 *   command hitting the no-match error path, argument tokenization,
 *   backspace editing, CR/LF coalescing) stay covered by the host
 *   tests (tests/test_console.c), which feed the RX ring through
 *   pic18_sim_drive_usart_rx.
 */

#include "epic_console.h"
#include "epic_serial.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#include <string.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  All gate work happens before the loop; the bound only lets the
 *  simulator idle a moment before the report TX, mirroring every other
 *  family's .sim build. */
#define SIM_ITERATIONS 1000UL

/** Outer guard on the paced TX drain: one pop per in-flight byte, 74
 *  bytes in the gate's own help table plus slack. */
#define TX_POP_GUARD 200UL
/** Inner guard on the per-byte TRMT wait: a 9600-baud byte takes ~5000
 *  cycles to shift, each spin iteration a few instructions, so 100000
 *  is a comfortable ceiling that fails loudly instead of hanging. */
#define TX_TRMT_GUARD 100000UL

typedef struct {
    uint8_t status_count;
    uint8_t set_argc;
    uint8_t ping_count;
    uint8_t help_count;
} con_ctx_t;

/* Handlers mirror the module's example table. They are never invoked
 * under MPLAB SIM (no RX injection, see file header); they exist so
 * the table is the real shape a firmware would wire, and the help
 * framing gate walks their name/help rows for real. */
static void cmd_status(uint8_t argc, char **argv, void *ctx_)
{
    con_ctx_t *ctx = (con_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    ctx->status_count++;
}

static void cmd_set(uint8_t argc, char **argv, void *ctx_)
{
    con_ctx_t *ctx = (con_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    ctx->set_argc = argc;
}

static void cmd_ping(uint8_t argc, char **argv, void *ctx_)
{
    con_ctx_t *ctx = (con_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    ctx->ping_count++;
}

static void cmd_help(uint8_t argc, char **argv, void *ctx_)
{
    con_ctx_t *ctx = (con_ctx_t *)ctx_;
    (void)argc;
    (void)argv;
    ctx->help_count++;
}

static uint16_t g_tx_pops;
static uint8_t g_drain_failed;

/** Oracle for the documented print_help framing: one
 *  "name - help\r\n" line per row, NULL help rendered as "". */
static uint8_t help_len(const epic_console_t *con)
{
    uint8_t n = 0u;
    for (uint8_t i = 0u; i < con->table_len; i++) {
        const char *h = con->table[i].help != NULL ? con->table[i].help : "";
        n = (uint8_t)(n + (uint8_t)strlen(con->table[i].name) + 3u +
                      (uint8_t)strlen(h) + 2u);
    }
    return n;
}

/** Paced drain of the epic-serial TX ring through the real TX ISR
 *  entry: wait for the shift register to empty (so the byte in TXREG
 *  has fully left), then pop one ring byte (which loads TXREG), and
 *  repeat. Same per-byte pacing the harness's own polled report TX
 *  uses, so the uart1io capture receives every byte intact. */
static void drain_tx(void)
{
    uint32_t outer = 0UL;
    while (epic_serial_tx_pending() > 0 && outer < TX_POP_GUARD) {
        uint32_t guard = 0UL;
        while (!EPIC_USART_IsTxShiftRegisterEmpty() &&
               guard < TX_TRMT_GUARD) {
            guard++;
        }
        if (guard >= TX_TRMT_GUARD) {
            g_drain_failed = 1u;
            epic_harness_log("console sim: TX shift wait timeout\n");
            return;
        }
        epic_dispatch_all_irqs();
        g_tx_pops++;
        outer++;
    }
    if (epic_serial_tx_pending() > 0) {
        g_drain_failed = 1u;
        epic_harness_log("console sim: TX drain timeout\n");
    }
}

static void log_u16(uint16_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[5];
    c[0] = hx[(v >> 12) & 0xFu];
    c[1] = hx[(v >> 8) & 0xFu];
    c[2] = hx[(v >> 4) & 0xFu];
    c[3] = hx[v & 0xFu];
    c[4] = '\0';
    epic_harness_log(c);
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    epic_serial_init(FOSC_HZ, 9600u);

    con_ctx_t ctx = {0};

    int ok = 1;

    /* Phase 1: exact framing oracle. This table's framed help output is
     * exactly EPIC_SERIAL_RING_SZ (32) bytes, so print_help never hits
     * the ring-full blocking interlock: every byte that leaves the ring
     * is popped by the paced drain below and counted, and the count
     * must equal the documented framing length exactly. */
    {
        static const epic_console_cmd_t t_small[] = {
            { "status", cmd_status, "st" },   /* 6 + 3 + 2 + 2 = 13 */
            { "set",    cmd_set,    "sv" },   /* 3 + 3 + 2 + 2 = 10 */
            { "ping",   cmd_ping,   NULL },   /* 4 + 3 + 0 + 2 =  9 */
        };                                    /* total = 32         */
        epic_console_t con1;
        EPIC_CONSOLE_INIT(&con1, t_small, &ctx);

        /* (a) init contract. */
        ok = ok && (con1.table == t_small);
        ok = ok && (con1.table_len == 3u);
        ok = ok && (con1.ctx == &ctx);
        ok = ok && (con1.line_len == 0u);
        ok = ok && (con1.line[0] == '\0');
        ok = ok && (con1.last_was_cr == false);
        if (!(con1.table == t_small && con1.table_len == 3u &&
              con1.ctx == &ctx && con1.line_len == 0u &&
              con1.line[0] == '\0' && con1.last_was_cr == false)) {
            epic_harness_log("console sim: init contract FAIL\n");
        } else {
            epic_harness_log("console sim: init contract ok\n");
        }

        /* (d) idle poll: empty RX ring, nothing echoed or dispatched. */
        epic_console_poll(&con1);
        ok = ok && (con1.line_len == 0u);
        ok = ok && (epic_serial_tx_pending() == 0);

        /* (b) exact help framing through the real TX path. The output
         * fits the ring exactly, so print_help never hits the blocking
         * interlock: the ring must hold all 32 bytes afterwards and
         * the paced drain must pop all 32. */
        uint8_t expected = help_len(&con1);
        g_tx_pops = 0u;
        epic_console_print_help(&con1);
        uint8_t pending = (uint8_t)epic_serial_tx_pending();
        drain_tx();
        ok = ok && (g_drain_failed == 0u);
        ok = ok && (expected == 32u);
        ok = ok && (pending == expected);
        ok = ok && (g_tx_pops == expected);
        if (!(g_drain_failed == 0u && expected == 32u &&
              pending == expected && g_tx_pops == expected)) {
            epic_harness_log("console sim: help framing FAIL\n");
        } else {
            epic_harness_log("console sim: help framing ok\n");
        }
    }

    /* Phase 2: realistic table whose framed output (74 bytes) exceeds
     * the ring size, exercising the ring-full blocking interlock inside
     * epic_serial_write; verify the ring never wedges and the paced
     * drain empties it completely. The uart1io capture shows all four
     * lines. */
    {
        static const epic_console_cmd_t table[] = {
            { "status", cmd_status, "show status" },
            { "set",    cmd_set,    "set key value" },
            { "ping",   cmd_ping,   NULL },   /* NULL-help fallback row */
            { "help",   cmd_help,   "list commands" },
        };
        epic_console_t con;
        EPIC_CONSOLE_INIT(&con, table, &ctx);

        epic_console_print_help(&con);
        drain_tx();
        ok = ok && (g_drain_failed == 0u);
        if (g_drain_failed != 0u) {
            epic_harness_log("console sim: drain FAIL (2nd)\n");
        }

        /* (c) empty table: zero rows print nothing. */
        {
            epic_console_t con0;
            epic_console_init(&con0, NULL, 0u, 0);
            g_tx_pops = 0u;
            epic_console_print_help(&con0);
            ok = ok && (epic_serial_tx_pending() == 0);
            ok = ok && (g_tx_pops == 0u);
            if (epic_serial_tx_pending() != 0 || g_tx_pops != 0u) {
                epic_harness_log("console sim: empty table emitted bytes\n");
            }
        }
    }

    if (ok) {
        epic_harness_log("console sim: all checks passed\n");
    } else {
        epic_harness_log("console sim: one or more checks FAILED\n");
    }
    epic_harness_log("console sim: ring_sz=");
    log_u16(EPIC_SERIAL_RING_SZ);
    epic_harness_log("\n");

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }
    return epic_harness_report(ok);
}
