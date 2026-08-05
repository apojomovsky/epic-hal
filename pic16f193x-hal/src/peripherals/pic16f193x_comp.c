/**
 * @file    pic16f193x_comp.c
 * @brief   PIC16F193X dual comparator driver (DS41364B §9.0).
 * @details Two instances, branch-before-touch per-instance access.
 */

#include "peripherals/pic16f193x_comp.h"
#include "core/pic16f193x_irq.h"

EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->PosChannel > 0x03U || h->NegChannel > 0x03U) return EPIC_INVALID;

    uint8_t con1 = (uint8_t)(h->NegChannel & 0x03U);
    con1 |= (uint8_t)((h->PosChannel << 4) & 0x30U);
    if (h->InterruptEdge & COMP_INT_EDGE_RISING)  con1 |= PIC8_BIT(7);
    if (h->InterruptEdge & COMP_INT_EDGE_FALLING) con1 |= PIC8_BIT(6);

    uint8_t con0 = PIC8_BIT(7);  /* CxON */
    if (h->HysteresisOn)  con0 |= PIC8_BIT(1);
    if (h->InvertOutput)  con0 |= PIC8_BIT(4);
    if (h->OutputToPin)   con0 |= PIC8_BIT(5);

    if (h->Instance == COMP_INSTANCE_1) {
        PIC8_REG8(PIC_REG_CM1CON1) = con1;
        PIC8_REG8(PIC_REG_CM1CON0) = con0;
    } else if (h->Instance == COMP_INSTANCE_2) {
        PIC8_REG8(PIC_REG_CM2CON1) = con1;
        PIC8_REG8(PIC_REG_CM2CON0) = con0;
    } else {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst)
{
    if (inst == COMP_INSTANCE_1) {
        PIC8_REG8(PIC_REG_CM1CON0) = 0x00U;
        PIC8_REG8(PIC_REG_CM1CON1) = 0x00U;
    } else if (inst == COMP_INSTANCE_2) {
        PIC8_REG8(PIC_REG_CM2CON0) = 0x00U;
        PIC8_REG8(PIC_REG_CM2CON1) = 0x00U;
    } else {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst)
{
    uint8_t cmout = PIC8_REG8(PIC_REG_CMOUT);
    if (inst == COMP_INSTANCE_1) return (cmout & PIC_CMOUT_MC1OUT) ? 1U : 0U;
    if (inst == COMP_INSTANCE_2) return (cmout & PIC_CMOUT_MC2OUT) ? 1U : 0U;
    return 0U;
}

void CMP1_IRQHandler(void) {}
void CMP2_IRQHandler(void) {}
