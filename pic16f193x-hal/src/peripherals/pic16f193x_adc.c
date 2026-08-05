/**
 * @file    pic16f193x_adc.c
 * @brief   PIC16F193X ADC driver implementation (DS41364B ADC chapter).
 */

#include "peripherals/pic16f193x_adc.h"

EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    uint8_t con1 = (uint8_t)(h->PosRef & PIC_ADCON1_ADPREF_MASK);
    if (h->NegRef) con1 |= PIC_ADCON1_ADNREF;
    con1 |= (uint8_t)(((uint8_t)h->ClockSource << PIC_ADCON1_ADCS_SHIFT) & PIC_ADCON1_ADCS_MASK);
    if (h->Format == ADC_FORMAT_RIGHT_JUSTIFIED) con1 |= PIC_ADCON1_ADFM;
    PIC8_REG8(PIC_REG_ADCON1) = con1;

    PIC8_REG8(PIC_REG_ADCON0) = PIC_ADCON0_ADON;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_ADC_DeInit(void)
{
    PIC8_REG8(PIC_REG_ADCON0) = 0x00U;
    PIC8_REG8(PIC_REG_ADCON1) = 0x00U;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_ADC_SelectChannel(uint8_t channel)
{
    if (channel >= PIC16F193X_FAMILY_ADC_CH) return EPIC_INVALID;
    uint8_t con0 = PIC8_REG8(PIC_REG_ADCON0);
    con0 = (uint8_t)((con0 & (uint8_t)~PIC_ADCON0_CHS_MASK)
                    | (uint8_t)((channel << PIC_ADCON0_CHS_SHIFT) & PIC_ADCON0_CHS_MASK));
    PIC8_REG8(PIC_REG_ADCON0) = con0;
    return EPIC_OK;
}

void EPIC_ADC_Start(void)
{
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_ADCON0), PIC_ADCON0_GO_NDONE);
}

uint8_t EPIC_ADC_IsConversionDone(void)
{
    return (PIC8_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO_NDONE) ? 0U : 1U;
}

uint16_t EPIC_ADC_Read(void)
{
    uint8_t lo = PIC8_REG8(PIC_REG_ADRESL);
    uint8_t hi = PIC8_REG8(PIC_REG_ADRESH);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

void ADC_IRQHandler(void) {}
