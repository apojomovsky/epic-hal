/* End-to-end smoke test for the Timer2 shim (peripherals/hal_timer2.h)
 * on the sim backend. PR2=249, prescaler 1:1, postscaler 1:1: period =
 * (PR2+1) x pre x post = 250 instruction cycles (DS41364B section 17.0).
 * After Init the caller's handle is scrubbed, simulating XC8 reusing
 * that stack: a shim that registered the caller's address instead of
 * copying into the driver-owned slot then calls garbage.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/hal_timer2.h"
#include "core/pic16f193x_irq.h"
#include "core/epic_harness.h"
#include <stdio.h>
#include <string.h>

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
 * @brief Verify the shim's Timer2 period and overflow cadence.
 */
int main(void)
{
    epic_harness_init(SIM_BUDGET);

    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    h.Prescaler        = TIMER2_PRESCALER_1_1;
    h.Postscaler       = TIMER2_POSTSCALER_1_1;
    h.Period           = 249U;    /* PR2 = 249, 250 ticks per period. */
    h.OverflowCallback = on_t2_overflow;
    EPIC_TIMER2_Init(&h);
    EPIC_TIMER2_Start(&h);
    /* The stack slot is dead from here on: XC8 reuses it on return
     * into the caller, so scrub it. The ISR must use the owned copy. */
    memset(&h, 0xA5, sizeof(h));
    EPIC_IRQ_Restore(1);

    if (EPIC_TIMER2_ReadPeriod() != 249U)
    {
        printf("FAIL: PR2=%u, expected 249\n",
               (unsigned)EPIC_TIMER2_ReadPeriod());
        return 1;
    }

    for (uint32_t i = 0; i < SIM_BUDGET; i++)
    {
        cycle_counter = i + 1;
        epic_harness_tick();
        if (overflows >= EXPECTED_OVERFLOWS) break;
    }

    /* Same slack as the pic14 Timer2 test: observe after the step. */
    int32_t delta = (int32_t)first_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    if (overflows >= EXPECTED_OVERFLOWS && delta <= 2)
    {
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
