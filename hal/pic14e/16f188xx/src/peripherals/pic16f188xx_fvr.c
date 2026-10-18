/**
 * PIC16F188xx Fixed Voltage Reference driver (DS40001802D §16). Init
 * programs FVRCON (enable plus the ADC and comparator/DAC gains).
 * Every access names its register literally (adding-a-device.md §4
 * high-risk pattern).
 */

#include <stddef.h>
#include "peripherals/pic16f188xx_fvr.h"

/**
 * @brief Enable the Fixed Voltage Reference and program the ADC and
 *        comparator/DAC gain selections from the handle.
 * @param h handle with ADC and comparator/DAC gain selections
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_FVR_Init(const FVR_HandleTypeDef *h)
{
    uint8_t v;

    if (h == NULL)
    {
        return EPIC_INVALID;
    }
    v = (uint8_t)(PIC_FVRCON_FVREN |
        ((uint8_t)h->AdcGain & PIC_FVRCON_ADFVR_MASK) |
        (((uint8_t)h->CompDacGain << PIC_FVRCON_CDAFVR_SHIFT) & PIC_FVRCON_CDAFVR_MASK));
    EPIC_REG8(PIC_REG_FVRCON) = v;
    return EPIC_OK;
}

/**
 * @brief Disable the Fixed Voltage Reference (clears FVRCON).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_FVR_DeInit(void)
{
    EPIC_REG8(PIC_REG_FVRCON) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Poll whether the Fixed Voltage Reference output is stable.
 * @return 1 when FVRRDY is set, 0 while the reference is stabilizing
 */
uint8_t EPIC_FVR_IsReady(void)
{
    return (EPIC_REG8(PIC_REG_FVRCON) & PIC_FVRCON_FVRRDY) ? 1U : 0U;
}
