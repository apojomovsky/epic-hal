/** PIC16F193X ADC driver implementation (DS41364B ADC chapter). */

#include "peripherals/pic16f193x_adc.h"

/**
 * @brief Configure the ADC from `h` and enable the module.
 * @param h handle with clock, references and result format
 * @return EPIC_OK on success, EPIC_ERROR for an invalid handle
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    uint8_t con1 = (uint8_t)(h->PosRef & PIC_ADCON1_ADPREF_MASK);
    if (h->NegRef) con1 |= PIC_ADCON1_ADNREF;
    con1 |= (uint8_t)(((uint8_t)h->ClockSource << PIC_ADCON1_ADCS_SHIFT) & PIC_ADCON1_ADCS_MASK);
    if (h->Format == ADC_FORMAT_RIGHT_JUSTIFIED) con1 |= PIC_ADCON1_ADFM;
    EPIC_REG8(PIC_REG_ADCON1) = con1;

    EPIC_REG8(PIC_REG_ADCON0) = PIC_ADCON0_ADON;

    return EPIC_OK;
}

/**
 * @brief Disable the ADC and return the module to its reset state.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void)
{
    EPIC_REG8(PIC_REG_ADCON0) = 0x00U;
    EPIC_REG8(PIC_REG_ADCON1) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Select the analog input channel for the next conversion.
 * @param channel channel number (0-15, depending on the part)
 * @return EPIC_OK on success, EPIC_ERROR for an invalid channel
 */
EPIC_StatusTypeDef EPIC_ADC_SelectChannel(uint8_t channel)
{
    if (channel >= PIC16F193X_FAMILY_ADC_CH) return EPIC_INVALID;
    uint8_t con0 = EPIC_REG8(PIC_REG_ADCON0);
    con0 = (uint8_t)((con0 & (uint8_t)~PIC_ADCON0_CHS_MASK)
                    | (uint8_t)((channel << PIC_ADCON0_CHS_SHIFT) & PIC_ADCON0_CHS_MASK));
    EPIC_REG8(PIC_REG_ADCON0) = con0;
    return EPIC_OK;
}

/**
 * @brief Start a conversion on the selected channel (no wait).
 */
void EPIC_ADC_Start(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_ADCON0), PIC_ADCON0_GO_NDONE);
}

/**
 * @brief Poll whether the current conversion has completed.
 * @return 1 if conversion is done, 0 while it is still running
 */
uint8_t EPIC_ADC_IsConversionDone(void)
{
    return (EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO_NDONE) ? 0U : 1U;
}

/**
 * @brief Read the last conversion result, right or left justified
 *        per the handle format.
 * @return the 10-bit conversion result
 */
uint16_t EPIC_ADC_Read(void)
{
    uint8_t lo = EPIC_REG8(PIC_REG_ADRESL);
    uint8_t hi = EPIC_REG8(PIC_REG_ADRESH);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/**
 * @brief ADC interrupt handler (weak, override in user code).
 */
void ADC_IRQHandler(void) {}
