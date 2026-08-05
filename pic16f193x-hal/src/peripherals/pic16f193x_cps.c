#include "peripherals/pic16f193x_cps.h"
EPIC_StatusTypeDef EPIC_CPS_Init(const CPS_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    uint8_t con0 = PIC_CPSCON0_CPSON | PIC_CPSCON0_T0XCS;
    con0 |= (uint8_t)((h->Range << 2) & PIC_CPSCON0_CPSRNG_MASK);
    EPIC_REG8(PIC_REG_CPSCON0) = con0;
    EPIC_REG8(PIC_REG_CPSCON1) = (uint8_t)(h->Channel & PIC_CPSCON1_CPSCH_MASK);
    return EPIC_OK;
}
EPIC_StatusTypeDef EPIC_CPS_DeInit(void)
{
    EPIC_REG8(PIC_REG_CPSCON0) = 0x00U;
    EPIC_REG8(PIC_REG_CPSCON1) = 0x00U;
    return EPIC_OK;
}
