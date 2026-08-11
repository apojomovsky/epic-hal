#include "peripherals/pic16f193x_fvr.h"
/**
 * @brief Enable the fixed voltage reference and program the ADC and
 *        comparator/DAC gains.
 * @param h handle with the ADC and comparator/DAC gain selections
 * @return EPIC_OK on success, EPIC_INVALID for a null handle
 */
EPIC_StatusTypeDef EPIC_FVR_Init(const FVR_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    uint8_t v = PIC_FVRCON_FVREN;
    v |= (uint8_t)(h->AdcGain & PIC_FVRCON_ADFVR_MASK);
    v |= (uint8_t)((h->CompDacGain << PIC_FVRCON_CDAFVR_SHIFT) & PIC_FVRCON_CDAFVR_MASK);
    EPIC_REG8(PIC_REG_FVRCON) = v;
    return EPIC_OK;
}
/**
 * @brief Disable the fixed voltage reference and reset FVRCON.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_FVR_DeInit(void)
{
    EPIC_REG8(PIC_REG_FVRCON) = 0x00U;
    return EPIC_OK;
}
/**
 * @brief Poll whether the fixed voltage reference output is stable.
 * @return 1 if ready, 0 while it is still stabilizing
 */
uint8_t EPIC_FVR_IsReady(void)
{
    return (EPIC_REG8(PIC_REG_FVRCON) & PIC_FVRCON_FVRRDY) ? 1U : 0U;
}
