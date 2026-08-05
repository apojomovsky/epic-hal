#include "peripherals/pic16f193x_cps.h"
HAL_StatusTypeDef HAL_CPS_Init(const CPS_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    uint8_t con0 = PIC_CPSCON0_CPSON | PIC_CPSCON0_T0XCS;
    con0 |= (uint8_t)((h->Range << 2) & PIC_CPSCON0_CPSRNG_MASK);
    PIC8_REG8(PIC_REG_CPSCON0) = con0;
    PIC8_REG8(PIC_REG_CPSCON1) = (uint8_t)(h->Channel & PIC_CPSCON1_CPSCH_MASK);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_CPS_DeInit(void)
{
    PIC8_REG8(PIC_REG_CPSCON0) = 0x00U;
    PIC8_REG8(PIC_REG_CPSCON1) = 0x00U;
    return HAL_OK;
}
