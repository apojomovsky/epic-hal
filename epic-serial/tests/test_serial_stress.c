/*
 * Host property test for epic-serial: randomized producer/consumer stress
 * through the real public API over the host sim's USART model. RX is
 * injected via *_sim_drive_usart_rx (which dispatches into the module's
 * RX ring), TX drained with epic_dispatch_all_irqs (the host equivalent
 * of the target's interrupt vector); byte-exact round trips are checked
 * against an independent model. Deterministic: fixed-seed LCG.
 *
 * Host loop-safety rule the test relies on: TXIF is re-asserted only by
 * the sim step, so the module's internal drain pops at most one byte
 * between steps; the test drains one step+dispatch pair per byte and
 * never starts a write with a full ring.
 */

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

#define SZ EPIC_SERIAL_RING_SZ

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

/* Fixed-seed LCG, same shape as epic-math's pic_math_test_rand. */
static uint32_t g_seed = 0x5E2D0001u;
static uint32_t rnd(void)
{
    g_seed = (1664525u * g_seed + 1013904223u);
    return g_seed;
}

/* Independent model of the RX ring, byte-exact FIFO. */
static uint8_t g_rx_model[SZ * 8u];
static size_t  g_rx_model_head = 0u;   /* next byte the reader expects */
static size_t  g_rx_model_len = 0u;    /* bytes not yet read */

/* Drain the TX ring: one step (re-asserts TXIF) + one dispatch (pops
 * one byte into TXREG) per byte, capturing each popped byte. Returns
 * the number of bytes captured.
 *
 * The sim's free-running Timer0 can overflow inside the step and fire
 * the IRQ callback before the explicit dispatch, popping a byte whose
 * TXREG contents the explicit dispatch would overwrite; the
 * pending-count deltas catch every pop in order. */
static int drain_tx(uint8_t *out, int max)
{
    int n = 0;
    while (epic_serial_tx_pending() > 0 && n < max) {
        int before = epic_serial_tx_pending();
        epic_harness_tick();
        int mid = epic_serial_tx_pending();
        if (mid < before) {
            out[n++] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);  /* tick popped */
        }
        epic_dispatch_all_irqs();
        int after = epic_serial_tx_pending();
        if (after < mid) {
            out[n++] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);  /* dispatch popped */
        }
    }
    /* Trailing step re-asserts TXIF for the next write's internal drain
     * (see the file header note). */
    epic_harness_tick();
    return n;
}

/* Inject one RX byte through the sim; the sim dispatches immediately. */
static void rx_inject(uint8_t b)
{
    SIM_RX(b);
}

static void rx_check_model(void)
{
    while (g_rx_model_len > 0u) {
        /* Varied read sizes, never more than the model holds. */
        int n = (int)(g_rx_model_len % 7u) + 1;
        if ((size_t)n > g_rx_model_len) n = (int)g_rx_model_len;
        uint8_t buf[16];
        int got = epic_serial_read(buf, n);
        if (got != n) {
            CHECK(0, "rx read returns requested count");
            break;
        }
        for (int i = 0; i < n; i++) {
            if (buf[i] != g_rx_model[g_rx_model_head + (size_t)i]) {
                CHECK(0, "rx byte mismatch vs model");
            }
        }
        g_rx_model_head += (size_t)n;
        g_rx_model_len  -= (size_t)n;
    }
    /* Compact so the linear model buffer never overflows. */
    if (g_rx_model_head > 0u) {
        memmove(g_rx_model, g_rx_model + g_rx_model_head, g_rx_model_len);
        g_rx_model_head = 0u;
    }
}

static void test_stress_roundtrip(void)
{
    for (int it = 0; it < 3000; it++) {
        /* TX producer: 0..SZ+1 bytes (SZ+1 exercises the blocking
         * boundary: the module drains one byte internally, then the
         * external drain finishes the ring). */
        int wlen = (int)(rnd() % (SZ + 2u));
        uint8_t wbuf[SZ + 2u];
        memset(wbuf, 0, sizeof(wbuf));
        for (int i = 0; i < wlen; i++) {
            wbuf[i] = (uint8_t)rnd();
        }
        int w = epic_serial_write(wbuf, wlen);
        CHECK(w == wlen, "tx write returns full length");
        /* A write longer than the ring blocks and the module's internal
         * drain pops the oldest byte into TXREG; capture it before the
         * external drain overwrites TXREG with the next byte. */
        int pre = 0;
        uint8_t tbuf[SZ + 2u];
        if (wlen > (int)SZ) {
            tbuf[0] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);
            CHECK(tbuf[0] == wbuf[0], "tx internal drain pops oldest byte");
            pre = 1;
        }
        int n = drain_tx(tbuf + pre, (int)(SZ + 2u) - pre);
        CHECK(n + pre == wlen, "tx stream captured exactly what was written");
        CHECK(memcmp(tbuf, wbuf, (size_t)wlen) == 0, "tx stream byte-exact");

        /* RX producer: 0..4 random bytes, injected one at a time. */
        int rlen = (int)(rnd() % 5u);
        for (int i = 0; i < rlen; i++) {
            uint8_t b = (uint8_t)rnd();
            if (g_rx_model_len >= SZ) {
                CHECK(epic_serial_available() == (int)SZ,
                      "rx ring full reported");
                continue;   /* overflow drop is the documented contract */
            }
            rx_inject(b);
            g_rx_model[g_rx_model_head + g_rx_model_len] = b;
            g_rx_model_len++;
        }
        CHECK(epic_serial_available() == (int)g_rx_model_len,
              "rx available matches model");

        /* RX consumer: drain the model whenever it holds bytes. */
        rx_check_model();
        CHECK(epic_serial_available() == 0, "rx empty after full drain");
    }

    /* Final: the TX ring must be empty at the end. */
    CHECK(epic_serial_tx_pending() == 0, "tx ring empty at end");
}

static void test_full_boundaries(void)
{
    /* TX ring full: write exactly SZ bytes with no drain in between,
     * then a single extra byte (the blocking boundary). On the host
     * the block must be exercised one byte at a time: the module's
     * internal drain can pop only once per sim step (see the file
     * header note), so the blocking write is a one-byte write with
     * TXIF re-asserted first. */
    uint8_t wbuf[SZ + 1u];
    for (int i = 0; i < (int)sizeof(wbuf); i++) {
        wbuf[i] = (uint8_t)(i * 7u + 1u);
    }
    CHECK(epic_serial_write(wbuf, SZ) == (int)SZ, "tx fill: write SZ");
    CHECK(epic_serial_tx_pending() == (int)SZ, "tx fill: ring full");
    epic_harness_tick();   /* re-assert TXIF so the block's drain pops */
    CHECK(epic_serial_write(wbuf + SZ, 1) == 1,
          "tx fill: blocking write of one byte completes");
    CHECK((uint8_t)EPIC_REG8(PIC_REG_TXREG) == wbuf[0],
          "tx fill: blocked write drained oldest byte first");
    uint8_t tbuf[SZ + 1u];
    tbuf[0] = (uint8_t)EPIC_REG8(PIC_REG_TXREG);
    int n = drain_tx(tbuf + 1, (int)SZ);
    CHECK(n + 1 == (int)(SZ + 1u), "tx fill: all bytes captured");
    CHECK(memcmp(tbuf, wbuf, sizeof(wbuf)) == 0, "tx fill: byte-exact order");

    /* RX ring full: inject SZ bytes, then one more which must drop. */
    g_rx_model_head = 0u;
    g_rx_model_len = 0u;
    uint8_t rbuf[SZ + 1u];
    for (int i = 0; i < (int)sizeof(rbuf); i++) {
        rbuf[i] = (uint8_t)(i * 13u + 3u);
    }
    for (int i = 0; i < SZ; i++) {
        rx_inject(rbuf[i]);
    }
    CHECK(epic_serial_available() == (int)SZ, "rx fill: ring full");
    rx_inject(rbuf[SZ]);   /* must be dropped */
    CHECK(epic_serial_available() == (int)SZ, "rx fill: overflow byte dropped");
    int got_rx = epic_serial_read(rbuf, (int)sizeof(rbuf));
    CHECK(got_rx == SZ, "rx fill: reads exactly the ring capacity");
    int ok = 1;
    for (int i = 0; i < got_rx; i++) {
        if (rbuf[i] != (uint8_t)(i * 13u + 3u)) ok = 0;
    }
    CHECK(ok, "rx fill: first SZ bytes byte-exact");

    /* Empty contracts. */
    CHECK(epic_serial_available() == 0, "empty: available == 0");
    CHECK(epic_serial_read(rbuf, 8) == 0, "empty: read returns 0");
    CHECK(epic_serial_tx_pending() == 0, "empty: tx_pending == 0");
    epic_serial_flush();
    CHECK(epic_serial_tx_pending() == 0, "empty: flush returns");

    /* Wrap-around: several fill/read cycles force head and tail to
     * wrap the ring many times. */
    uint8_t pat[20];
    for (int cyc = 0; cyc < 40; cyc++) {
        for (int i = 0; i < 20; i++) {
            pat[i] = (uint8_t)(rnd() & 0xFFu);
        }
        for (int i = 0; i < 20; i++) {
            rx_inject(pat[i]);
        }
        int got = epic_serial_read(pat, 20);
        CHECK(got == 20, "wrap: full cycle read");
        /* Byte-exactness was already proven per byte above; here just
         * verify the ring stays coherent (available == 0). */
        CHECK(epic_serial_available() == 0, "wrap: ring empty after cycle");
    }
}

int main(void)
{
    epic_harness_init(4000000UL);
    epic_serial_init(TEST_FOSC_HZ, 9600u);

    test_stress_roundtrip();
    test_full_boundaries();

    printf("test_serial_stress: %d passed, %d failed\n", g_pass, g_fail);
    return epic_harness_report(g_fail == 0);
}
