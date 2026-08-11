/* TX-only host test: no CCP hardware exists in the host sim, so this
 * drives the compare event handler directly and inspects the
 * mode/compare-value sequence it arms at each step, rather than
 * sampling a simulated output pin. */
#include <stdio.h>
#include "epic_swuart.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Test-only hook: exposes the last (mode, compare-value) pair Task 4's
 * code armed via EPIC_CCP_SetMode/SetCompare, so this test can check
 * the sequence without any real CCP hardware behind it. Defined in
 * epic_swuart.c behind EPIC_SWUART_TEST_HOOKS, not part of the public
 * header (test-only, not an API commitment). */
#ifndef EPIC_SWUART_TEST_HOOKS
#define EPIC_SWUART_TEST_HOOKS 1
#endif
extern uint8_t swuart_test_last_tx_mode(void);
extern uint16_t swuart_test_last_tx_compare(void);
extern void swuart_test_fire_tx_event(void);

int main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_StatusTypeDef st = EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2,
                                              20000000UL, 9600u);
    CHECK(st == EPIC_OK, "init ok");

    size_t queued = EPIC_SWUART_Write(&h, (const uint8_t *)"A", 1);
    CHECK(queued == 1u, "queued one byte");

    /* 'A' = 0x41 = 0b01000001. LSB first: start=0, d0=1, d1..d5=0,
     * d6=1, d7=0, stop=1. Mode 9 = CCP_MODE_COMPARE_CLEAR (space/low),
     * mode 8 = CCP_MODE_COMPARE_SET (mark/high), matching this
     * family's CCP_ModeTypeDef encoding (pic16f87xa_ccp.h). */
    static const uint8_t expected_modes[] = {8, 9, 9, 9, 9, 9, 8, 9, 8};
    /* Index 0 is the mode armed by Write() itself for the start bit
     * (checked separately below); indices 1..9 are what each of the
     * nine compare events (d0..d7, stop) arms for the *following*
     * event, per tx_compare_event's one-ahead scheduling. */

    CHECK(swuart_test_last_tx_mode() == 9, "start bit armed as CLEAR (space)");

    for (size_t i = 0; i < 9; i++) {
        swuart_test_fire_tx_event();
        char msg[32];
        snprintf(msg, sizeof(msg), "event %u mode", (unsigned)i);
        CHECK(swuart_test_last_tx_mode() == expected_modes[i], msg);
    }

    printf("swuart_tx: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
