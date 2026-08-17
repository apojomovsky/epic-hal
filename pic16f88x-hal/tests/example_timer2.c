/* End-to-end smoke test for the Timer2 driver on the sim backend.
 * PR2=249, prescaler 1:1, postscaler 1:1: period = (PR2+1) x pre x
 * post = 250 instruction cycles (DS40001291H §7.0). */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_timer2.h"
#include "core/pic16_irq.h"
#include <stdio.h>

/** Cycles per TMR2IF: (PR2+1) × pre × post. With PR2=249, this is 250. */
#define EXPECTED_PERIOD_CYCLES  250UL
#define EXPECTED_OVERFLOWS      5U
#define SIM_BUDGET              ((EXPECTED_PERIOD_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t overflows = 0;
static volatile uint32_t first_cycle = 0;
static uint32_t cycle_counter = 0;

/**
 * @brief Count Timer2 overflows, recording the first overflow cycle.
 */
static void on_t2_overflow(void)
{
    overflows++;
    if (overflows == 1U) {
        first_cycle = cycle_counter;
    }
}

/**
 * @brief Verify Timer2 period/overflow cadence on the sim backend.
 */
int main(void)
{
    pic16f88x_sim_reset();
    pic16f88x_sim_set_irq_callback(TIMER2_IRQHandler);

    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    h.Prescaler       = TIMER2_PRESCALER_1_1;
    h.Postscaler      = TIMER2_POSTSCALER_1_1;
    h.Period          = 249U;     /* PR2 = 249 → 250 ticks per period. */
    h.OverflowCallback = on_t2_overflow;

    EPIC_StatusTypeDef st = EPIC_TIMER2_Init(&h);
    if (st != EPIC_OK) { printf("FAIL: Init returned %u\n", (unsigned)st); return 1; }
    EPIC_TIMER2_Start(&h);

    for (uint32_t i = 0; i < SIM_BUDGET; i++) {
        cycle_counter = i + 1;
        pic16f88x_sim_step(1);
        if (overflows >= EXPECTED_OVERFLOWS) break;
    }

    /* The expected period is 250 cycles. Allow ±2 cycles slack for sim
     * bookkeeping (we observe after the step that just incremented). */
    int32_t delta = (int32_t)first_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    if (overflows >= EXPECTED_OVERFLOWS && delta <= 2) {
        printf("OK: Timer2 produced %u overflows, first at cycle %u (expected ~%u)\n",
               (unsigned)overflows, (unsigned)first_cycle,
               (unsigned)EXPECTED_PERIOD_CYCLES);
        return 0;
    }
    printf("FAIL: Timer2 overflows=%u, first_cycle=%u, expected ~%u\n",
           (unsigned)overflows, (unsigned)first_cycle,
           (unsigned)EXPECTED_PERIOD_CYCLES);
    return 1;
}
