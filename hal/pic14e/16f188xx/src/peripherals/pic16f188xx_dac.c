/**
 * PIC16F188xx DAC1 driver (DS40001802D §25). Init programs DAC1CON1
 * (5-bit code) and DAC1CON0 (enable, sources, pin enables). Every
 * access names its register literally (adding-a-device.md §4
 * high-risk pattern).
 */

#include <stddef.h>
#include "peripherals/pic16f188xx_dac.h"

/**
 * @brief Enable DAC1 and program the code, sources and pin enables.
 * @param h handle with the 5-bit code, sources and output enables
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle or a code
 *         above 31
 */
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h)
{
    uint8_t con0;

    if ((h == NULL) || (h->OutputValue > PIC_DAC1CON1_DAC1R_MASK))
    {
        return EPIC_INVALID;
    }
    EPIC_REG8(PIC_REG_DAC1CON1) = (uint8_t)(h->OutputValue & PIC_DAC1CON1_DAC1R_MASK);
    con0 = (uint8_t)(PIC_DAC1CON0_EN |
        (((uint8_t)h->PosSource << PIC_DAC1CON0_PSS_SHIFT) & PIC_DAC1CON0_PSS_MASK));
    if (h->NegSource != DAC_NEGSRC_VSS)
    {
        con0 |= PIC_DAC1CON0_NSS;
    }
    if ((h->OutputEnable & DAC_OUT1_ENABLE) != 0U)
    {
        con0 |= PIC_DAC1CON0_OE1;
    }
    if ((h->OutputEnable & DAC_OUT2_ENABLE) != 0U)
    {
        con0 |= PIC_DAC1CON0_OE2;
    }
    EPIC_REG8(PIC_REG_DAC1CON0) = con0;
    return EPIC_OK;
}

/**
 * @brief Disable DAC1 and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_DAC_DeInit(void)
{
    EPIC_REG8(PIC_REG_DAC1CON0) = 0x00U;
    EPIC_REG8(PIC_REG_DAC1CON1) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Change the output code while the DAC stays enabled.
 * @param value 5-bit code, DAC1R<4:0>
 * @return EPIC_OK on success, EPIC_INVALID for a code above 31
 */
EPIC_StatusTypeDef EPIC_DAC_SetValue(uint8_t value)
{
    if (value > PIC_DAC1CON1_DAC1R_MASK)
    {
        return EPIC_INVALID;
    }
    EPIC_REG8(PIC_REG_DAC1CON1) = (uint8_t)(value & PIC_DAC1CON1_DAC1R_MASK);
    return EPIC_OK;
}
