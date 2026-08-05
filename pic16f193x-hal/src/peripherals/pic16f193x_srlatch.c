#include "peripherals/pic16f193x_srlatch.h"
HAL_StatusTypeDef HAL_SRLATCH_Enable(void)
{
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_SRCON0), PIC_SRCON0_SRLEN);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SRLATCH_Disable(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_SRCON0), PIC_SRCON0_SRLEN);
    return HAL_OK;
}
