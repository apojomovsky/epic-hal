/**
 * PIC16F193X CCP1-5 driver implementation (DS41364B §15.0). CCP1-3 are
 * Enhanced CCP, CCP4/5 are plain. All capture/compare only this phase
 * (PWM rejected by Init). Branch-before-touch per-instance macros, 5
 * instances.
 */

#include "peripherals/pic16f193x_ccp.h"
#include "core/pic16f193x_irq.h"

#define CCP_WRITE_CPRL(inst, value)                                     \
    do {                                                                \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCPR1L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCPR2L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCPR3L) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCPR4L) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCPR5L) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CPRH(inst, value)                                     \
    do {                                                                \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCPR1H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCPR2H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCPR3H) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCPR4H) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCPR5H) = (uint8_t)(value); \
    } while (0)

#define CCP_WRITE_CON(inst, value)                                      \
    do {                                                                \
        if      ((inst) == CCP_INSTANCE_1) EPIC_REG8(PIC_REG_CCP1CON) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_2) EPIC_REG8(PIC_REG_CCP2CON) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_3) EPIC_REG8(PIC_REG_CCP3CON) = (uint8_t)(value); \
        else if ((inst) == CCP_INSTANCE_4) EPIC_REG8(PIC_REG_CCP4CON) = (uint8_t)(value); \
        else                               EPIC_REG8(PIC_REG_CCP5CON) = (uint8_t)(value); \
    } while (0)

#define CCP_READ_CPRL(inst, out)                                        \
    do {                                                                \
        if      ((inst) == CCP_INSTANCE_1) (out) = EPIC_REG8(PIC_REG_CCPR1L); \
        else if ((inst) == CCP_INSTANCE_2) (out) = EPIC_REG8(PIC_REG_CCPR2L); \
        else if ((inst) == CCP_INSTANCE_3) (out) = EPIC_REG8(PIC_REG_CCPR3L); \
        else if ((inst) == CCP_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_CCPR4L); \
        else                               (out) = EPIC_REG8(PIC_REG_CCPR5L); \
    } while (0)

#define CCP_READ_CPRH(inst, out)                                        \
    do {                                                                \
        if      ((inst) == CCP_INSTANCE_1) (out) = EPIC_REG8(PIC_REG_CCPR1H); \
        else if ((inst) == CCP_INSTANCE_2) (out) = EPIC_REG8(PIC_REG_CCPR2H); \
        else if ((inst) == CCP_INSTANCE_3) (out) = EPIC_REG8(PIC_REG_CCPR3H); \
        else if ((inst) == CCP_INSTANCE_4) (out) = EPIC_REG8(PIC_REG_CCPR4H); \
        else                               (out) = EPIC_REG8(PIC_REG_CCPR5H); \
    } while (0)

/**
 * @brief Map a CCP instance to its interrupt request number.
 * @param inst CCP instance (1-5)
 * @return the PIC16F193X_IRQn for the instance
 */
static PIC16F193X_IRQn ccp_irq(CCP_InstanceTypeDef inst)
{
    if (inst == CCP_INSTANCE_1) return PIC16F193X_IRQ_CCP1;
    if (inst == CCP_INSTANCE_2) return PIC16F193X_IRQ_CCP2;
    if (inst == CCP_INSTANCE_3) return PIC16F193X_IRQ_CCP3;
    if (inst == CCP_INSTANCE_4) return PIC16F193X_IRQ_CCP4;
    return PIC16F193X_IRQ_CCP5;
}

/**
 * @brief Convert an instance number to the 0-based g_handle index.
 * @param inst CCP instance (1-5)
 * @return index into g_handle (0-4)
 */
static int idx_of(CCP_InstanceTypeDef inst)
{
    return (int)inst - 1;  /* 1->0, 2->1, 3->2, 4->3, 5->4 */
}

static const CCP_HandleTypeDef *g_handle[5] = { NULL, NULL, NULL, NULL, NULL };

/**
 * @brief Check whether a CCP instance number is supported.
 * @param inst CCP instance to validate
 * @return 1 if in range CCP_INSTANCE_1..CCP_INSTANCE_5, 0 otherwise
 */
static int valid_instance(CCP_InstanceTypeDef inst)
{
    return (inst >= CCP_INSTANCE_1 && inst <= CCP_INSTANCE_5);
}

/**
 * @brief Configure a CCP module from `h` and arm its interrupt if a
 *        callback is registered. PWM mode is rejected this phase.
 * @param h handle with instance, mode and compare value
 * @return EPIC_OK on success, EPIC_INVALID for a null handle, PWM mode
 *         or out-of-range instance
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->Mode == CCP_MODE_PWM) return EPIC_INVALID;
    if (!valid_instance(h->Instance)) return EPIC_INVALID;

    CCP_WRITE_CPRH(h->Instance, (uint8_t)(h->CompareValue >> 8));
    CCP_WRITE_CPRL(h->Instance, (uint8_t)(h->CompareValue & 0xFFU));
    CCP_WRITE_CON(h->Instance, (uint8_t)((uint8_t)h->Mode & PIC_CCP1CON_CCPM_MASK));

    PIC16F193X_IRQn irq = ccp_irq(h->Instance);
    EPIC_IRQ_ClearFlag(irq);
    if (h->EventCallback) {
        EPIC_IRQ_Enable(irq);
    } else {
        EPIC_IRQ_DisableSrc(irq);
    }

    g_handle[idx_of(h->Instance)] = h;
    return EPIC_OK;
}

/**
 * @brief Disable a CCP module's interrupt, clear its flag, zero its
 *        control register and drop the stored handle.
 * @param inst CCP instance (1-5)
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst)
{
    if (!valid_instance(inst)) return EPIC_INVALID;
    PIC16F193X_IRQn irq = ccp_irq(inst);
    EPIC_IRQ_DisableSrc(irq);
    EPIC_IRQ_ClearFlag(irq);
    CCP_WRITE_CON(inst, 0x00U);
    g_handle[idx_of(inst)] = NULL;
    return EPIC_OK;
}

/**
 * @brief Load a new compare value into a CCP module's CCPRxL/H pair.
 * @param inst CCP instance (1-5)
 * @param value 16-bit compare value to write
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance
 */
EPIC_StatusTypeDef EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value)
{
    if (!valid_instance(inst)) return EPIC_INVALID;
    CCP_WRITE_CPRH(inst, (uint8_t)(value >> 8));
    CCP_WRITE_CPRL(inst, (uint8_t)(value & 0xFFU));
    return EPIC_OK;
}

/**
 * @brief Change only CCPxCON's mode field, leaving CCPRx and IRQ state
 *        untouched.
 * @param inst CCP instance (1-5)
 * @param mode capture/compare mode to select
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode)
{
    if (!valid_instance(inst)) return;
    CCP_WRITE_CON(inst, (uint8_t)((uint8_t)mode & PIC_CCP1CON_CCPM_MASK));
}

/**
 * @brief Read the last captured value, retrying until the high byte is
 *        stable across the 16-bit read.
 * @param inst CCP instance (1-5)
 * @return the captured 16-bit value, 0 for an out-of-range instance
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst)
{
    if (!valid_instance(inst)) return 0U;
    uint8_t hi1, lo, hi2;
    do {
        CCP_READ_CPRH(inst, hi1);
        CCP_READ_CPRL(inst, lo);
        CCP_READ_CPRH(inst, hi2);
    } while (hi1 != hi2);
    return (uint16_t)(((uint16_t)hi1 << 8) | lo);
}

/**
 * @brief Shared CCP ISR body: clear the instance's flag in PIR1/PIR2/
 *        PIR3, then fire the registered event callback if any.
 * @param inst CCP instance (1-5)
 * @param irq interrupt request number matching the instance
 */
static void ccp_irq_common(CCP_InstanceTypeDef inst, PIC16F193X_IRQn irq)
{
    switch (irq) {
    case PIC16F193X_IRQ_CCP1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_CCP1IF); break;
    case PIC16F193X_IRQ_CCP2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CCP2IF); break;
    case PIC16F193X_IRQ_CCP3: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CCP3IF); break;
    case PIC16F193X_IRQ_CCP4: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CCP4IF); break;
    default: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CCP5IF); break;
    }
    const CCP_HandleTypeDef *h = g_handle[idx_of(inst)];
    if (h && h->EventCallback) {
        h->EventCallback();
    }
}

/**
 * @brief CCP1 interrupt handler (weak, override in user code).
 */
void CCP1_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_1, PIC16F193X_IRQ_CCP1); }
/**
 * @brief CCP2 interrupt handler (weak, override in user code).
 */
void CCP2_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_2, PIC16F193X_IRQ_CCP2); }
/**
 * @brief CCP3 interrupt handler (weak, override in user code).
 */
void CCP3_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_3, PIC16F193X_IRQ_CCP3); }
/**
 * @brief CCP4 interrupt handler (weak, override in user code).
 */
void CCP4_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_4, PIC16F193X_IRQ_CCP4); }
/**
 * @brief CCP5 interrupt handler (weak, override in user code).
 */
void CCP5_IRQHandler(void) { ccp_irq_common(CCP_INSTANCE_5, PIC16F193X_IRQ_CCP5); }
