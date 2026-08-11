/* Bounded, self-reporting HARNESS=sim `mdb` gate: writes one byte
 * through the real CCP2 compare-driven TX state machine and confirms
 * it drains, reporting PASS/FAIL over the target's real USART (same
 * pattern as pic16_harness_sim_target.c). TX-only: MPLAB SIM cannot
 * inject an RX bitstream (SCL stimulus never registers a CCP1
 * capture; breakpoint-driven pin writes never reproduced the byte, see
 * docs/superpowers/plans/2026-08-07-swuart-v3.md Task 8 for the
 * write-up), so the real RX path stays uncovered (docs/API.md).
 * tx_count drops when Write() dequeues into the shift register, but
 * the SIM_ITERATIONS budget still lets the real compare events run, so
 * a broken CCP2 ISR path fails by never reaching epic_harness_report. */
#include "epic_swuart.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

/** Loop-iteration bound, not a real time unit (see
 *  core/epic_harness.h). 200000 confirmed (by hand, under `mdb`) to
 *  finish and reach epic_harness_report well inside a 60000 ms
 *  wait_ms budget on PIC16F877A/MPLAB SIM. */
#define SIM_ITERATIONS 200000UL

#define TEST_BYTE 0x41u

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, FOSC_HZ, 9600u);

    uint8_t tx_byte = TEST_BYTE;
    size_t queued = EPIC_SWUART_Write(&h, &tx_byte, 1);

    int drained = 0;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        EPIC_WDT_Refresh();
        if (!drained && h.tx_count == 0u) {
            drained = 1;
        }
    }

    epic_harness_log((queued == 1u && drained)
                          ? "swuart sim: tx drained\n"
                          : "swuart sim: tx did not drain\n");
    return epic_harness_report(queued == 1u && drained);
}
