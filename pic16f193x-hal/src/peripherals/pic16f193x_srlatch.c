#include "peripherals/pic16f193x_srlatch.h"
/**
 * @brief Enable the SR latch (sets SRCON0<SRLEN>).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Enable(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_SRCON0), PIC_SRCON0_SRLEN);
    return EPIC_OK;
}
/**
 * @brief Disable the SR latch (clears SRCON0<SRLEN>).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Disable(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_SRCON0), PIC_SRCON0_SRLEN);
    return EPIC_OK;
}
