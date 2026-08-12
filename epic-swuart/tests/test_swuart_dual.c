/* Two channels active at once: channel A transmits while channel B
 * receives. PIC16F193X only (two full CCP RX+TX pairs); compiles to an
 * empty, trivially-passing TU on the other families. Drives each
 * channel's event handler directly via the test hooks
 * test_swuart_tx.c/test_swuart_rx.c use. */
#include "epic_swuart.h"

#if EPIC_SWUART_MAX_CHANNELS >= 2

#include <stdio.h>
#include "pic16f193x_sim.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Test-only hooks: see test_swuart_tx.c/test_swuart_rx.c for the
 * channel-A originals. epic_swuart_test_fire_rx_event_b/on_tx_event_b's
 * peer for channel A (epic_swuart_test_fire_tx_event) is reused as-is;
 * channel B needs its own RX-side hook since this scenario only
 * receives on B. Defined in epic_swuart.c behind EPIC_SWUART_TEST_HOOKS.
 *
 * epic_swuart_test_fire_tx_event_b/epic_swuart_test_last_tx_mode_b are channel
 * B's own TX-side hooks, added alongside the EPIC_SWUART_Write() CCP
 * dispatch fix (a final-review bug: Write() used to hardcode channel
 * A's CCP2 for every handle, so a Write() on channel B silently armed
 * channel A's hardware instead of channel B's own CCP4). The "channel B
 * transmits" scenario below exercises channel B's TX path specifically
 * to catch a regression of that bug. */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
/** @brief Test hook: fire one channel A TX compare event. */
extern void epic_swuart_test_fire_tx_event(void);
/** @brief Test hook: fire one channel B TX compare event. */
extern void epic_swuart_test_fire_tx_event_b(void);
/** @brief Test hook: channel B's last armed TX mode (CCP4CON). */
extern uint8_t epic_swuart_test_last_tx_mode_b(void);
/** @brief Test hook: fire one channel B RX capture/compare event. */
extern void epic_swuart_test_fire_rx_event_b(void);
/** @brief Test hook: inject the generic RX capture value. */
extern void epic_swuart_test_set_capture(uint16_t value);

/** @brief Dual-channel host test main: A transmits while B receives. */
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

    /* Channel A transmits 'Z' (0x5A) via direct compare-event firing,
     * same technique as test_swuart_tx.c: Write() arms the start bit
     * itself, then nine fires (d0..d7, stop) drive the rest. */
    size_t queued = EPIC_SWUART_Write(&chan_a, (const uint8_t *)"Z", 1);
    CHECK(queued == 1u, "channel A queued one byte");
    for (size_t i = 0; i < 9; i++) epic_swuart_test_fire_tx_event();
    CHECK(chan_a.tx_count == 0u, "channel A finished transmitting");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_a) == 0u, "channel A no errors");

    /* Channel B receives 'A' (0x41) via direct capture/compare-event
     * firing, same technique as test_swuart_rx.c: inject a synthetic
     * capture value and a simulated RX pin level for each sample the
     * handler is expected to take. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1}; /* 'A', LSB first */
    pic16f193x_sim_drive_input('B', 5, bits[0]);
    pic16f193x_sim_step(1); /* refresh PORTB so EPIC_GPIO_ReadPin sees it */
    epic_swuart_test_set_capture(1000u);
    epic_swuart_test_fire_rx_event_b(); /* capture event: IDLE -> CONFIRM_START */
    epic_swuart_test_fire_rx_event_b(); /* confirm event, half a bit later */
    for (size_t i = 1; i < 10; i++) {
        pic16f193x_sim_drive_input('B', 5, bits[i]);
        pic16f193x_sim_step(1);
        epic_swuart_test_fire_rx_event_b(); /* compare event: sample + arm next */
    }

    uint8_t rx_buf[4] = {0};
    int n = EPIC_SWUART_Read(&chan_b, rx_buf, sizeof(rx_buf));
    CHECK(n == 1, "channel B received one byte");
    CHECK(rx_buf[0] == 0x41u, "channel B byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B no errors");

    /* Regression coverage: channel B transmits 'B' (0x42) via
     * EPIC_SWUART_Write(&chan_b, ...), the one scenario the original
     * bug (Write() hardcoding SWUART_CCP_TX/CCP2 for every handle) was
     * never caught by. With the bug present, this Write() would arm
     * channel A's CCP2CON instead of channel B's own CCP4CON, which
     * the two checks right after Write() catch: mode_b stays at
     * CCP_MODE_OFF (0) instead of CLEAR (9), and channel A's mode is
     * clobbered from its post-scenario-1 value (8) to 9. */
    uint8_t byte_b = 0x42u; /* 'B' = 0b01000010, LSB first: start=0,
                                d0=0, d1=1, d2..d5=0, d6=1, d7=0, stop=1 */
    uint8_t chan_a_mode_before = epic_swuart_test_last_tx_mode();
    size_t queued_b = EPIC_SWUART_Write(&chan_b, &byte_b, 1);
    CHECK(queued_b == 1u, "channel B queued one byte");
    CHECK(epic_swuart_test_last_tx_mode_b() == 9u,
          "channel B start bit armed as CLEAR on its OWN CCP4 (not left at CCP_MODE_OFF)");
    CHECK(epic_swuart_test_last_tx_mode() == chan_a_mode_before,
          "channel A's own CCP2 left untouched by channel B's Write()");
    for (size_t i = 0; i < 9; i++) epic_swuart_test_fire_tx_event_b();
    CHECK(chan_b.tx_count == 0u, "channel B finished transmitting");
    CHECK(EPIC_SWUART_GetErrorCount(&chan_b) == 0u, "channel B TX no errors");

    /* EPIC_SWUART_Init: NULL handle and a full channel registry both
     * return EPIC_INVALID. Both slots (A, B) are already occupied at
     * this point in the test. */
    CHECK(EPIC_SWUART_Init(NULL, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects NULL handle");
    EPIC_SWUART_HandleTypeDef chan_c;
    CHECK(EPIC_SWUART_Init(&chan_c, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                            FOSC_HZ, 9600u) == EPIC_INVALID,
          "init rejects a third channel when both slots are full");

    printf("epic_swuart_dual: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}

#else /* EPIC_SWUART_MAX_CHANNELS < 2: not a PIC16F193X build. */

/** @brief Empty TU main for single-channel families. */
int main(void) { return 0; }

#endif
