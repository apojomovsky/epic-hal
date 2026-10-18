/**
 * PWM6 and PWM7 host-sim probe. Host-only: advances the Timer2 and PWM
 * models through epic_harness_tick, so it is not in the XC8 build.
 * Covers Init/SetDuty validation, the register images of both modules
 * (enable, polarity, the split 10-bit duty, the CCPTMRS1 time base
 * field), the duty and period against Timer2, the polarity inversion and
 * the disabled idle level, the end-of-period duty latch, 0% and 100%
 * duty, and the time bases the model does not run.
 *
 * Expected register image (PWM6, duty 0x2A5 = 677, active high, Timer2,
 * after Init):
 *   PWM6CON  = 0x80   (EN, POL clear)
 *   PWM6DCH  = 0xA9   (duty bits 9:2)
 *   PWM6DCL  = 0x40   (duty bits 1:0 in bits 7:6)
 *   CCPTMRS1 = 0x15   (PWM6 and PWM7 on Timer2, CCP5 on the Timer1/2 couple)
 * Timer2 at PR2 9 and 1:1 has a 10 tick period (40 counts): the first
 * period starts on tick 10, and every 4 counts of duty are one tick high.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_ccp.h"
#include "peripherals/pic16f188xx_pwm.h"
#include "peripherals/pic16f188xx_timer2.h"

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Run n ticks of a PWM module and count high ticks and rising edges.
 * @param inst Module 6 or 7.
 * @param n Ticks to run.
 * @param high Out: ticks the output read high after the step.
 * @param rises Out: low-to-high transitions seen.
 */
static void sample(uint8_t inst, unsigned int n, unsigned int *high, unsigned int *rises)
{
    unsigned int i;
    uint8_t prev = pic16f188xx_sim_pwm_out(inst);

    *high = 0U;
    *rises = 0U;
    for (i = 0U; i < n; i++)
    {
        uint8_t now;

        step(1U);
        now = pic16f188xx_sim_pwm_out(inst);
        *high += now;
        if ((now != 0U) && (prev == 0U))
        {
            (*rises)++;
        }
        prev = now;
    }
}

/**
 * @brief Start Timer2 free-running from FOSC/4 with PR2 9.
 */
static void t2_start(void)
{
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

    t2.Period = 9U;
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER2_Start(&t2);
}

/**
 * @brief Stop and reset Timer2.
 */
static void t2_stop(void)
{
    (void)EPIC_TIMER2_Stop();
    (void)EPIC_TIMER2_DeInit();
}

/**
 * @brief PWM6 and PWM7 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    unsigned int high;
    unsigned int rises;
    PWM_HandleTypeDef h = PWM_HANDLE_DEFAULT;

    epic_harness_init(10000UL);

    /* Validation: NULL, bad module, bad time base and duty are rejected
     * unwritten. */
    ok &= (EPIC_PWM_Init(NULL) == EPIC_INVALID);
    h.Instance = (PWM_InstanceTypeDef)5;
    ok &= (EPIC_PWM_Init(&h) == EPIC_INVALID);
    h.Instance = (PWM_InstanceTypeDef)8;
    ok &= (EPIC_PWM_Init(&h) == EPIC_INVALID);
    h.Instance = PWM_INSTANCE_6;
    h.Timer = (PWM_TimerTypeDef)0U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_INVALID);
    h.Timer = (PWM_TimerTypeDef)4U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_INVALID);
    h.Timer = PWM_TIMER_2;
    h.Duty = 1024U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_6, 1024U) == EPIC_INVALID);
    ok &= (EPIC_PWM_SetDuty((PWM_InstanceTypeDef)5, 1U) == EPIC_INVALID);
    ok &= (EPIC_PWM_DeInit((PWM_InstanceTypeDef)8) == EPIC_INVALID);
    ok &= (EPIC_PWM_GetOutput((PWM_InstanceTypeDef)5) == 0U);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);

    /* Register image of both modules at their own addresses. */
    h = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
    h.Duty = 0x2A5U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_PWM6DCH) == 0xA9U);
    ok &= (EPIC_REG8(PIC_REG_PWM6DCL) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);
    h.Instance = PWM_INSTANCE_7;
    h.Duty = 0x3FFU;
    h.ActiveLow = 1U;
    h.Timer = PWM_TIMER_6;
    ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_PWM7CON) == 0x90U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCH) == 0xFFU);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCL) == 0xC0U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x35U);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0x80U);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_7, 0x001U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCL) == 0x40U);

    /* DeInit restores each module and only its time base field. */
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_7) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_PWM7CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0x80U);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_6) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0x00U);

    /* The time base fields leave CCP5's field alone. */
    {
        CCP_HandleTypeDef c = CCP_HANDLE_DEFAULT;

        c.Instance = CCP_INSTANCE_5;
        c.Mode = CCP_MODE_COMPARE_SET;
        c.Timer = CCP_TIMER_5_6;
        ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
        h = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
        h.Timer = PWM_TIMER_4;
        ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x1BU);
        ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_6) == EPIC_OK);
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);
    }

    /* 20 counts are 5 ticks high in a 10 tick period; the output reads
     * low until the first period starts on tick 10. */
    h = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
    h.Duty = 20U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
    t2_start();
    step(9U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 0U) && (EPIC_PWM_GetOutput(PWM_INSTANCE_6) == 0U);
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 1U) && (EPIC_PWM_GetOutput(PWM_INSTANCE_6) == 1U);
    ok &= (EPIC_REG8(PIC_REG_PWM6CON) == 0xA0U);
    sample(6U, 50U, &high, &rises);
    ok &= (high == 25U) && (rises == 5U);
    ok &= (pic16f188xx_sim_pwm_out(7U) == 0U);

    /* The duty latches at the period boundary. */
    step(2U);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_6, 4U) == EPIC_OK);
    sample(6U, 8U, &high, &rises);
    ok &= (high == 3U);
    sample(6U, 10U, &high, &rises);
    ok &= (high == 1U) && (rises == 1U);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_6, 0U) == EPIC_OK);
    step(10U);
    sample(6U, 30U, &high, &rises);
    ok &= (high == 0U) && (rises == 0U);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_6, EPIC_CCP_PWMPeriodCounts(9U)) == EPIC_OK);
    step(10U);
    sample(6U, 30U, &high, &rises);
    ok &= (high == 30U) && (rises == 0U);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_6) == EPIC_OK);
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 0U);
    t2_stop();

    /* Active low inverts the pulse and idles high while disabled. */
    h = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
    h.Instance = PWM_INSTANCE_7;
    h.Duty = 20U;
    h.ActiveLow = 1U;
    ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(7U) == 1U);
    t2_start();
    step(10U);
    sample(7U, 50U, &high, &rises);
    ok &= (high == 25U);
    EPIC_REG8(PIC_REG_PWM7CON) &= (uint8_t)~PIC_PWM6CON_EN;
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(7U) == 1U);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_7) == EPIC_OK);
    t2_stop();

    /* A time base the model does not run holds the idle level. Timer4 PWM
     * is covered by example_timer4. */
    h = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
    h.Instance = PWM_INSTANCE_7;
    h.Duty = 20U;
    h.Timer = PWM_TIMER_6;
    ok &= (EPIC_PWM_Init(&h) == EPIC_OK);
    t2_start();
    sample(7U, 40U, &high, &rises);
    ok &= (high == 0U) && (rises == 0U);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_7) == EPIC_OK);
    t2_stop();

    epic_harness_log("pwm: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
