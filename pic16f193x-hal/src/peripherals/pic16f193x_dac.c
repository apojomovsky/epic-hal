#include "peripherals/pic16f193x_dac.h"
HAL_StatusTypeDef HAL_DAC_Init(const DAC_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    PIC8_REG8(PIC_REG_DACCON0) = PIC_DACCON0_DACEN;
    PIC8_REG8(PIC_REG_DACCON1) = (uint8_t)(h->OutputValue & PIC_DACCON1_DACR_MASK);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_DeInit(void)
{
    PIC8_REG8(PIC_REG_DACCON0) = 0x00U;
    PIC8_REG8(PIC_REG_DACCON1) = 0x00U;
    return HAL_OK;
}
