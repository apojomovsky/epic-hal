/**
 * @file    test_swuart_fuzz.c
 * @brief   Host property test for epic-swuart's ring + error-count
 *          invariants under randomized writes/reads, driven through
 *          the same test hooks as test_swuart_tx.c/test_swuart_rx.c
 *          (no real CCP hardware in the host sim): TX bytes are fired
 *          through compare events and decoded from the armed CCP mode
 *          sequence, RX bytes are injected pin-level and read back,
 *          byte-exact, against the model. Also covers the TX short-
 *          write boundary (never blocks, ring capacity respected),
 *          the RX ring-full drop, and the bad-stop-bit error path.
 *
 *          Deterministic: fixed-seed LCG. Channel A only, so the test
 *          compiles and runs on every family (the RX start sequence
 *          differs: one fire on the PIC16F87XA fast path, two on the
 *          generic paths).
 */

#include <stdio.h>
#include "epic_swuart.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Family dispatch for the sim's drive_input, same shape as
 * test_swuart_rx.c (PIC16F193X's sim only stages the level and needs
 * a step to refresh the PORT register). */
#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) \
      do { pic16f193x_sim_drive_input((port), (pin), (lvl)); pic16f193x_sim_step(1); } while (0)
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
#endif

/* Test-only hooks (defined in epic_swuart.c behind
 * EPIC_SWUART_TEST_HOOKS, enabled for this library build by
 * epic-swuart/CMakeLists.txt). */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
extern void swuart_test_fire_tx_event(void);
extern void swuart_test_fire_rx_event(void);
extern uint8_t swuart_test_last_tx_mode(void);
#if EPIC_SWUART_HAS_RX_FAST_PATH
extern void swuart_test_set_capture_fast(uint16_t value);
#else
extern void swuart_test_set_capture(uint16_t value);
#endif

static uint32_t g_seed = 0x5EED0001u;
static uint32_t rnd(void)
{
    g_seed = (1664525u * g_seed + 1013904223u);
    return g_seed;
}

/* Queue `len` bytes and transmit them by firing compare events,
 * decoding each byte from the armed CCP mode sequence (data bits LSB
 * first: SET = 1, CLEAR = 0; one stop event per byte). */
static void tx_send_decode(EPIC_SWUART_HandleTypeDef *h,
                           const uint8_t *data, size_t len)
{
    size_t queued = EPIC_SWUART_Write(h, data, len);
    CHECK(queued == len, "tx queued fully (ring had room)");

    for (size_t b = 0; b < len; b++) {
        /* Byte 0's start bit was armed by Write() itself; each later
         * byte needs one TX_IDLE pop event first (that event arms the
         * start bit, mode CLEAR, and is not a data bit). */
        if (b > 0u) {
            swuart_test_fire_tx_event();
        }
        uint8_t got = 0u;
        for (int k = 1; k <= 8; k++) {
            swuart_test_fire_tx_event();
            if (swuart_test_last_tx_mode() == (uint8_t)CCP_MODE_COMPARE_SET) {
                got |= (uint8_t)(1u << (k - 1));
            }
        }
        swuart_test_fire_tx_event();   /* stop bit */
        if (got != data[b]) {
            CHECK(0, "tx byte decoded from mode sequence");
        }
    }
    CHECK(h->tx_count == 0u, "tx ring drained");
    CHECK(EPIC_SWUART_GetErrorCount(h) == 0u, "tx produced no errors");
}

/* Inject one byte on the RX pin and drive the state machine to a
 * complete frame. Returns the number of events fired. */
static unsigned rx_send_byte(EPIC_SWUART_HandleTypeDef *h, uint8_t byte)
{
    uint8_t bits[10];
    bits[0] = 0u;   /* start */
    for (int i = 0; i < 8; i++) {
        bits[1 + i] = (uint8_t)((byte >> i) & 1u);
    }
    bits[9] = 1u;   /* stop */

    /* Note: the sim drive API takes the port LETTER and pin INDEX,
     * while the HAL handle stores the GPIO enum (0-based port) and a
     * pin bitmask; test_swuart_rx.c sidesteps the mismatch with
     * literals, so do the same: channel A's RX pin is RC2 everywhere. */
    SIM_DRIVE('C', 2u, bits[0]);
#if EPIC_SWUART_HAS_RX_FAST_PATH
    swuart_test_fire_rx_event();   /* IDLE -> DATA0 (deglitch: pin low) */
#else
    swuart_test_set_capture(1000u);
    swuart_test_fire_rx_event();   /* capture: IDLE -> CONFIRM_START */
    swuart_test_fire_rx_event();   /* confirm: pin still low -> DATA0 */
#endif
    for (int i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2u, bits[i]);
        swuart_test_fire_rx_event();
    }
#if EPIC_SWUART_HAS_RX_FAST_PATH
    return 10u;
#else
    return 11u;
#endif
}

/* Same frame shape but with the stop bit driven low: the byte must be
 * dropped and the error count bumped. */
static void rx_send_bad_stop(EPIC_SWUART_HandleTypeDef *h, uint8_t byte)
{
    uint8_t bits[10];
    bits[0] = 0u;
    for (int i = 0; i < 8; i++) {
        bits[1 + i] = (uint8_t)((byte >> i) & 1u);
    }
    bits[9] = 0u;   /* bad stop */

    SIM_DRIVE('C', 2u, bits[0]);
#if EPIC_SWUART_HAS_RX_FAST_PATH
    swuart_test_fire_rx_event();
#else
    swuart_test_set_capture(1000u);
    swuart_test_fire_rx_event();
    swuart_test_fire_rx_event();
#endif
    for (int i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2u, bits[i]);
        swuart_test_fire_rx_event();
    }
}

static void test_tx_fuzz(EPIC_SWUART_HandleTypeDef *h)
{
    for (int it = 0; it < 300; it++) {
        size_t len = (size_t)(rnd() % EPIC_SWUART_RING_SZ) + 1u;
        uint8_t buf[EPIC_SWUART_RING_SZ];
        for (size_t i = 0; i < len; i++) {
            buf[i] = (uint8_t)rnd();
        }
        tx_send_decode(h, buf, len);
    }
}

static void test_tx_short_write(EPIC_SWUART_HandleTypeDef *h)
{
    /* Fill the ring: a full Write pops one byte into the shift
     * register, so the ring ends RING_SZ-1 full. */
    uint8_t buf[EPIC_SWUART_RING_SZ + 3u];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)(i * 3u + 1u);
    }
    size_t q1 = EPIC_SWUART_Write(h, buf, EPIC_SWUART_RING_SZ);
    CHECK(q1 == EPIC_SWUART_RING_SZ, "short-write: full queue accepted");
    CHECK(h->tx_count == (uint8_t)(EPIC_SWUART_RING_SZ - 1u),
          "short-write: one byte moved to the shift register");

    /* The next Write may only queue what the ring still fits. */
    size_t q2 = EPIC_SWUART_Write(h, buf + EPIC_SWUART_RING_SZ, 3u);
    CHECK(q2 == 1u, "short-write: returns the ring's remaining capacity");
    CHECK(h->tx_count == (uint8_t)EPIC_SWUART_RING_SZ,
          "short-write: ring never exceeds capacity");

    /* Drain everything: RING_SZ+1 bytes total. */
    size_t total = EPIC_SWUART_RING_SZ + 1u;
    for (size_t b = 0; b < total; b++) {
        if (b > 0u) {
            swuart_test_fire_tx_event();   /* TX_IDLE pop arms the start bit */
        }
        uint8_t got = 0u;
        for (int k = 1; k <= 8; k++) {
            swuart_test_fire_tx_event();
            if (swuart_test_last_tx_mode() == (uint8_t)CCP_MODE_COMPARE_SET) {
                got |= (uint8_t)(1u << (k - 1));
            }
        }
        swuart_test_fire_tx_event();
        if (got != buf[b]) {
            CHECK(0, "short-write: bytes transmitted in order");
        }
    }
    CHECK(h->tx_count == 0u, "short-write: fully drained");
    CHECK(EPIC_SWUART_GetErrorCount(h) == 0u, "short-write: no errors");
}

static void test_rx_fuzz(EPIC_SWUART_HandleTypeDef *h)
{
    uint8_t expect[EPIC_SWUART_RING_SZ + 2u];
    size_t  expect_len = 0u;
    uint16_t err_before = EPIC_SWUART_GetErrorCount(h);

    for (int it = 0; it < 300; it++) {
        /* Inject 1..4 bytes, then read them back byte-exact. */
        size_t k = (size_t)(rnd() % 4u) + 1u;
        for (size_t i = 0; i < k; i++) {
            uint8_t b = (uint8_t)rnd();
            rx_send_byte(h, b);
            if (expect_len < sizeof(expect)) {
                expect[expect_len++] = b;
            }
        }
        CHECK(h->rx_count == (uint8_t)expect_len, "rx ring holds the injected bytes");
        CHECK(EPIC_SWUART_GetErrorCount(h) == err_before, "valid frames: no errors");

        uint8_t out[8];
        int n = EPIC_SWUART_Read(h, out, sizeof(out));
        CHECK(n == (int)k, "rx read returns the injected count");
        int ok = 1;
        for (size_t i = 0; i < k; i++) {
            if (out[i] != expect[expect_len - k + i]) ok = 0;
        }
        CHECK(ok, "rx bytes byte-exact in order");
        expect_len -= k;
    }
    CHECK(expect_len == 0u && h->rx_count == 0u, "rx ring empty at end");
}

static void test_rx_overflow_and_errors(EPIC_SWUART_HandleTypeDef *h)
{
    /* Fill the RX ring exactly. */
    uint8_t buf[EPIC_SWUART_RING_SZ];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)(i * 5u + 2u);
    }
    for (size_t i = 0; i < sizeof(buf); i++) {
        rx_send_byte(h, buf[i]);
    }
    CHECK(h->rx_count == (uint8_t)EPIC_SWUART_RING_SZ, "rx ring full");

    /* One more frame: dropped, error count bumped, ring unchanged. */
    uint16_t err = EPIC_SWUART_GetErrorCount(h);
    rx_send_byte(h, 0xA5u);
    CHECK(EPIC_SWUART_GetErrorCount(h) == (uint16_t)(err + 1u),
          "rx ring-full frame counts as an error");
    CHECK(h->rx_count == (uint8_t)EPIC_SWUART_RING_SZ,
          "rx ring-full drop leaves the ring unchanged");

    /* Bad stop bit: dropped, error count bumped. */
    err = EPIC_SWUART_GetErrorCount(h);
    rx_send_bad_stop(h, 0x5Au);
    CHECK(EPIC_SWUART_GetErrorCount(h) == (uint16_t)(err + 1u),
          "bad stop bit counts as an error");
    CHECK(h->rx_count == (uint8_t)EPIC_SWUART_RING_SZ,
          "bad stop bit does not push");

    /* The original RING_SZ bytes are still intact, in order. */
    uint8_t out[EPIC_SWUART_RING_SZ + 2u];
    int n = EPIC_SWUART_Read(h, out, sizeof(out));
    CHECK(n == (int)EPIC_SWUART_RING_SZ, "rx overflow: original bytes readable");
    int ok = 1;
    for (size_t i = 0; i < EPIC_SWUART_RING_SZ; i++) {
        if (out[i] != buf[i]) ok = 0;
    }
    CHECK(ok, "rx overflow: bytes preserved in order");
}

int main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                               FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "init ok");

    test_tx_fuzz(&h);
    test_tx_short_write(&h);
    test_rx_fuzz(&h);
    test_rx_overflow_and_errors(&h);

    printf("swuart_fuzz: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
