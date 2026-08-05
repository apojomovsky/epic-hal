#include "peripherals/pic16f193x_fvr.h"
EPIC_StatusTypeDef EPIC_FVR_Init(const FVR_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    uint8_t v = PIC_FVRCON_FVREN;
    v |= (uint8_t)(h->AdcGain & PIC_FVRCON_ADFVR_MASK);
    v |= (uint8_t)((h->CompDacGain << PIC_FVRCON_CDAFVR_SHIFT) & PIC_FVRCON_CDAFVR_MASK);
    PIC8_REG8(PIC_REG_FVRCON) = v;
    return EPIC_OK;
}
EPIC_StatusTypeDef EPIC_FVR_DeInit(void)
{
    PIC8_REG8(PIC_REG_FVRCON) = 0x00U;
    return EPIC_OK;
}
uint8_t EPIC_FVR_IsReady(void)
{
    return (PIC8_REG8(PIC_REG_FVRCON) & PIC_FVRCON_FVRRDY) ? 1U : 0U;
}
