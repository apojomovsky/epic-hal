#include "peripherals/pic16f193x_fvr.h"
HAL_StatusTypeDef HAL_FVR_Init(const FVR_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    uint8_t v = PIC_FVRCON_FVREN;
    v |= (uint8_t)(h->AdcGain & PIC_FVRCON_ADFVR_MASK);
    v |= (uint8_t)((h->CompDacGain << PIC_FVRCON_CDAFVR_SHIFT) & PIC_FVRCON_CDAFVR_MASK);
    PIC8_REG8(PIC_REG_FVRCON) = v;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FVR_DeInit(void)
{
    PIC8_REG8(PIC_REG_FVRCON) = 0x00U;
    return HAL_OK;
}
uint8_t HAL_FVR_IsReady(void)
{
    return (PIC8_REG8(PIC_REG_FVRCON) & PIC_FVRCON_FVRRDY) ? 1U : 0U;
}
