/* Two RX error paths: a bad stop bit (framing error) and an RX ring
 * flooded past EPIC_SWUART_RING_SZ without being drained. Drives the
 * capture/compare event handler directly via the test hooks
 * test_swuart_rx.c uses, single channel (slot A's fixed pins). After
 * the RX hot-path fix rx_capture_event_fast and rx_capture_event are
 * separate functions, so this file's coverage is channel A's path. */
#include <stdio.h>
#include "epic_swuart.h"

/* Family dispatch for the sim's drive_input, same set of families/macro
 * shape as test_swuart_rx.c/test_swuart_dual.c use. PIC16F193X's sim
 * only stages the driven level; unlike PIC16F87XA/PIC18Fxx5x's sim it
 * does not itself refresh the PORT register that EPIC_GPIO_ReadPin
 * reads (that refresh only happens inside pic16f193x_sim_step()), so
 * SIM_DRIVE for that family pumps one cycle of it immediately after. */
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

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/** @brief Test-only hooks: see test_swuart_rx.c. Defined in
 *         epic_swuart.c behind EPIC_SWUART_TEST_HOOKS. */
extern void epic_swuart_test_fire_rx_event(void);
#if EPIC_SWUART_HAS_RX_FAST_PATH
/** @brief Test hook: inject the fast-path RX capture value. */
extern void epic_swuart_test_set_capture_fast(uint16_t value);
#else
/** @brief Test hook: inject the generic RX capture value. */
extern void epic_swuart_test_set_capture(uint16_t value);
#endif

/**
 * @brief  Drives one full byte (start + 8 data + stop, LSB first) onto
 *         slot A's RX pin and fires the matching sequence of
 *         capture/compare events: one capture fire for the start bit
 *         (deglitch check + arm d0 in a single synchronous pass, IDLE
 *         -> DATA0), then one compare event per remaining bit
 *         (d0..d7, stop), exactly test_swuart_rx.c's technique. The
 *         capture value itself is arbitrary (host-sim only, no real
 *         Timer1 behind it) and can be reused unchanged across repeated
 *         bytes: each call starts from a fresh RX_IDLE, so nothing
 *         carries over between bytes.
 * @param bits the 10 bit levels (start, d0..d7, stop), LSB first.
 */
static void receive_byte(const uint8_t *bits)
{
    SIM_DRIVE('C', 2, bits[0]);
#if EPIC_SWUART_HAS_RX_FAST_PATH
    epic_swuart_test_set_capture_fast(1000u);
    epic_swuart_test_fire_rx_event(); /* one fire: deglitch check + arm d0,
                                   * IDLE -> DATA0 (see test_swuart_rx.c
                                   * for why this collapsed from two). */
#else
    /* PIC18Fxx5x/PIC16F193X channel A keeps the generic two-fire
     * capture-then-confirm sequence; see rx_capture_event. */
    epic_swuart_test_set_capture(1000u);
    epic_swuart_test_fire_rx_event(); /* capture event: IDLE -> CONFIRM_START */
    epic_swuart_test_fire_rx_event(); /* confirm event, half a bit later */
#endif
    for (size_t i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2, bits[i]);
        epic_swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }
}

/** @brief RX error-path host test main: bad stop bit and ring overflow. */
int main(void)
{
    /* Bad stop bit: hold the line low instead of returning to mark.
     * Slot A's fixed pins (see epic_swuart.c EPIC_SWUART_Init). */
    EPIC_SWUART_HandleTypeDef h;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                              FOSC_HZ, 9600u);
    CHECK(st == EPIC_OK, "init ok");

    static const uint8_t framing_err_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 0}; /* stop=0 */
    receive_byte(framing_err_bits);

    uint8_t buf[4];
    CHECK(EPIC_SWUART_Read(&h, buf, sizeof(buf)) == 0, "bad-stop byte dropped");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 1u, "one framing error counted");

    /* RX ring overflow: send more bytes than EPIC_SWUART_RING_SZ
     * without draining. Reuse the handle from above unchanged: a fresh
     * DeInit/Init cycle would reset error_count, and this scenario
     * wants exactly one more error source layered on top. */
    static const uint8_t ok_bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A', valid framing */
    for (unsigned i = 0; i < EPIC_SWUART_RING_SZ + 2u; i++) {
        receive_byte(ok_bits);
    }
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 3u,
          "one framing drop plus two overflow drops counted");

    int total = 0, n;
    while ((n = EPIC_SWUART_Read(&h, buf, sizeof(buf))) > 0) total += n;
    CHECK(total == (int)EPIC_SWUART_RING_SZ, "ring held exactly RING_SZ bytes");

    printf("epic_swuart_errors: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
