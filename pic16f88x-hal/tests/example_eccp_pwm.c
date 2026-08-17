/* End-to-end smoke test: PWM output on RC2/CCP1. Timer2 PR2=99, 1:1
 * prescaler/postscaler gives a 100-cycle period; CCP1 in PWM mode at
 * 50% duty. The sim does not toggle the RC2/CCP1 pin, so the test
 * verifies the configured registers (CCP1CON, PR2, T2CON) and counts
 * TMR2 overflows for the period. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_ccp.h"
#include "peripherals/pic16f88x_timer2.h"
#include <stdio.h>

/* DS40001291H §8.3.1: PWM period = (PR2+1) × 4 × Tosc × T2 prescaler.
 * In instruction cycles, Fosc = 4 × Fcycle so the 4 × Tosc collapses
 * to 1 instruction cycle → period = (PR2+1) × T2 prescaler.
 * With PR2=99, prescaler=1:1: 100 instruction cycles per period. */
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
 * @brief Verify CCP1 PWM configuration registers and Timer2 period
 *        overflows on the sim backend.
 */
int main(void)
{
    pic16f88x_sim_reset();
    pic16f88x_sim_set_irq_callback(TIMER2_IRQHandler);

    /* 1. Configure Timer2 as the PWM time base (DS40001291H §8.3.3 step 4). */
    TIMER2_HandleTypeDef th = TIMER2_HANDLE_DEFAULT;
    th.Prescaler       = TIMER2_PRESCALER_1_1;
    th.Postscaler      = TIMER2_POSTSCALER_1_1;
    th.Period          = 99U;     /* PR2 = 99 → 100 ticks per period. */
    th.OverflowCallback = on_t2_overflow;
    EPIC_TIMER2_Init(&th);

    /* 2. Configure CCP1 in PWM mode at 50% duty.
     *    DS40001291H §8.3.2:  duty = CCPR1L:CCP1CON<5:4>.
     *    10-bit duty value for 50% of (PR2+1) = 50% of 100 = 50. */
    CCP_HandleTypeDef ch = { 0 };
    ch.Instance      = CCP_INSTANCE_1;
    ch.Mode          = CCP_MODE_PWM;
    ch.PWM.Period    = 99U;
    ch.PWM.Duty      = 50U;       /* 50 of 100 → 50%. */
    ch.EventCallback = NULL;      /* Don't need an IRQ. */
    EPIC_CCP_Init(&ch);

    /* 3. Start Timer2, this writes PR2 + T2CON. PWM output is generated
     *    as soon as TMR2 starts incrementing (DS40001291H §8.3.3 step 4). */
    EPIC_TIMER2_Start(&th);

    /* 4. Verify the configuration went to the right registers. */
    uint8_t con = EPIC_REG8(0x17U);  /* CCP1CON */
    uint8_t rl  = EPIC_REG8(0x15U);  /* CCPR1L */
    uint8_t pr2 = EPIC_REG8(0x92U);  /* PR2, Bank 1 */

    /* 50% of 100 = 50 → 10-bit value 50 = 0x032.
     * CCPR1L = 0x0C (50 >> 2).
     * CCP1CON<5:4> = 0x032 & 0x3 = 0x2, shifted to bits 4..5. */
    if (rl != 12U) {
        printf("FAIL: CCPR1L = %u, expected 12\n", (unsigned)rl);
        return 1;
    }
    if (pr2 != 99U) {
        printf("FAIL: PR2 = %u, expected 99\n", (unsigned)pr2);
        return 1;
    }
    if ((con & 0x0CU) != 0x0CU) {
        printf("FAIL: CCP1CON mode bits = 0x%X, expected 0xC (PWM)\n",
               (unsigned)(con & 0x0FU));
        return 1;
    }

    /* 5. Run the sim and count TMR2 overflows. Each overflow = one PWM
     *    period. First overflow should fire at the end of the period. */
    for (uint32_t i = 0; i < SIM_BUDGET; i++) {
        cycle_counter = i + 1;
        pic16f88x_sim_step(1);
        if (t2_overflows >= EXPECTED_OVERFLOWS) break;
    }

    int32_t delta = (int32_t)first_t2_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    if (t2_overflows >= EXPECTED_OVERFLOWS && delta <= 4) {
        printf("OK: CCP1 PWM configured; TMR2 produced %u overflows, "
               "first at cycle %u (expected ~%u)\n",
               (unsigned)t2_overflows,
               (unsigned)first_t2_cycle ? (unsigned)first_t2_cycle : 1U,
               (unsigned)EXPECTED_PERIOD_CYCLES);
        return 0;
    }
    printf("FAIL: t2_overflows=%u first_t2_cycle=%u expected ~%u\n",
           (unsigned)t2_overflows, (unsigned)first_t2_cycle,
           (unsigned)EXPECTED_PERIOD_CYCLES);
    return 1;
}
