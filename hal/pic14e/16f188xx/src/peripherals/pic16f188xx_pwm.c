/**
 * PWM6 and PWM7 driver, implementation (DS40001802D §19). Every access
 * branches on the module first, so each branch names its register
 * literally. Nothing here is ISR-reachable.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_pwm.h"

/**
 * @brief Check that a module number names a PWM module.
 * @param inst Module to check.
 * @return 1 for 6 and 7, 0 otherwise.
 */
static uint8_t valid_instance(PWM_InstanceTypeDef inst)
{
    return ((inst == PWM_INSTANCE_6) || (inst == PWM_INSTANCE_7)) ? 1U : 0U;
}

/**
 * @brief Write a 10-bit duty into PWMxDCH (bits 9:2) and PWMxDCL (bits 1:0 in 7:6).
 * @param inst Module, validated by the caller.
 * @param duty Duty, 0..1023.
 */
static void write_duty(PWM_InstanceTypeDef inst, uint16_t duty)
{
    uint8_t dcl = (uint8_t)((duty & 0x03U) << PIC_PWMDCL_DC_SHIFT);
    uint8_t dch = (uint8_t)(duty >> 2);

    if (inst == PWM_INSTANCE_6)
    {
        EPIC_REG8(PIC_REG_PWM6DCH) = dch;
        EPIC_REG8(PIC_REG_PWM6DCL) = dcl;
    }
    else
    {
        EPIC_REG8(PIC_REG_PWM7DCH) = dch;
        EPIC_REG8(PIC_REG_PWM7DCL) = dcl;
    }
}

/**
 * @brief Write PWMxCON.
 * @param inst Module, validated by the caller.
 * @param value PWMxCON image.
 */
static void write_con(PWM_InstanceTypeDef inst, uint8_t value)
{
    if (inst == PWM_INSTANCE_6)
    {
        epic_sfr_write8(PIC_REG_PWM6CON, value);
    }
    else
    {
        epic_sfr_write8(PIC_REG_PWM7CON, value);
    }
}

/**
 * @brief Write one module's time base into its CCPTMRS1 field.
 * @details P6TSEL is bits 3:2 and P7TSEL bits 5:4; the CCP5 field in bits
 * 1:0 is kept.
 * @param inst Module, validated by the caller.
 * @param timer Time base value, 1..3.
 */
static void set_timer(PWM_InstanceTypeDef inst, uint8_t timer)
{
    uint8_t shift = (inst == PWM_INSTANCE_6) ? 2U : 4U;

    EPIC_REG8(PIC_REG_CCPTMRS1) = (uint8_t)((EPIC_REG8(PIC_REG_CCPTMRS1) &
        (uint8_t)~(PIC_CCPTMRS_TSEL_MASK << shift)) | (uint8_t)(timer << shift));
}

/**
 * @brief Configure a PWM module from the handle and enable it.
 * @param h Handle with module, time base, duty and polarity.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle,
 *         a time base of 0 or above 3, or a duty above 1023.
 */
EPIC_StatusTypeDef EPIC_PWM_Init(const PWM_HandleTypeDef *h)
{
    uint8_t con;

    if ((h == NULL) || (valid_instance(h->Instance) == 0U) ||
        ((unsigned int)h->Timer < (unsigned int)PWM_TIMER_2) ||
        ((unsigned int)h->Timer > (unsigned int)PWM_TIMER_6) ||
        (h->Duty > 1023U))
    {
        return EPIC_INVALID;
    }
    con = (h->ActiveLow != 0U) ? PIC_PWM6CON_POL : 0U;
    write_con(h->Instance, con);
    set_timer(h->Instance, (uint8_t)h->Timer);
    write_duty(h->Instance, h->Duty);
    write_con(h->Instance, (uint8_t)(con | PIC_PWM6CON_EN));
    return EPIC_OK;
}

/**
 * @brief Reset one PWM module to its POR state.
 * @param inst Module, 6 or 7.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range module.
 */
EPIC_StatusTypeDef EPIC_PWM_DeInit(PWM_InstanceTypeDef inst)
{
    if (valid_instance(inst) == 0U)
    {
        return EPIC_INVALID;
    }
    write_con(inst, 0U);
    write_duty(inst, 0U);
    set_timer(inst, (uint8_t)PWM_TIMER_2);
    return EPIC_OK;
}

/**
 * @brief Set the duty in 10-bit counts.
 * @param inst Module, 6 or 7.
 * @param duty Duty in counts, 0..1023.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range module or duty above 1023.
 */
EPIC_StatusTypeDef EPIC_PWM_SetDuty(PWM_InstanceTypeDef inst, uint16_t duty)
{
    if ((valid_instance(inst) == 0U) || (duty > 1023U))
    {
        return EPIC_INVALID;
    }
    write_duty(inst, duty);
    return EPIC_OK;
}

/**
 * @brief Read PWMxCON.OUT.
 * @param inst Module, 6 or 7.
 * @return 1 when the output reads high, 0 when low or the module is out of range.
 */
uint8_t EPIC_PWM_GetOutput(PWM_InstanceTypeDef inst)
{
    uint8_t con;

    if (valid_instance(inst) == 0U)
    {
        return 0U;
    }
    con = (inst == PWM_INSTANCE_6) ? EPIC_REG8(PIC_REG_PWM6CON) : EPIC_REG8(PIC_REG_PWM7CON);
    return ((con & PIC_PWM6CON_OUT) != 0U) ? 1U : 0U;
}
