/**
 * CCP1..CCP5 host-sim probe. Host-only: advances the Timer1, Timer2 and
 * CCP models through epic_harness_tick, so it is not in the XC8 build.
 * Covers Init/SetMode/SetCompare/SetPWMDuty validation, the register
 * images of every instance, each compare mode against TMR1 (set, clear,
 * toggle, pulse, and the two that clear the timer), the CCPxIF flag and
 * the vectored callbacks of all five instances, capture on the CCPxPPS
 * pins (every edge, falling, rising, every 4th and 16th rising edge) with
 * a remapped input, and PWM against Timer2 (period, duty, both FMT
 * alignments, the end-of-period latch, 0% and 100% duty, prescaled time
 * base, and the timer couples the model does not run).
 *
 * Expected register image (CCP1, compare set mode, value 0x1234, Timer1/2
 * couple, no callback, after Init):
 *   CCP1CON  = 0x88   (EN, FMT clear, MODE=1000)
 *   CCPR1H   = 0x12, CCPR1L = 0x34
 *   CCP1CAP  = 0x00   (CTS=CCP1PPS)
 *   CCPTMRS0 = 0x55   (every CCPx on the Timer1/Timer2 couple)
 *   PIE6     = 0x00   (CCP1IE clear without a callback)
 * Match timing with Timer1 started at 0: the compare value N matches on
 * the Nth tick after Start.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_ccp.h"
#include "peripherals/pic16f188xx_pps.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"

/* Per-instance callback call counts, read by the checks in main(). */
static unsigned int s_calls[6];

/** @brief CCP1 callback that records its call. */
static void cb1(void)
{
    s_calls[1]++;
}

/** @brief CCP2 callback that records its call. */
static void cb2(void)
{
    s_calls[2]++;
}

/** @brief CCP3 callback that records its call. */
static void cb3(void)
{
    s_calls[3]++;
}

/** @brief CCP4 callback that records its call. */
static void cb4(void)
{
    s_calls[4]++;
}

/** @brief CCP5 callback that records its call. */
static void cb5(void)
{
    s_calls[5]++;
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
 * @brief Report whether an instance's CCPxIF flag is set.
 * @param inst Instance 1..5.
 * @return Non-zero while the flag is set.
 */
static unsigned int flag(unsigned int inst)
{
    return (EPIC_REG8(PIC_REG_PIR6) & (uint8_t)(1U << (inst - 1U))) != 0U;
}

/**
 * @brief Clear an instance's CCPxIF flag.
 * @param inst Instance 1..5.
 */
static void clear_flag(unsigned int inst)
{
    EPIC_REG8(PIC_REG_PIR6) &= (uint8_t)~(1U << (inst - 1U));
}

/**
 * @brief Start Timer1 from FOSC/4 at count 0.
 */
static void t1_start(void)
{
    TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;

    (void)EPIC_TIMER1_Init(&t1);
    (void)EPIC_TIMER1_Start(&t1);
}

/**
 * @brief Stop and reset Timer1.
 */
static void t1_stop(void)
{
    (void)EPIC_TIMER1_Stop();
    (void)EPIC_TIMER1_DeInit();
}

/**
 * @brief Init one CCP instance in a compare mode against Timer1.
 * @param inst Instance.
 * @param mode Compare mode.
 * @param value Compare value.
 * @return Init status.
 */
static EPIC_StatusTypeDef compare_init(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode,
                                       uint16_t value)
{
    CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

    h.Instance = inst;
    h.Mode = mode;
    h.CompareValue = value;
    return EPIC_CCP_Init(&h);
}

/**
 * @brief Init one CCP instance in a capture mode.
 * @param inst Instance.
 * @param mode Capture mode.
 * @return Init status.
 */
static EPIC_StatusTypeDef capture_init(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

    h.Instance = inst;
    h.Mode = mode;
    return EPIC_CCP_Init(&h);
}

/**
 * @brief Drive one pulse (high tick, then low tick) onto an input pin.
 * @param port Port letter.
 * @param pin Pin number.
 */
static void pulse(char port, uint8_t pin)
{
    pic16f188xx_sim_drive_input(port, pin, 1U);
    step(1U);
    pic16f188xx_sim_drive_input(port, pin, 0U);
    step(1U);
}

/**
 * @brief Run n ticks of a PWM and count high ticks and rising edges.
 * @param inst CCP instance 1..5, or 6 and 7 for the PWM modules.
 * @param n Ticks to run.
 * @param high Out: ticks the output read high after the step.
 * @param rises Out: low-to-high transitions seen.
 */
static void sample(unsigned int inst, unsigned int n, unsigned int *high, unsigned int *rises)
{
    unsigned int i;
    uint8_t prev = (inst >= 6U) ? pic16f188xx_sim_pwm_out((uint8_t)inst) :
        pic16f188xx_sim_ccp_out((uint8_t)inst);

    *high = 0U;
    *rises = 0U;
    for (i = 0U; i < n; i++)
    {
        uint8_t now;

        step(1U);
        now = (inst >= 6U) ? pic16f188xx_sim_pwm_out((uint8_t)inst) :
            pic16f188xx_sim_ccp_out((uint8_t)inst);
        *high += now;
        if ((now != 0U) && (prev == 0U))
        {
            (*rises)++;
        }
        prev = now;
    }
}

/**
 * @brief Init Timer2 free-running from FOSC/4 as a PWM time base.
 * @param period PR2 value.
 * @param prescaler Prescaler selection.
 */
static void t2_start(uint8_t period, TIMER2_PrescalerTypeDef prescaler)
{
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

    t2.Period = period;
    t2.Prescaler = prescaler;
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
 * @brief Init a PWM instance and start Timer2 with the same period.
 * @param inst Instance.
 * @param duty Duty in counts.
 * @param left Left-aligned format.
 * @param period PR2.
 * @param prescaler Timer2 prescaler.
 * @return Init status.
 */
static EPIC_StatusTypeDef pwm_start(CCP_InstanceTypeDef inst, uint16_t duty, uint8_t left,
                                    uint8_t period, TIMER2_PrescalerTypeDef prescaler)
{
    CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;
    EPIC_StatusTypeDef st;

    h.Instance = inst;
    h.Mode = CCP_MODE_PWM;
    h.PWMDuty = duty;
    h.PWMLeftAligned = left;
    st = EPIC_CCP_Init(&h);
    t2_start(period, prescaler);
    return st;
}

/**
 * @brief Validation, register images and POR restore of every instance.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_images(void)
{
    int ok = 1;
    CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;
    static const uint16_t con[5] = {PIC_REG_CCP1CON, PIC_REG_CCP2CON, PIC_REG_CCP3CON,
                                    PIC_REG_CCP4CON, PIC_REG_CCP5CON};
    static const uint16_t rh[5] = {PIC_REG_CCPR1H, PIC_REG_CCPR2H, PIC_REG_CCPR3H,
                                   PIC_REG_CCPR4H, PIC_REG_CCPR5H};
    static const uint16_t rl[5] = {PIC_REG_CCPR1L, PIC_REG_CCPR2L, PIC_REG_CCPR3L,
                                   PIC_REG_CCPR4L, PIC_REG_CCPR5L};
    static const uint16_t cap[5] = {PIC_REG_CCP1CAP, PIC_REG_CCP2CAP, PIC_REG_CCP3CAP,
                                    PIC_REG_CCP4CAP, PIC_REG_CCP5CAP};
    unsigned int i;

    ok &= (EPIC_CCP_Init(NULL) == EPIC_INVALID);
    h.Instance = (CCP_InstanceTypeDef)0;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.Instance = (CCP_InstanceTypeDef)6;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    h.Mode = (CCP_ModeTypeDef)0xCU;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.Mode = (CCP_ModeTypeDef)0x10U;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.Mode = CCP_MODE_COMPARE_SET;
    h.Timer = (CCP_TimerTypeDef)0U;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.Timer = (CCP_TimerTypeDef)4U;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.Timer = CCP_TIMER_1_2;
    h.CaptureSource = (CCP_CaptureSourceTypeDef)8U;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    h.CaptureSource = CCP_CAPTURE_CCPPPS;
    h.Mode = CCP_MODE_PWM;
    h.PWMDuty = 1024U;
    ok &= (EPIC_CCP_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, (CCP_ModeTypeDef)0xDU) == EPIC_INVALID);
    ok &= (EPIC_CCP_SetMode((CCP_InstanceTypeDef)0, CCP_MODE_OFF) == EPIC_INVALID);
    ok &= (EPIC_CCP_SetCompare((CCP_InstanceTypeDef)6, 1U) == EPIC_INVALID);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_1, 1024U) == EPIC_INVALID);
    ok &= (EPIC_CCP_SetPWMDuty((CCP_InstanceTypeDef)0, 1U) == EPIC_INVALID);
    ok &= (EPIC_CCP_DeInit((CCP_InstanceTypeDef)6) == EPIC_INVALID);
    ok &= (EPIC_CCP_GetCapture((CCP_InstanceTypeDef)6) == 0U);
    ok &= (EPIC_CCP_GetOutput((CCP_InstanceTypeDef)6) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0x55U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);

    /* Every instance takes the same image at its own registers. */
    for (i = 0U; i < 5U; i++)
    {
        h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
        h.Instance = (CCP_InstanceTypeDef)(i + 1U);
        h.Mode = CCP_MODE_COMPARE_SET;
        h.CompareValue = 0x1234U;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        ok &= (EPIC_REG8(con[i]) == 0x88U);
        ok &= (EPIC_REG8(rh[i]) == 0x12U);
        ok &= (EPIC_REG8(rl[i]) == 0x34U);
        ok &= (EPIC_REG8(cap[i]) == 0x00U);
        ok &= ((EPIC_REG8(PIC_REG_PIE6) & (uint8_t)(1U << i)) == 0U);
    }
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0x55U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);

    /* Timer couples and the capture source land in their fields only. */
    h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    h.Instance = CCP_INSTANCE_2;
    h.Mode = CCP_MODE_CAPTURE_RISING;
    h.Timer = CCP_TIMER_3_4;
    h.CaptureSource = CCP_CAPTURE_LC4OUT;
    ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0x59U);
    ok &= (EPIC_REG8(PIC_REG_CCP2CAP) == 0x07U);
    ok &= (EPIC_REG8(PIC_REG_CCP2CON) == 0x85U);
    h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    h.Instance = CCP_INSTANCE_4;
    h.Mode = CCP_MODE_COMPARE_TOGGLE;
    h.Timer = CCP_TIMER_5_6;
    ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0xD9U);
    h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    h.Instance = CCP_INSTANCE_5;
    h.Mode = CCP_MODE_COMPARE_TOGGLE;
    h.Timer = CCP_TIMER_5_6;
    ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x17U);

    /* DeInit restores each instance and only its timer field. */
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0xD5U);
    ok &= (EPIC_REG8(PIC_REG_CCP2CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCP2CAP) == 0x00U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_4) == EPIC_OK);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0x55U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x15U);
    for (i = 0U; i < 5U; i++)
    {
        ok &= (EPIC_CCP_DeInit((CCP_InstanceTypeDef)(i + 1U)) == EPIC_OK);
        ok &= (EPIC_REG8(con[i]) == 0x00U);
    }

    /* SetMode keeps the value and FMT; OFF drops EN. */
    ok &= (compare_init(CCP_INSTANCE_1, CCP_MODE_COMPARE_SET, 0x0102U) == EPIC_OK);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_COMPARE_CLEAR) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0x89U);
    ok &= (EPIC_REG8(PIC_REG_CCPR1H) == 0x01U);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_OFF) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0x00U);
    ok &= (EPIC_CCP_SetCompare(CCP_INSTANCE_1, 0xABCDU) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPR1H) == 0xABU);
    ok &= (EPIC_REG8(PIC_REG_CCPR1L) == 0xCDU);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    ok &= (EPIC_CCP_PWMPeriodCounts(0U) == 4U);
    ok &= (EPIC_CCP_PWMPeriodCounts(9U) == 40U);
    ok &= (EPIC_CCP_PWMPeriodCounts(255U) == 1024U);
    return ok;
}

/**
 * @brief Compare modes against a running Timer1.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_compare(void)
{
    int ok = 1;

    /* Set on match: the Nth tick matches value N, flags, drives high and
     * the OUT bit follows; the output stays high. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_1, CCP_MODE_COMPARE_SET, 10U) == EPIC_OK);
    step(9U);
    ok &= (flag(1U) == 0U) && (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 0U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 1U);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0xA8U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 1U);
    clear_flag(1U);
    step(20U);
    ok &= (flag(1U) == 0U) && (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 1U);

    /* Re-writing the same mode keeps the latch; changing it resets the
     * latch low. */
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_COMPARE_SET) == EPIC_OK);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 1U);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_COMPARE_CLEAR) == EPIC_OK);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 0U);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_OFF) == EPIC_OK);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 0U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* Clear on match flags but the latch is already low. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_2, CCP_MODE_COMPARE_CLEAR, 6U) == EPIC_OK);
    step(6U);
    ok &= (flag(2U) == 1U) && (EPIC_CCP_GetOutput(CCP_INSTANCE_2) == 0U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);
    t1_stop();

    /* Toggle with timer clear: TMR1 returns to 0 on each match, so the
     * output flips every 4 ticks. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_3, CCP_MODE_COMPARE_TOGGLE_CLR_TMR, 4U) == EPIC_OK);
    step(4U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 1U) && (EPIC_TIMER1_ReadCounter() == 0U);
    step(3U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 1U);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 0U) && (EPIC_TIMER1_ReadCounter() == 0U);
    step(4U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_3) == EPIC_OK);
    t1_stop();

    /* Plain toggle matches once per 16-bit wrap; the timer keeps counting. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_3, CCP_MODE_COMPARE_TOGGLE, 4U) == EPIC_OK);
    step(4U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 1U) && (EPIC_TIMER1_ReadCounter() == 4U);
    step(20U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_3) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_3) == EPIC_OK);
    t1_stop();

    /* Pulse: high for exactly one tick, then low; the flag stays set. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_4, CCP_MODE_COMPARE_PULSE, 5U) == EPIC_OK);
    step(5U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_4) == 1U) && (flag(4U) == 1U);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_4) == 0U) && (flag(4U) == 1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 6U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_4) == EPIC_OK);
    t1_stop();

    /* Pulse with timer clear repeats every 5 ticks. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_5, CCP_MODE_COMPARE_PULSE_CLR_TMR, 5U) == EPIC_OK);
    step(5U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_5) == 1U) && (EPIC_TIMER1_ReadCounter() == 0U);
    step(1U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_5) == 0U);
    clear_flag(5U);
    step(4U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_5) == 1U) && (flag(5U) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
    t1_stop();

    /* A halted timer cannot match again: the compare is an edge. */
    t1_start();
    ok &= (compare_init(CCP_INSTANCE_1, CCP_MODE_COMPARE_SET, 3U) == EPIC_OK);
    step(3U);
    ok &= (flag(1U) == 1U);
    clear_flag(1U);
    (void)EPIC_TIMER1_Stop();
    step(10U);
    ok &= (flag(1U) == 0U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* A timer couple the model does not run never matches. */
    {
        CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

        t1_start();
        h.Instance = CCP_INSTANCE_1;
        h.Mode = CCP_MODE_COMPARE_SET;
        h.Timer = CCP_TIMER_3_4;
        h.CompareValue = 3U;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        step(10U);
        ok &= (flag(1U) == 0U) && (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 0U);
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
        t1_stop();
    }
    return ok;
}

/**
 * @brief Vectored callbacks of all five instances through the dispatcher.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_interrupts(void)
{
    int ok = 1;
    CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;
    static void (*const cbs[5])(void) = {cb1, cb2, cb3, cb4, cb5};
    unsigned int i;

    t1_start();
    for (i = 0U; i < 5U; i++)
    {
        h = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
        h.Instance = (CCP_InstanceTypeDef)(i + 1U);
        h.Mode = CCP_MODE_COMPARE_SET;
        h.CompareValue = (uint16_t)(4U + i);
        h.EventCallback = cbs[i];
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE6) & (uint8_t)(1U << i)) != 0U);
    }
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    step(3U);
    ok &= (s_calls[1] == 0U);
    step(1U);
    ok &= (s_calls[1] == 1U) && (s_calls[2] == 0U) && (flag(1U) == 0U);
    step(1U);
    ok &= (s_calls[2] == 1U) && (s_calls[3] == 0U);
    step(1U);
    ok &= (s_calls[3] == 1U) && (s_calls[4] == 0U);
    step(1U);
    ok &= (s_calls[4] == 1U) && (s_calls[5] == 0U);
    step(1U);
    ok &= (s_calls[5] == 1U);
    step(10U);
    ok &= (s_calls[1] == 1U) && (s_calls[2] == 1U) && (s_calls[3] == 1U) &&
          (s_calls[4] == 1U) && (s_calls[5] == 1U);
    ok &= (EPIC_REG8(PIC_REG_PIR6) == 0x00U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_CCP3) == 0U);

    /* A flag set while masked stays pending and is readable. */
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_3) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE6) & 0x04U) == 0U);
    EPIC_REG8(PIC_REG_PIR6) |= 0x04U;
    step(3U);
    ok &= (s_calls[3] == 1U) && (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_CCP3) == 1U);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_CCP3);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_CCP3) == 0U);
    for (i = 0U; i < 5U; i++)
    {
        ok &= (EPIC_CCP_DeInit((CCP_InstanceTypeDef)(i + 1U)) == EPIC_OK);
    }
    ok &= (EPIC_REG8(PIC_REG_PIE6) == 0x00U);
    t1_stop();
    return ok;
}

/**
 * @brief Capture modes against the CCPxPPS pins.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_capture(void)
{
    int ok = 1;
    unsigned int i;

    /* Rising edge on RC2 (CCP1PPS at POR): the capture is the TMR1 count
     * of the tick that saw the edge, and the flag sets. */
    t1_start();
    ok &= (capture_init(CCP_INSTANCE_1, CCP_MODE_CAPTURE_RISING) == EPIC_OK);
    step(5U);
    ok &= (flag(1U) == 0U);
    pic16f188xx_sim_drive_input('C', 2U, 1U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 6U);
    clear_flag(1U);
    pic16f188xx_sim_drive_input('C', 2U, 0U);
    step(3U);
    ok &= (flag(1U) == 0U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 6U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* Falling edge only. */
    t1_start();
    pic16f188xx_sim_drive_input('C', 2U, 1U);
    step(1U);
    ok &= (capture_init(CCP_INSTANCE_1, CCP_MODE_CAPTURE_FALLING) == EPIC_OK);
    step(2U);
    ok &= (flag(1U) == 0U);
    pic16f188xx_sim_drive_input('C', 2U, 0U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 4U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* Every edge: both edges capture. */
    t1_start();
    ok &= (capture_init(CCP_INSTANCE_1, CCP_MODE_CAPTURE_ANY_EDGE) == EPIC_OK);
    pic16f188xx_sim_drive_input('C', 2U, 1U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 1U);
    clear_flag(1U);
    step(2U);
    pic16f188xx_sim_drive_input('C', 2U, 0U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 4U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* Every 4th rising edge. */
    t1_start();
    ok &= (capture_init(CCP_INSTANCE_1, CCP_MODE_CAPTURE_4TH) == EPIC_OK);
    for (i = 0U; i < 3U; i++)
    {
        pulse('C', 2U);
    }
    ok &= (flag(1U) == 0U);
    pic16f188xx_sim_drive_input('C', 2U, 1U);
    step(1U);
    ok &= (flag(1U) == 1U) && (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 7U);
    clear_flag(1U);
    pic16f188xx_sim_drive_input('C', 2U, 0U);
    step(1U);
    for (i = 0U; i < 3U; i++)
    {
        pulse('C', 2U);
    }
    ok &= (flag(1U) == 0U);
    pulse('C', 2U);
    ok &= (flag(1U) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* Every 16th rising edge. */
    t1_start();
    ok &= (capture_init(CCP_INSTANCE_1, CCP_MODE_CAPTURE_16TH) == EPIC_OK);
    for (i = 0U; i < 15U; i++)
    {
        pulse('C', 2U);
    }
    ok &= (flag(1U) == 0U);
    pulse('C', 2U);
    ok &= (flag(1U) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    t1_stop();

    /* A remapped input: CCP2 on RB3, and the RC1 POR pin no longer counts. */
    t1_start();
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP2, PPS_PORTB, 3U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x0BU);
    ok &= (capture_init(CCP_INSTANCE_2, CCP_MODE_CAPTURE_RISING) == EPIC_OK);
    pulse('C', 1U);
    ok &= (flag(2U) == 0U);
    pulse('B', 3U);
    ok &= (flag(2U) == 1U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);
    ok &= (EPIC_PPS_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x11U);
    t1_stop();

    /* Capture sources other than the pin are not modelled; capture on a
     * timer couple the model does not run never fires. */
    t1_start();
    {
        CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

        h.Instance = CCP_INSTANCE_1;
        h.Mode = CCP_MODE_CAPTURE_RISING;
        h.CaptureSource = CCP_CAPTURE_C1OUT;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        pulse('C', 2U);
        ok &= (flag(1U) == 0U);
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
        h.CaptureSource = CCP_CAPTURE_CCPPPS;
        h.Timer = CCP_TIMER_5_6;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        pulse('C', 2U);
        ok &= (flag(1U) == 0U);
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    }
    t1_stop();
    return ok;
}

/**
 * @brief PWM against Timer2: period, duty, alignment, latch and edges.
 * @return 1 when every check passes, 0 otherwise.
 */
static int check_pwm(void)
{
    int ok = 1;
    unsigned int high;
    unsigned int rises;

    /* PR2 9 at 1:1 is a 10 tick period (40 counts); 20 counts is 5 ticks
     * high. The first period starts at the first PR2 match, tick 10. */
    ok &= (pwm_start(CCP_INSTANCE_1, 20U, 0U, 9U, TIMER2_PRESCALER_1_1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1CON) == 0x8FU);
    ok &= (EPIC_REG8(PIC_REG_CCPR1H) == 0x00U) && (EPIC_REG8(PIC_REG_CCPR1L) == 20U);
    step(9U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U) && (flag(1U) == 0U);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 1U) && (flag(1U) == 1U);
    clear_flag(1U);
    step(4U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 1U);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(1U) == 0U);
    sample(1U, 50U, &high, &rises);
    ok &= (high == 25U) && (rises == 5U);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);

    /* Left-aligned duty keeps the same 10-bit value in the other layout. */
    ok &= (pwm_start(CCP_INSTANCE_2, 0x155U, 1U, 255U, TIMER2_PRESCALER_1_1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP2CON) == 0x9FU);
    ok &= (EPIC_REG8(PIC_REG_CCPR2H) == 0x55U) && (EPIC_REG8(PIC_REG_CCPR2L) == 0x40U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_2, 0x3FFU) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPR2H) == 0xFFU) && (EPIC_REG8(PIC_REG_CCPR2L) == 0xC0U);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);
    ok &= (pwm_start(CCP_INSTANCE_2, 0x155U, 0U, 255U, TIMER2_PRESCALER_1_1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP2CON) == 0x8FU);
    ok &= (EPIC_REG8(PIC_REG_CCPR2H) == 0x01U) && (EPIC_REG8(PIC_REG_CCPR2L) == 0x55U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_2, 0x3FFU) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPR2H) == 0x03U) && (EPIC_REG8(PIC_REG_CCPR2L) == 0xFFU);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);

    /* The duty latches at the period boundary: a write mid-period takes
     * effect on the next one. 0 counts never raise the output, 40 counts
     * (a full period) hold it high. */
    ok &= (pwm_start(CCP_INSTANCE_3, 20U, 0U, 9U, TIMER2_PRESCALER_1_1) == EPIC_OK);
    step(12U);
    ok &= (pic16f188xx_sim_ccp_out(3U) == 1U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_3, 4U) == EPIC_OK);
    sample(3U, 8U, &high, &rises);
    ok &= (high == 3U);
    sample(3U, 10U, &high, &rises);
    ok &= (high == 1U) && (rises == 1U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_3, 0U) == EPIC_OK);
    step(10U);
    sample(3U, 30U, &high, &rises);
    ok &= (high == 0U) && (rises == 0U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_3, EPIC_CCP_PWMPeriodCounts(9U)) == EPIC_OK);
    step(10U);
    sample(3U, 30U, &high, &rises);
    ok &= (high == 30U) && (rises == 0U);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_3) == EPIC_OK);

    /* A 1:4 prescaler makes every count one tick: a 40 tick period, and
     * 10 counts are 10 ticks high. */
    ok &= (pwm_start(CCP_INSTANCE_4, 10U, 0U, 9U, TIMER2_PRESCALER_1_4) == EPIC_OK);
    step(40U);
    sample(4U, 80U, &high, &rises);
    ok &= (high == 20U) && (rises == 2U);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_4) == EPIC_OK);

    /* The output holds its level when Timer2 stops, and the module reads
     * low after DeInit. A Timer2 clock other than FOSC/4 does not run it. */
    ok &= (pwm_start(CCP_INSTANCE_5, 20U, 0U, 9U, TIMER2_PRESCALER_1_1) == EPIC_OK);
    step(12U);
    ok &= (pic16f188xx_sim_ccp_out(5U) == 1U);
    (void)EPIC_TIMER2_Stop();
    step(30U);
    ok &= (pic16f188xx_sim_ccp_out(5U) == 1U);
    t2_stop();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
    step(1U);
    ok &= (pic16f188xx_sim_ccp_out(5U) == 0U);
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
        CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

        t2.ClockSource = TIMER2_CS_HFINTOSC;
        t2.Period = 9U;
        h.Instance = CCP_INSTANCE_1;
        h.Mode = CCP_MODE_PWM;
        h.PWMDuty = 20U;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        sample(1U, 40U, &high, &rises);
        ok &= (high == 0U) && (rises == 0U);
        t2_stop();
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    }

    /* A PWM instance tied to Timer4 is idle in the model. */
    {
        CCP_HandleTypeDef h = CCP_HANDLE_DEFAULT;

        h.Instance = CCP_INSTANCE_1;
        h.Mode = CCP_MODE_PWM;
        h.Timer = CCP_TIMER_3_4;
        h.PWMDuty = 20U;
        ok &= (EPIC_CCP_Init(&h) == EPIC_OK);
        t2_start(9U, TIMER2_PRESCALER_1_1);
        sample(1U, 40U, &high, &rises);
        ok &= (high == 0U) && (rises == 0U);
        t2_stop();
        ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    }
    return ok;
}

/**
 * @brief CCP checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    /* The sim vectors callbacks only with GIE set, as on silicon. */
    EPIC_IRQ_Restore(1U);

    ok &= check_images();
    ok &= check_compare();
    ok &= check_interrupts();
    ok &= check_capture();
    ok &= check_pwm();

    epic_harness_log("ccp: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
