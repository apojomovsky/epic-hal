#include "peripherals/pic16f193x_dac.h"
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    PIC8_REG8(PIC_REG_DACCON0) = PIC_DACCON0_DACEN;
    PIC8_REG8(PIC_REG_DACCON1) = (uint8_t)(h->OutputValue & PIC_DACCON1_DACR_MASK);
    return EPIC_OK;
}
EPIC_StatusTypeDef EPIC_DAC_DeInit(void)
{
    PIC8_REG8(PIC_REG_DACCON0) = 0x00U;
    PIC8_REG8(PIC_REG_DACCON1) = 0x00U;
    return EPIC_OK;
}
