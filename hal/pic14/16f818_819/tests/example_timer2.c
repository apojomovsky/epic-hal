/* End-to-end smoke test for the Timer2 driver on the sim backend.
 * Expected register image after Init + Start with Period = 249 and a
 * 1:1 prescaler/postscaler: PR2 = 0xF9 (Bank 1, 0x92) and T2CON = 0x04
 * (TMR2ON set, T2CKPS = 00, TOUTPS = 0000, Register 8-1). The period is
 * (PR2+1) x prescaler x postscaler = 250 instruction cycles, so TMR2IF
 * fires every 250 cycles (DS39598F §8.0). */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_timer2.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/** Cycles per TMR2IF: (PR2+1) x pre x post. With PR2=249, this is 250. */
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
    if (overflows == 1U)
    {
        first_cycle = cycle_counter;
    }
}

/**
 * @brief Verify the Timer2 register image and overflow cadence on the
 *        sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();
    pic16f818_819_sim_set_irq_callback(TIMER2_IRQHandler);

    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    h.Prescaler       = TIMER2_PRESCALER_1_1;
    h.Postscaler      = TIMER2_POSTSCALER_1_1;
    h.Period          = 249U;     /* PR2 = 249, 250 ticks per period. */
    h.OverflowCallback = on_t2_overflow;

    EPIC_StatusTypeDef st = EPIC_TIMER2_Init(&h);
    CHECK(st == EPIC_OK, "Init returned error");
    EPIC_TIMER2_Start(&h);

    CHECK(EPIC_REG8(PIC_REG_PR2) == 249U, "PR2 (0x92) != 249");
    CHECK(EPIC_REG8(PIC_REG_T2CON) == 0x04U, "T2CON != 0x04 after Start");

    for (uint32_t i = 0; i < SIM_BUDGET; i++)
    {
        cycle_counter = i + 1;
        pic16f818_819_sim_step(1);
        if (overflows >= EXPECTED_OVERFLOWS) break;
    }

    /* Allow +-2 cycles for sim bookkeeping (the overflow is observed
     * after the step that incremented past the period). */
    int32_t delta = (int32_t)first_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    printf("TMR2: %u overflows, first at cycle %u (expected ~%u)\n",
           (unsigned)overflows, (unsigned)first_cycle,
           (unsigned)EXPECTED_PERIOD_CYCLES);
    CHECK(overflows >= EXPECTED_OVERFLOWS, "too few overflows inside the budget");
    CHECK(delta <= 2, "first overflow cycle outside the 250 cycle period");

    printf("example_timer2: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
