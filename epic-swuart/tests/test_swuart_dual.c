/**
 * @file    test_swuart_dual.c
 * @brief   Two channels active at once, v3: channel A transmits while
 *          channel B receives. PIC16F193X only, the only family with
 *          two full CCP RX+TX pairs (channel A: CCP1/CCP2, channel B:
 *          CCP3/CCP4); PIC16F87XA/PIC18Fxx5x have exactly one pair,
 *          already spent on channel A, so EPIC_SWUART_MAX_CHANNELS is
 *          1 there (see epic_swuart.h). No real CCP hardware exists in
 *          the host sim, so this test drives each channel's event
 *          handler directly via the same test hooks
 *          test_swuart_tx.c/test_swuart_rx.c use, not through
 *          epic_harness_tick()/real interrupt dispatch.
 *
 *          Compiles to an empty, trivially-passing translation unit on
 *          the other two families instead of failing to build there.
 */
#include "epic_swuart.h"

#if EPIC_SWUART_MAX_CHANNELS >= 2

#include <stdio.h>
#include "pic16f193x_sim.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Test-only hooks: see test_swuart_tx.c/test_swuart_rx.c for the
 * channel-A originals. swuart_test_fire_rx_event_b/on_tx_event_b's
 * peer for channel A (swuart_test_fire_tx_event) is reused as-is;
 * channel B needs its own RX-side hook since this scenario only
 * receives on B. Defined in epic_swuart.c behind EPIC_SWUART_TEST_HOOKS. */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
extern void swuart_test_fire_tx_event(void);
extern void swuart_test_fire_rx_event_b(void);
extern void swuart_test_set_capture(uint16_t value);

int main(void)
{
    /* pic16f193x_sim_drive_input only stages the driven level; unlike
     * pic16f87xa_sim_drive_input it does not itself update the PORT
     * register that EPIC_GPIO_ReadPin reads (confirmed by reading
     * pic16f193x_sim.c: the refresh only happens inside
     * pic16f193x_sim_step(), normally pumped by epic_harness_tick()).
     * This test drives events directly instead of ticking a harness
     * loop, so each drive_input() below is paired with a one-cycle
     * pic16f193x_sim_step(1) to force that refresh. pic16f193x_sim_reset()
     * puts every SFR at its POR value first, matching what
     * epic_harness_init() would do. */
    pic16f193x_sim_reset();

    EPIC_SWUART_HandleTypeDef chan_a, chan_b;

    /* Channel A: CCP1 = RX (RC2), CCP2 = TX (RC1). */
    EPIC_StatusTypeDef st_a = EPIC_SWUART_Init(&chan_a, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                                FOSC_HZ, 9600u);
    CHECK(st_a == EPIC_OK, "channel A init ok");
    /* Channel B: CCP3 = RX (RB5), CCP4 = TX (RD1). */
    EPIC_StatusTypeDef st_b = EPIC_SWUART_Init(&chan_b, GPIOD, GPIO_PIN_1, GPIOB, GPIO_PIN_5,
                                                FOSC_HZ, 9600u);
    CHECK(st_b == EPIC_OK, "channel B init ok");

    /* ---- Channel A transmits 'Z' (0x5A) via direct compare-event
     * firing, same technique as test_swuart_tx.c: Write() arms the
     * start bit itself, then nine fires (d0..d7, stop) drive the rest. ---- */
    size_t queued = EPIC_SWUART_Write(&chan_a, (const uint8_t *)"Z", 1);
    CHECK(queued == 1u, "channel A queued one byte");
    for (size_t i = 0; i < 9; i++) swuart_test_fire_tx_event();
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors");

    /* ---- Channel B receives 'A' (0x41) via direct capture/compare-
     * event firing, same technique as test_swuart_rx.c: inject a
     * synthetic capture value and a simulated RX pin level for each
     * sample the handler is expected to take. ---- */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A', LSB first */
    pic16f193x_sim_drive_input('B', 5, bits[0]);
    pic16f193x_sim_step(1); /* refresh PORTB so EPIC_GPIO_ReadPin sees it */
    swuart_test_set_capture(1000u);
    swuart_test_fire_rx_event_b(); /* capture event: IDLE -> CONFIRM_START */
    swuart_test_fire_rx_event_b(); /* confirm event, half a bit later */
    for (size_t i = 1; i < 10; i++) {
        pic16f193x_sim_drive_input('B', 5, bits[i]);
        pic16f193x_sim_step(1);
        swuart_test_fire_rx_event_b(); /* compare event: sample + arm next */
    }

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan_b, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel B received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel B byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors");

    /* ---- EPIC_SWUART_Init: NULL handle and a full channel registry
     * both return EPIC_INVALID. Both slots (A, B) are already occupied
     * at this point in the test. ---- */
    CHECK(EPIC_SWUART_Init(NULL, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects NULL handle");
    EPIC_SWUART_HandleTypeDef chan_c;
    CHECK(EPIC_SWUART_Init(&chan_c, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects a third channel when both slots are full");

    printf("swuart_dual: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}

#else /* EPIC_SWUART_MAX_CHANNELS < 2: not a PIC16F193X build. */

int main(void) { return 0; }

#endif
