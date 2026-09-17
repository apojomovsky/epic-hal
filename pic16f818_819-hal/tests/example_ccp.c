/* End-to-end smoke test: PWM on the CCP1 pin, RB3 with the default
 * CCPMX configuration bit and RB2 with CCPMX = 1 (DS39598F Table 1-2).
 * Timer2 runs PR2 = 99 with a 1:1 prescaler/postscaler, so the PWM
 * period is (PR2+1) x 4 x Tosc x T2 prescaler, which is (PR2+1) x
 * prescaler = 100 instruction cycles once 4 x Tosc collapses to one
 * instruction cycle (§9.3.1), at 50% duty: PR2 = 0x63, T2CON = 0x04,
 * CCPR1L = 0x0C, CCP1CON = 0x2C (mode 1100 PWM, duty LSBs 10, §9.3.2).
 * The sim does not drive the pin, so TMR2 overflows stand in as markers. */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_ccp.h"
#include "peripherals/hal_timer2.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

#define EXPECTED_PERIOD_CYCLES  100UL
#define EXPECTED_OVERFLOWS      5U
#define SIM_BUDGET              ((EXPECTED_PERIOD_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t t2_overflows   = 0;
static volatile uint32_t first_t2_cycle = 0;
static uint32_t cycle_counter = 0;

/**
 * @brief Count Timer2 overflows, recording the first overflow cycle.
 */
static void on_t2_overflow(void)
{
    if (t2_overflows == 0U) first_t2_cycle = cycle_counter;
    t2_overflows++;
}

/**
 * @brief Verify the CCP1 PWM register image and the Timer2 period on
 *        the sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();
    pic16f818_819_sim_set_irq_callback(TIMER2_IRQHandler);

    /* 1. Configure Timer2 as the PWM time base (DS39598F §9.3.3 steps 1
     *    and 4: the period register, then the T2CON prescale and enable). */
    TIMER2_HandleTypeDef th = TIMER2_HANDLE_DEFAULT;
    th.Prescaler       = TIMER2_PRESCALER_1_1;
    th.Postscaler      = TIMER2_POSTSCALER_1_1;
    th.Period          = 99U;     /* PR2 = 99, 100 ticks per period. */
    th.OverflowCallback = on_t2_overflow;
    EPIC_StatusTypeDef st = EPIC_TIMER2_Init(&th);
    CHECK(st == EPIC_OK, "Timer2 Init returned error");

    /* 2. Configure CCP1 in PWM mode at 50% duty.
     *    DS39598F §9.3.2: duty = CCPR1L:CCP1CON<5:4>.
     *    10-bit duty value for 50% of (PR2+1) = 50% of 100 = 50. */
    CCP_HandleTypeDef ch = { 0 };
    ch.Instance      = CCP_INSTANCE_1;
    ch.Mode          = CCP_MODE_PWM;
    ch.PWM.Period    = 99U;
    ch.PWM.Duty      = 50U;       /* 50 of 100, 50%. */
    ch.EventCallback = NULL;      /* Don't need an IRQ. */
    st = EPIC_CCP_Init(&ch);
    CHECK(st == EPIC_OK, "CCP Init returned error");

    /* 3. Start Timer2, this writes PR2 + T2CON. PWM output starts as
     *    soon as TMR2 begins incrementing (DS39598F §9.3.3 step 4). */
    EPIC_TIMER2_Start(&th);

    /* 4. Verify the configuration went to the right registers. */
    CHECK(EPIC_REG8(PIC_REG_PR2) == 99U, "PR2 (0x92) != 99");
    CHECK(EPIC_REG8(PIC_REG_T2CON) == 0x04U, "T2CON != 0x04 after Start");
    /* 50 of 100: CCPR1L = 50 >> 2 = 12, CCP1CON<5:4> = 50 & 3 = 2. */
    CHECK(EPIC_REG8(PIC_REG_CCP1RL) == 12U, "CCPR1L (0x15) != 12");
    CHECK(EPIC_REG8(PIC_REG_CCP1CON) == 0x2CU, "CCP1CON (0x17) != 0x2C");

    /* 5. Run the sim and count TMR2 overflows. Each overflow is one PWM
     *    period, so the first one marks the end of the first period. */
    for (uint32_t i = 0; i < SIM_BUDGET; i++)
    {
        cycle_counter = i + 1;
        pic16f818_819_sim_step(1);
        if (t2_overflows >= EXPECTED_OVERFLOWS) break;
    }

    int32_t delta = (int32_t)first_t2_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    printf("CCP1 PWM: %u TMR2 overflows, first at cycle %u (expected ~%u)\n",
           (unsigned)t2_overflows, (unsigned)first_t2_cycle,
           (unsigned)EXPECTED_PERIOD_CYCLES);
    CHECK(t2_overflows >= EXPECTED_OVERFLOWS, "too few overflows inside the budget");
    CHECK(delta <= 4, "first overflow cycle outside the 100 cycle period");

    printf("example_ccp: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
