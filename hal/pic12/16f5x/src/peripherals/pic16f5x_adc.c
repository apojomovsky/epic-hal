/* 16F506 ADC implementation (DS41268D §9.0, Register 9-1/9-2).
 * Literal register tokens throughout (the §4.9 high-risk shape); no
 * IRQ hooks, this core has no vector. Only the 16F506 builds this
 * file. */

#include "peripherals/pic16f5x_adc.h"

#if PIC16F5X_FAMILY_HAS_COMP_ADC

/**
 * @brief Minimal ANS cover sampling the channel pin (Register 9-1:
 *        01 = AN2, 10 = AN2+AN0, 11 = AN2+AN1+AN0).
 * @param ch the channel to cover.
 * @return the ANS value, or 0xFF when no pin is sampled.
 */
static uint8_t adc_ans_cover(ADC_ChannelTypeDef ch)
{
    switch (ch)
    {
        case ADC_CHANNEL_AN0: return 0x2U;
        case ADC_CHANNEL_AN1: return 0x3U;
        case ADC_CHANNEL_AN2: return 0x1U;
        default:              return 0xFFU;
    }
}

/**
 * @brief Initialize the ADC from the handle.
 * @param h handle with channel and clock selection.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    uint8_t adcon0;
    uint8_t ans;

    if (h == NULL) return EPIC_INVALID;

    /* ADON on, GO clear, CHS and ADCS from the handle. ANS covers
     * the channel pin; the 0.6V channel keeps the current ANS
     * (the GPIO-cleared 0x00 when nothing sampled it before). */
    adcon0 = PIC_ADCON0_ADON;
    adcon0 |= (uint8_t)(((uint8_t)h->Channel & 0x3U) << PIC_ADCON0_CHS_POS);
    adcon0 |= (uint8_t)(((uint8_t)h->ClockSource & 0x3U) << PIC_ADCON0_ADCS_POS);
    ans = adc_ans_cover(h->Channel);
    if (ans != 0xFFU) adcon0 |= (uint8_t)(ans << PIC_ADCON0_ANS_POS);
    else adcon0 |= (uint8_t)(EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_ANS_MASK);
    EPIC_REG8(PIC_REG_ADCON0) = adcon0;

    return EPIC_OK;
}

/**
 * @brief Shut the ADC off and return its pins to digital.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void)
{
    EPIC_REG8(PIC_REG_ADCON0) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Switch the sampled channel and its ANS cover.
 * @param ch the channel to sample next.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch)
{
    uint8_t adcon0 = (uint8_t)(EPIC_REG8(PIC_REG_ADCON0) & (uint8_t)~PIC_ADCON0_CHS_MASK);
    uint8_t ans = adc_ans_cover(ch);

    adcon0 |= (uint8_t)(((uint8_t)ch & 0x3U) << PIC_ADCON0_CHS_POS);
    if (ans != 0xFFU)
    {
        adcon0 &= (uint8_t)~PIC_ADCON0_ANS_MASK;
        adcon0 |= (uint8_t)(ans << PIC_ADCON0_ANS_POS);
    }
    EPIC_REG8(PIC_REG_ADCON0) = adcon0;
}

/**
 * @brief Start a conversion by setting GO/DONE.
 * @return 0 on success, 0xFFFF if running or shut off.
 */
uint16_t EPIC_ADC_Start(void)
{
    uint8_t adcon0 = EPIC_REG8(PIC_REG_ADCON0);

    if ((adcon0 & PIC_ADCON0_ADON) == 0U) return 0xFFFFU;
    if ((adcon0 & PIC_ADCON0_GO) != 0U) return 0xFFFFU;
    EPIC_REG8(PIC_REG_ADCON0) = (uint8_t)(adcon0 | PIC_ADCON0_GO);
    return 0x0000U;
}

/**
 * @brief Report whether a conversion is in progress.
 * @return 1 if GO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void)
{
    return (EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO) ? 1U : 0U;
}

/**
 * @brief Read the last conversion result.
 * @return the ADRES byte (0..255, VDD reference).
 */
uint8_t EPIC_ADC_Read(void)
{
    return EPIC_REG8(PIC_REG_ADRES);
}

#endif /* PIC16F5X_FAMILY_HAS_COMP_ADC */
