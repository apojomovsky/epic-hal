#include "peripherals/pic16f193x_dac.h"
/**
 * @brief Enable the DAC and load the initial output value.
 * @param h handle with the output value
 * @return EPIC_OK on success, EPIC_INVALID for a null handle
 */
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    EPIC_REG8(PIC_REG_DACCON0) = PIC_DACCON0_DACEN;
    EPIC_REG8(PIC_REG_DACCON1) = (uint8_t)(h->OutputValue & PIC_DACCON1_DACR_MASK);
    return EPIC_OK;
}
/**
 * @brief Disable the DAC and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_DAC_DeInit(void)
{
    EPIC_REG8(PIC_REG_DACCON0) = 0x00U;
    EPIC_REG8(PIC_REG_DACCON1) = 0x00U;
    return EPIC_OK;
}
