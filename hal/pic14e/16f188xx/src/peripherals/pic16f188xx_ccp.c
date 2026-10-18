/**
 * CCP1..CCP5 driver, implementation (DS40001802D §30). Every per-instance
 * access branches on the instance first, so each branch names its register
 * literally and XC8 emits a bank select per access. The handlers keep no
 * cstack autos (ISR bank rule, family MANUAL.md); each callback slot is a
 * file-scope static written from main context only, stored before the
 * interrupt is armed.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_ccp.h"
#include "core/pic16f188xx_irq.h"

#define CCP_WRITE_CPRL(inst, value)                                             \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCPR2L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCPR3L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCPR4L) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCPR5L) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CPRH(inst, value)                                             \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCPR1H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCPR2H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCPR3H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCPR4H) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCPR5H) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CON(inst, value)                                              \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) epic_sfr_write8(PIC_REG_CCP1CON, value); \
        else if ((inst) == CCP_INSTANCE_2) epic_sfr_write8(PIC_REG_CCP2CON, value); \
        else if ((inst) == CCP_INSTANCE_3) epic_sfr_write8(PIC_REG_CCP3CON, value); \
        else if ((inst) == CCP_INSTANCE_4) epic_sfr_write8(PIC_REG_CCP4CON, value); \
        else                               epic_sfr_write8(PIC_REG_CCP5CON, value); \
    } while (0)

#define CCP_READ_CON(inst, out)                                                 \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) (out) = EPIC_REG8(PIC_REG_CCP1CON);  \
        else if ((inst) == CCP_INSTANCE_2) (out) = EPIC_REG8(PIC_REG_CCP2CON);  \
        else if ((inst) == CCP_INSTANCE_3) (out) = EPIC_REG8(PIC_REG_CCP3CON);  \
        else if ((inst) == CCP_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_CCP4CON);  \
        else                               (out) = EPIC_REG8(PIC_REG_CCP5CON);  \
    } while (0)

#define CCP_WRITE_CAP(inst, value)                                              \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCP1CAP) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCP2CAP) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCP3CAP) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCP4CAP) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCP5CAP) = (uint8_t)(value); \
    } while (0)

#define CCP_READ_CPRL(inst, out)                                                \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) (out) = EPIC_REG8(PIC_REG_CCPR1L);   \
        else if ((inst) == CCP_INSTANCE_2) (out) = EPIC_REG8(PIC_REG_CCPR2L);   \
        else if ((inst) == CCP_INSTANCE_3) (out) = EPIC_REG8(PIC_REG_CCPR3L);   \
        else if ((inst) == CCP_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_CCPR4L);   \
        else                               (out) = EPIC_REG8(PIC_REG_CCPR5L);   \
    } while (0)

#define CCP_READ_CPRH(inst, out)                                                \
    do {                                                                        \
        if      ((inst) == CCP_INSTANCE_1) (out) = EPIC_REG8(PIC_REG_CCPR1H);   \
        else if ((inst) == CCP_INSTANCE_2) (out) = EPIC_REG8(PIC_REG_CCPR2H);   \
        else if ((inst) == CCP_INSTANCE_3) (out) = EPIC_REG8(PIC_REG_CCPR3H);   \
        else if ((inst) == CCP_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_CCPR4H);   \
        else                               (out) = EPIC_REG8(PIC_REG_CCPR5H);   \
    } while (0)

/* ISR callback slots, written from main context only. Scalars, not array
 * elements: each handler references a fixed symbol. */
static void (*s_ccp1_cb)(void) = NULL;
static void (*s_ccp2_cb)(void) = NULL;
static void (*s_ccp3_cb)(void) = NULL;
static void (*s_ccp4_cb)(void) = NULL;
static void (*s_ccp5_cb)(void) = NULL;

/**
 * @brief Check that an instance number names a CCP module.
 * @param inst Instance to check.
 * @return 1 for 1..5, 0 otherwise.
 */
static uint8_t valid_instance(CCP_InstanceTypeDef inst)
{
    return (((unsigned int)inst >= (unsigned int)CCP_INSTANCE_1) &&
            ((unsigned int)inst <= (unsigned int)CCP_INSTANCE_5)) ? 1U : 0U;
}

/**
 * @brief Check that a mode code is defined.
 * @param mode Mode code to check.
 * @return 1 for 0x0..0xB and 0xF, 0 for the reserved 0xC..0xE and above.
 */
static uint8_t mode_ok(CCP_ModeTypeDef mode)
{
    return (((unsigned int)mode <= (unsigned int)CCP_MODE_COMPARE_PULSE_CLR_TMR) ||
            (mode == CCP_MODE_PWM)) ? 1U : 0U;
}

/**
 * @brief Map an instance to its interrupt source.
 * @param inst Instance, validated by the caller.
 * @return The PIC16F188XX_IRQn of the instance.
 */
static PIC16F188XX_IRQn irq_of(CCP_InstanceTypeDef inst)
{
    if (inst == CCP_INSTANCE_1) return PIC16F188XX_IRQ_CCP1;
    if (inst == CCP_INSTANCE_2) return PIC16F188XX_IRQ_CCP2;
    if (inst == CCP_INSTANCE_3) return PIC16F188XX_IRQ_CCP3;
    if (inst == CCP_INSTANCE_4) return PIC16F188XX_IRQ_CCP4;
    return PIC16F188XX_IRQ_CCP5;
}

/**
 * @brief Store the callback for an instance.
 * @param inst Instance, validated by the caller.
 * @param cb Callback, or NULL.
 */
static void set_callback(CCP_InstanceTypeDef inst, void (*cb)(void))
{
    if (inst == CCP_INSTANCE_1) s_ccp1_cb = cb;
    else if (inst == CCP_INSTANCE_2) s_ccp2_cb = cb;
    else if (inst == CCP_INSTANCE_3) s_ccp3_cb = cb;
    else if (inst == CCP_INSTANCE_4) s_ccp4_cb = cb;
    else s_ccp5_cb = cb;
}

/**
 * @brief Write one instance's timer couple into CCPTMRS0 or CCPTMRS1.
 * @details CCP1..CCP4 take bit pairs 1:0 to 7:6 of CCPTMRS0 and CCP5 takes
 * bits 1:0 of CCPTMRS1, which PWM6 and PWM7 share, so the other fields are
 * kept. Not ISR-reachable.
 * @param inst Instance, validated by the caller.
 * @param timer Timer couple value, 1..3.
 */
static void set_timer(CCP_InstanceTypeDef inst, uint8_t timer)
{
    if (inst == CCP_INSTANCE_5)
    {
        EPIC_REG8(PIC_REG_CCPTMRS1) = (uint8_t)((EPIC_REG8(PIC_REG_CCPTMRS1) &
            (uint8_t)~PIC_CCPTMRS_TSEL_MASK) | timer);
    }
    else
    {
        uint8_t shift = (uint8_t)(((unsigned int)inst - 1U) * 2U);

        EPIC_REG8(PIC_REG_CCPTMRS0) = (uint8_t)((EPIC_REG8(PIC_REG_CCPTMRS0) &
            (uint8_t)~(PIC_CCPTMRS_TSEL_MASK << shift)) | (uint8_t)(timer << shift));
    }
}

/**
 * @brief CCPxCON image for a mode: EN set for every mode but OFF, plus FMT.
 * @param mode Validated mode.
 * @param left Nonzero sets FMT (left-aligned PWM duty).
 * @return The CCPxCON value.
 */
static uint8_t con_for(CCP_ModeTypeDef mode, uint8_t left)
{
    uint8_t con = (uint8_t)((unsigned int)mode & PIC_CCP1CON_MODE_MASK);

    if (mode != CCP_MODE_OFF)
    {
        con |= PIC_CCP1CON_EN;
    }
    if (left != 0U)
    {
        con |= PIC_CCP1CON_FMT;
    }
    return con;
}

/**
 * @brief Write a 10-bit PWM duty into CCPRxH:L for an alignment.
 * @details FMT clear: duty bits 9:8 in CCPRxH<1:0>, 7:0 in CCPRxL. FMT set:
 * bits 9:2 in CCPRxH, bits 1:0 in CCPRxL<7:6> (DS40001802D Figure 30-5).
 * @param inst Instance, validated by the caller.
 * @param duty Duty, 0..1023.
 * @param left Nonzero for the left-aligned format.
 */
static void write_duty(CCP_InstanceTypeDef inst, uint16_t duty, uint8_t left)
{
    if (left != 0U)
    {
        CCP_WRITE_CPRH(inst, (uint8_t)(duty >> 2));
        CCP_WRITE_CPRL(inst, (uint8_t)((duty & 0x03U) << 6));
    }
    else
    {
        CCP_WRITE_CPRH(inst, (uint8_t)(duty >> 8));
        CCP_WRITE_CPRL(inst, (uint8_t)(duty & 0xFFU));
    }
}

/**
 * @brief Configure a CCP module from the handle.
 * @param h Handle with instance, mode, timer couple, source, values and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle,
 *         a reserved mode, a timer couple of 0 or above 3, a capture source
 *         above 7, or a PWM duty above 1023.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    PIC16F188XX_IRQn irq;

    if ((h == NULL) || (valid_instance(h->Instance) == 0U) ||
        (mode_ok(h->Mode) == 0U) ||
        ((unsigned int)h->Timer < (unsigned int)CCP_TIMER_1_2) ||
        ((unsigned int)h->Timer > (unsigned int)CCP_TIMER_5_6) ||
        ((unsigned int)h->CaptureSource > (unsigned int)CCP_CAPTURE_LC4OUT) ||
        ((h->Mode == CCP_MODE_PWM) && (h->PWMDuty > 1023U)))
    {
        return EPIC_INVALID;
    }
    irq = irq_of(h->Instance);
    EPIC_IRQ_DisableSrc(irq);
    CCP_WRITE_CON(h->Instance, 0U);
    set_callback(h->Instance, h->EventCallback);
    set_timer(h->Instance, (uint8_t)h->Timer);
    CCP_WRITE_CAP(h->Instance, (uint8_t)h->CaptureSource);
    if (h->Mode == CCP_MODE_PWM)
    {
        write_duty(h->Instance, h->PWMDuty, h->PWMLeftAligned);
    }
    else
    {
        CCP_WRITE_CPRH(h->Instance, (uint8_t)(h->CompareValue >> 8));
        CCP_WRITE_CPRL(h->Instance, (uint8_t)(h->CompareValue & 0xFFU));
    }
    EPIC_IRQ_ClearFlag(irq);
    if (h->EventCallback != NULL)
    {
        EPIC_IRQ_Enable(irq);
    }
    CCP_WRITE_CON(h->Instance, con_for(h->Mode, h->PWMLeftAligned));
    return EPIC_OK;
}

/**
 * @brief Reset one CCP module to its POR state.
 * @param inst Instance, 1..5.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    PIC16F188XX_IRQn irq;

    if (valid_instance(inst) == 0U)
    {
        return EPIC_INVALID;
    }
    irq = irq_of(inst);
    EPIC_IRQ_DisableSrc(irq);
    EPIC_IRQ_ClearFlag(irq);
    CCP_WRITE_CON(inst, 0U);
    CCP_WRITE_CAP(inst, 0U);
    set_timer(inst, (uint8_t)CCP_TIMER_1_2);
    set_callback(inst, NULL);
    return EPIC_OK;
}

/**
 * @brief Load a new 16-bit compare value, high byte first.
 * @param inst Instance, 1..5.
 * @param value Value compared against the timer.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance.
 */
EPIC_StatusTypeDef EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (valid_instance(inst) == 0U)
    {
        return EPIC_INVALID;
    }
    CCP_WRITE_CPRH(inst, (uint8_t)(value >> 8));
    CCP_WRITE_CPRL(inst, (uint8_t)(value & 0xFFU));
    return EPIC_OK;
}

/**
 * @brief Change only the mode field of CCPxCON.
 * @param inst Instance, 1..5.
 * @param mode New mode.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance or reserved mode.
 */
EPIC_StatusTypeDef EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    uint8_t con;

    if ((valid_instance(inst) == 0U) || (mode_ok(mode) == 0U))
    {
        return EPIC_INVALID;
    }
    CCP_READ_CON(inst, con);
    CCP_WRITE_CON(inst, (uint8_t)(con_for(mode, (uint8_t)((con & PIC_CCP1CON_FMT) != 0U))));
    return EPIC_OK;
}

/**
 * @brief Read the captured value, retrying until the high byte is stable.
 * @param inst Instance, 1..5.
 * @return The 16-bit CCPRxH:L value, or 0 for an out-of-range instance.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    uint8_t hi1;
    uint8_t lo;
    uint8_t hi2;

    if (valid_instance(inst) == 0U)
    {
        return 0U;
    }
    do
    {
        CCP_READ_CPRH(inst, hi1);
        CCP_READ_CPRL(inst, lo);
        CCP_READ_CPRH(inst, hi2);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi1 << 8) | lo);
}

/**
 * @brief Set the PWM duty in 10-bit counts, honouring FMT.
 * @param inst Instance, 1..5.
 * @param duty Duty in counts, 0..1023.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance or duty above 1023.
 */
EPIC_StatusTypeDef EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty)
{
    uint8_t con;

    if ((valid_instance(inst) == 0U) || (duty > 1023U))
    {
        return EPIC_INVALID;
    }
    CCP_READ_CON(inst, con);
    write_duty(inst, duty, (uint8_t)((con & PIC_CCP1CON_FMT) != 0U));
    return EPIC_OK;
}

/**
 * @brief Number of PWM duty counts in one period.
 * @param period PRx value, 0..255.
 * @return 4 x (period + 1).
 */
uint16_t EPIC_CCP_PWMPeriodCounts(uint8_t period)
{
    return (uint16_t)(((uint16_t)period + 1U) * 4U);
}

/**
 * @brief Read CCPxCON.OUT.
 * @param inst Instance, 1..5.
 * @return 1 when the output is high, 0 when low or the instance is out of range.
 */
uint8_t EPIC_CCP_GetOutput(CCP_InstanceTypeDef inst)
{
    uint8_t con;

    if (valid_instance(inst) == 0U)
    {
        return 0U;
    }
    CCP_READ_CON(inst, con);
    return ((con & PIC_CCP1CON_OUT) != 0U) ? 1U : 0U;
}

/**
 * @brief Acknowledge CCP1IF, then run the callback if one is set.
 * @details Clears CCP1IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void CCP1_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP1IF);
    if (s_ccp1_cb != NULL)
    {
        s_ccp1_cb();
    }
}

/**
 * @brief Acknowledge CCP2IF, then run the callback if one is set.
 */
void CCP2_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP2IF);
    if (s_ccp2_cb != NULL)
    {
        s_ccp2_cb();
    }
}

/**
 * @brief Acknowledge CCP3IF, then run the callback if one is set.
 */
void CCP3_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP3IF);
    if (s_ccp3_cb != NULL)
    {
        s_ccp3_cb();
    }
}

/**
 * @brief Acknowledge CCP4IF, then run the callback if one is set.
 */
void CCP4_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP4IF);
    if (s_ccp4_cb != NULL)
    {
        s_ccp4_cb();
    }
}

/**
 * @brief Acknowledge CCP5IF, then run the callback if one is set.
 */
void CCP5_IRQHandler(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP5IF);
    if (s_ccp5_cb != NULL)
    {
        s_ccp5_cb();
    }
}
