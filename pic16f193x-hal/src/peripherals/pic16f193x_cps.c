#include "peripherals/pic16f193x_cps.h"
/**
 * @brief Configure the capacitive sensing module from `h` and enable
 *        it.
 * @param h handle with channel and range settings
 * @return EPIC_OK on success, EPIC_INVALID for a null handle
 */
EPIC_StatusTypeDef EPIC_CPS_Init(const CPS_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    uint8_t con0 = PIC_CPSCON0_CPSON | PIC_CPSCON0_T0XCS;
    con0 |= (uint8_t)((h->Range << 2) & PIC_CPSCON0_CPSRNG_MASK);
    EPIC_REG8(PIC_REG_CPSCON0) = con0;
    EPIC_REG8(PIC_REG_CPSCON1) = (uint8_t)(h->Channel & PIC_CPSCON1_CPSCH_MASK);
    return EPIC_OK;
}
/**
 * @brief Disable the capacitive sensing module and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_CPS_DeInit(void)
{
    EPIC_REG8(PIC_REG_CPSCON0) = 0x00U;
    EPIC_REG8(PIC_REG_CPSCON1) = 0x00U;
    return EPIC_OK;
}
