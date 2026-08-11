/**
 * PIC16F193X dual comparator driver implementation (DS41364B §9.0).
 * Two instances, branch-before-touch per-instance access.
 */

#include "peripherals/pic16f193x_comp.h"
#include "core/pic16f193x_irq.h"

/**
 * @brief Configure a comparator from `h` and enable it. Per-instance
 *        CMxCON0/CMxCON1 are written with branch-before-touch access.
 * @param h handle with instance, channel selection, hysteresis,
 *        output options and interrupt edge
 * @return EPIC_OK on success, EPIC_INVALID for a null handle, invalid
 *         channel or unknown instance
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->PosChannel > 0x03U || h->NegChannel > 0x03U) return EPIC_INVALID;

    uint8_t con1 = (uint8_t)(h->NegChannel & 0x03U);
    con1 |= (uint8_t)((h->PosChannel << 4) & 0x30U);
    if (h->InterruptEdge & COMP_INT_EDGE_RISING)  con1 |= EPIC_BIT(7);
    if (h->InterruptEdge & COMP_INT_EDGE_FALLING) con1 |= EPIC_BIT(6);

    uint8_t con0 = EPIC_BIT(7);  /* CxON */
    if (h->HysteresisOn)  con0 |= EPIC_BIT(1);
    if (h->InvertOutput)  con0 |= EPIC_BIT(4);
    if (h->OutputToPin)   con0 |= EPIC_BIT(5);

    if (h->Instance == COMP_INSTANCE_1) {
        EPIC_REG8(PIC_REG_CM1CON1) = con1;
        EPIC_REG8(PIC_REG_CM1CON0) = con0;
    } else if (h->Instance == COMP_INSTANCE_2) {
        EPIC_REG8(PIC_REG_CM2CON1) = con1;
        EPIC_REG8(PIC_REG_CM2CON0) = con0;
    } else {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

/**
 * @brief Disable a comparator by clearing its CMxCON0/CMxCON1 registers.
 * @param inst comparator instance (1-2)
 * @return EPIC_OK on success, EPIC_INVALID for an unknown instance
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst)
{
    if (inst == COMP_INSTANCE_1) {
        EPIC_REG8(PIC_REG_CM1CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM1CON1) = 0x00U;
    } else if (inst == COMP_INSTANCE_2) {
        EPIC_REG8(PIC_REG_CM2CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM2CON1) = 0x00U;
    } else {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

/**
 * @brief Read the comparator's current output level from CMOUT.
 * @param inst comparator instance (1-2)
 * @return 1 if the output is high, 0 if low (0 for an unknown instance)
 */
uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst)
{
    uint8_t cmout = EPIC_REG8(PIC_REG_CMOUT);
    if (inst == COMP_INSTANCE_1) return (cmout & PIC_CMOUT_MC1OUT) ? 1U : 0U;
    if (inst == COMP_INSTANCE_2) return (cmout & PIC_CMOUT_MC2OUT) ? 1U : 0U;
    return 0U;
}

/**
 * @brief Comparator 1 interrupt handler (weak, override in user code).
 */
void CMP1_IRQHandler(void) {}
/**
 * @brief Comparator 2 interrupt handler (weak, override in user code).
 */
void CMP2_IRQHandler(void) {}
