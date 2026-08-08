/**
 * @file    test_swuart_rx.c
 * @brief   RX-only host test, v3: no CCP hardware in the host sim, so
 *          this test drives the capture-then-compare event handler
 *          directly, injecting a synthetic capture value and a
 *          simulated RX pin level via *_sim_drive_input for each
 *          sample the handler is expected to take.
 */
#include <stdio.h>
#include "epic_swuart.h"
#include "pic16f87xa_sim.h"

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

extern void swuart_test_fire_rx_event(void);
extern void swuart_test_set_capture(uint16_t value);

int main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, 20000000UL, 9600u);

    /* 'A' = 0x41 = 0b01000001, LSB first: start=0,1,0,0,0,0,0,1,0,stop=1. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};

    pic16f87xa_sim_drive_input('C', 2, bits[0]);
    swuart_test_set_capture(1000u);
    swuart_test_fire_rx_event(); /* capture event: IDLE -> CONFIRM_START */
    swuart_test_fire_rx_event(); /* confirm event, half a bit later, same
                                   * bits[0]=0 still on the line: checks
                                   * the start bit is real, CONFIRM_START
                                   * -> DATA0. Two separate fires here,
                                   * not one: the confirm point (edge +
                                   * 0.5 bit) and d0's own sample point
                                   * (edge + 1.5 bit) are different
                                   * instants, matching on_rx_event_a's
                                   * two-hop deadline math. */

    for (size_t i = 1; i < 10; i++) {
        pic16f87xa_sim_drive_input('C', 2, bits[i]);
        swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }

    uint8_t buf[4] = {0};
    int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
    CHECK(n == 1, "one byte read");
    CHECK(buf[0] == 0x41u, "byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 0u, "no framing errors");

    printf("swuart_rx: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
