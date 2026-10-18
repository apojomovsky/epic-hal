/**
 * Timer4 host-sim probe. Host-only: advances the TMR4 model through
 * epic_harness_tick, so it is not in the XC8 build. Covers ratio lookups,
 * Init/Start validation, the register images, the PR4 match period and
 * flag, prescaler and postscaler division, FOSC clocking, the match
 * callback through the shared dispatcher, the DeInit POR state, Timer4
 * running independently of Timer2, and the Timer3/Timer4 couple: CCP and
 * PWM6/PWM7 PWM on Timer4 stay idle until Timer4 starts, then run.
 *
 * Expected register image (FOSC/4, 1:1, 1:1, period 7, no callback, after
 * Init, before Start):
 *   T4CLKCON = 0x01   (CS=0001 FOSC/4)
 *   T4RST    = 0x00
 *   T4HLT    = 0x00   (software mode)
 *   T4CON    = 0x00   (ON=0, CKPS=000, OUTPS=0000)
 *   PR4      = 0x07
 *   PIE4     = 0x00   (TMR4IE clear without a callback)
 * After Start: T4CON = 0x80, TMR4 = 0.
 * Flag timing at 1:1/1:1: 7 ticks reach TMR4 7 without flagging, the 8th
 * clears TMR4 with TMR4IF. PWM with PR4 9 at 1:1 has a 10 tick period
 * (40 counts): the first period starts on tick 10 and every 4 counts of
 * duty are one tick high.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_ccp.h"
#include "peripherals/pic16f188xx_pwm.h"
#include "peripherals/pic16f188xx_timer2.h"
#include "peripherals/pic16f188xx_timer4.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer4 match callback that records its call.
 */
static void t4_cb(void)
{
    s_calls++;
}

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
 * @brief Report whether TMR4IF is set.
 * @return Non-zero while the flag is set.
 */
static unsigned int t4_flag(void)
{
    return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR4IF) != 0U;
}

/**
 * @brief Report whether TMR2IF is set.
 * @return Non-zero while the flag is set.
 */
static unsigned int t2_flag(void)
{
    return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U;
}

/**
 * @brief Init and start Timer4 free-running from FOSC/4, no callback.
 * @param period PR4 value.
 * @param prescaler T4CON prescaler.
 */
static void t4_start(uint8_t period, TIMER4_PrescalerTypeDef prescaler)
{
    TIMER4_HandleTypeDef h = TIMER4_HANDLE_DEFAULT;

    h.Period = period;
    h.Prescaler = prescaler;
    (void)EPIC_TIMER4_Init(&h);
    (void)EPIC_TIMER4_Start(&h);
}

/**
 * @brief Stop Timer4 and restore its POR state.
 * @return 1 when both calls succeeded.
 */
static int t4_stop(void)
{
    int ok = (EPIC_TIMER4_Stop() == EPIC_OK);

    ok &= (EPIC_TIMER4_DeInit() == EPIC_OK);
    return ok;
}

/**
 * @brief Run n ticks and count the high ticks and rising edges of an output.
 * @param read Reads the output level after each step.
 * @param n Ticks to run.
 * @param high Out: ticks the output read high after the step.
 * @param rises Out: low-to-high transitions seen.
 */
static void sample(uint8_t (*read)(void), unsigned int n, unsigned int *high,
                   unsigned int *rises)
{
    unsigned int i;
    uint8_t prev = read();

    *high = 0U;
    *rises = 0U;
    for (i = 0U; i < n; i++)
    {
        uint8_t now;

        step(1U);
        now = read();
        *high += now;
        if ((now != 0U) && (prev == 0U))
        {
            (*rises)++;
        }
        prev = now;
    }
}

/**
 * @brief Read the modelled CCP1 output.
 * @return 1 when high.
 */
static uint8_t ccp1_out(void)
{
    return pic16f188xx_sim_ccp_out(1U);
}

/**
 * @brief Read the modelled PWM6 output.
 * @return 1 when high.
 */
static uint8_t pwm6_out(void)
{
    return pic16f188xx_sim_pwm_out(6U);
}

/**
 * @brief Register images, period, ratios, clocking, callback and POR restore.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_timer(void)
{
    int ok = 1;
    TIMER4_HandleTypeDef h = TIMER4_HANDLE_DEFAULT;

    /* Ratio lookups, including the out-of-range fallbacks. */
    ok &= (EPIC_TIMER4_PrescalerToRatio(TIMER4_PRESCALER_1_1) == 1U);
    ok &= (EPIC_TIMER4_PrescalerToRatio(TIMER4_PRESCALER_1_16) == 16U);
    ok &= (EPIC_TIMER4_PrescalerToRatio(TIMER4_PRESCALER_1_128) == 128U);
    ok &= (EPIC_TIMER4_PrescalerToRatio((TIMER4_PrescalerTypeDef)8) == 1U);
    ok &= (EPIC_TIMER4_PostscalerToRatio(TIMER4_POSTSCALER_1_1) == 1U);
    ok &= (EPIC_TIMER4_PostscalerToRatio(TIMER4_POSTSCALER_1_16) == 16U);
    ok &= (EPIC_TIMER4_PostscalerToRatio((TIMER4_PostscalerTypeDef)16) == 1U);

    /* Validation: NULL and out-of-range codes are rejected unwritten. */
    ok &= (EPIC_TIMER4_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER4_Start(NULL) == EPIC_INVALID);
    h.ClockSource = (TIMER4_ClockSourceTypeDef)14;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_INVALID);
    h.ClockSource = TIMER4_CS_FOSC4;
    h.Prescaler = (TIMER4_PrescalerTypeDef)8;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_INVALID);
    h.Prescaler = TIMER4_PRESCALER_1_1;
    h.Postscaler = (TIMER4_PostscalerTypeDef)16;
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_INVALID);
    h.Postscaler = TIMER4_POSTSCALER_1_1;
    ok &= ((EPIC_REG8(PIC_REG_T4CON) & PIC_T4CON_ON) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T4CLKCON) == 0U);

    /* POR: PR4 reads 0xFF, everything else 0. A stale T4HLT or T4RST is
     * cleared by Init so no limit mode can gate the count. */
    ok &= (EPIC_TIMER4_ReadPeriod() == 0xFFU);
    EPIC_REG8(PIC_REG_T4HLT) = 0x03U;
    EPIC_REG8(PIC_REG_T4RST) = 0x05U;

    /* Init stages the control image without starting: ON clear,
     * interrupt disabled without a callback. */
    h.Period = 7U;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CLKCON) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_T4RST) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T4HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T4CON) == 0x00U);
    ok &= (EPIC_TIMER4_ReadPeriod() == 7U);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR4IE) == 0U);

    /* Start sets ON in one T4CON write with TMR4 zeroed. */
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CON) == 0x80U);
    ok &= (EPIC_TIMER4_ReadCounter() == 0U);

    /* Match period: 7 ticks reach TMR4 7 without flagging, the 8th clears
     * TMR4 with TMR4IF. Timer2 is off, so its flag never moves. */
    step(7U);
    ok &= (EPIC_TIMER4_ReadCounter() == 7U);
    ok &= (t4_flag() == 0U);
    step(1U);
    ok &= (EPIC_TIMER4_ReadCounter() == 0U);
    ok &= (t4_flag() != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_TMR4) == 1U);
    ok &= (t2_flag() == 0U);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR4);
    ok &= (t4_flag() == 0U);

    /* Stop freezes the counter; DeInit restores the POR state. */
    step(3U);
    ok &= (EPIC_TIMER4_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER4_ReadCounter() == 3U);
    step(5U);
    ok &= (EPIC_TIMER4_ReadCounter() == 3U);
    ok &= (EPIC_TIMER4_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T4HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T4CLKCON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T4RST) == 0x00U);
    ok &= (EPIC_TIMER4_ReadPeriod() == 0xFFU);
    ok &= (EPIC_TIMER4_ReadCounter() == 0U);
    ok &= (t4_flag() == 0U);

    /* Prescaler 1:4 and postscaler 1:2 at PR4 1: a match every 8 ticks, a
     * flag every 16. */
    h = (TIMER4_HandleTypeDef)TIMER4_HANDLE_DEFAULT;
    h.Prescaler = TIMER4_PRESCALER_1_4;
    h.Postscaler = TIMER4_POSTSCALER_1_2;
    h.Period = 1U;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CON) == 0xA1U);
    step(15U);
    ok &= (t4_flag() == 0U);
    step(1U);
    ok &= (t4_flag() != 0U);
    ok &= (t4_stop() != 0);

    /* Prescaler 1:128 at PR4 0 matches on every clock: flag per 128 ticks. */
    h = (TIMER4_HandleTypeDef)TIMER4_HANDLE_DEFAULT;
    h.Prescaler = TIMER4_PRESCALER_1_128;
    h.Period = 0U;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CON) == 0xF0U);
    step(127U);
    ok &= (t4_flag() == 0U);
    step(1U);
    ok &= (t4_flag() != 0U);
    ok &= (t4_stop() != 0);

    /* FOSC clocks four times per tick: PR4 7 matches every 2 ticks. */
    h = (TIMER4_HandleTypeDef)TIMER4_HANDLE_DEFAULT;
    h.ClockSource = TIMER4_CS_FOSC;
    h.Period = 7U;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T4CLKCON) == 0x02U);
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    step(1U);
    ok &= (t4_flag() == 0U);
    step(1U);
    ok &= (t4_flag() != 0U);
    ok &= (t4_stop() != 0);

    /* A clock row the model does not tick (the T4CKIPPS pin) holds the
     * counter. */
    h = (TIMER4_HandleTypeDef)TIMER4_HANDLE_DEFAULT;
    h.ClockSource = TIMER4_CS_T4CKIPPS;
    h.Period = 3U;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    step(20U);
    ok &= (EPIC_TIMER4_ReadCounter() == 0U) && (t4_flag() == 0U);
    ok &= (t4_stop() != 0);

    /* A nonzero T4HLT MODE (hardware limit modes are not modelled) holds
     * the counter. */
    t4_start(3U, TIMER4_PRESCALER_1_1);
    EPIC_REG8(PIC_REG_T4HLT) = 0x04U;
    step(20U);
    ok &= (EPIC_TIMER4_ReadCounter() == 0U) && (t4_flag() == 0U);
    ok &= (t4_stop() != 0);

    /* Match callback: Init enables TMR4IE and PEIE, the dispatcher clears
     * TMR4IF and runs the slot once per match. */
    s_calls = 0U;
    h = (TIMER4_HandleTypeDef)TIMER4_HANDLE_DEFAULT;
    h.Period = 3U;
    h.OverflowCallback = t4_cb;
    ok &= (EPIC_TIMER4_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR4IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER4_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (s_calls == 1U);
    ok &= (t4_flag() == 0U);
    step(4U);
    ok &= (s_calls == 2U);
    ok &= (EPIC_TIMER4_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER4_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR4IE) == 0U);
    step(8U);
    ok &= (s_calls == 2U);

    /* Timer4 and Timer2 run on their own periods and flags. */
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

        t2.Period = 7U;
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        t4_start(2U, TIMER4_PRESCALER_1_1);
        step(3U);
        ok &= (t4_flag() != 0U) && (t2_flag() == 0U);
        ok &= (EPIC_TIMER2_ReadCounter() == 3U) && (EPIC_TIMER4_ReadCounter() == 0U);
        step(5U);
        ok &= (t2_flag() != 0U);
        ok &= (t4_stop() != 0);
        ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    }
    return ok;
}

/**
 * @brief CCP and PWM6/PWM7 on the Timer3/Timer4 couple against Timer4.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_couple(void)
{
    int ok = 1;
    unsigned int high;
    unsigned int rises;
    CCP_HandleTypeDef c = CCP_HANDLE_DEFAULT;
    PWM_HandleTypeDef p = PWM_HANDLE_DEFAULT;

    /* CCP1 PWM tied to the Timer3/Timer4 couple: idle while Timer4 is off,
     * even with Timer2 running, then 20 counts of a 10 tick period once
     * Timer4 starts. */
    c.Instance = CCP_INSTANCE_1;
    c.Mode = CCP_MODE_PWM;
    c.Timer = CCP_TIMER_3_4;
    c.PWMDuty = 20U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0x8FU);
    ok &= ((EPIC_REG8(PIC_REG_CCPTMRS0) & PIC_CCPTMRS_TSEL_MASK) == 2U);
    step(40U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) == 0U);
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

        t2.Period = 9U;
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        sample(ccp1_out, 40U, &high, &rises);
        ok &= (high == 0U) && (rises == 0U);
        ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    }
    t4_start(9U, TIMER4_PRESCALER_1_1);
    step(9U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U) && ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) == 0U);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 1U) && ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) != 0U);
    step(4U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 1U);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U);
    sample(ccp1_out, 50U, &high, &rises);
    ok &= (high == 25U) && (rises == 5U);

    /* Timer4 stopping holds the output level; DeInit of the module drops it. */
    ok &= (EPIC_TIMER4_Stop() == EPIC_OK);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U);
    ok &= (EPIC_TIMER4_DeInit() == EPIC_OK);

    /* Timer4 clocked from FOSC instead of FOSC/4 does not run PWM. */
    c.PWMDuty = 20U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    {
        TIMER4_HandleTypeDef t4 = TIMER4_HANDLE_DEFAULT;

        t4.ClockSource = TIMER4_CS_FOSC;
        t4.Period = 9U;
        ok &= (EPIC_TIMER4_Init(&t4) == EPIC_OK);
        ok &= (EPIC_TIMER4_Start(&t4) == EPIC_OK);
        sample(ccp1_out, 40U, &high, &rises);
        ok &= (high == 0U) && (rises == 0U);
        ok &= (t4_stop() != 0);
    }
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);

    /* A capture or compare instance on the couple stays on Timer3, which
     * is not started here: Timer4 matches never fire it. */
    c = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    c.Instance = CCP_INSTANCE_2;
    c.Mode = CCP_MODE_COMPARE_SET;
    c.Timer = CCP_TIMER_3_4;
    c.CompareValue = 3U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    t4_start(9U, TIMER4_PRESCALER_1_1);
    step(30U);
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP2IF) == 0U);
    ok &= (t4_stop() != 0);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);

    /* PWM6 on Timer4: the same 10 tick period, 20 counts is 5 ticks high. */
    p.Instance = PWM_INSTANCE_6;
    p.Timer = PWM_TIMER_4;
    p.Duty = 20U;
    ok &= (EPIC_PWM_Init(&p) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCPTMRS1) & 0x0CU) == 0x08U);
    step(40U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 0U);
    t4_start(9U, TIMER4_PRESCALER_1_1);
    step(9U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 0U);
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 1U);
    sample(pwm6_out, 50U, &high, &rises);
    ok &= (high == 25U) && (rises == 5U);
    ok &= (t4_stop() != 0);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_6) == EPIC_OK);
    step(1U);
    ok &= (pic16f188xx_sim_pwm_out(6U) == 0U);
    return ok;
}

/**
 * @brief Timer4 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    /* The sim vectors timer callbacks only with GIE set, as on silicon. */
    EPIC_IRQ_Restore(1U);

    ok &= check_timer();
    ok &= check_couple();

    epic_harness_log("timer4: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
