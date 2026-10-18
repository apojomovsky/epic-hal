/**
 * PIC16F188xx ADC driver (DS40001802D §23). Basic polling mode: Init
 * programs ADCLK, ADREF, ADACT, ADCON1/ADCON2 (POR basic mode) and
 * ADCON0, then enables the module. Every access names its register
 * literally (adding-a-device.md §4 high-risk pattern).
 */

#include <stddef.h>
#include "peripherals/pic16f188xx_adc.h"
#include "core/pic16f188xx_irq.h"

/**
 * @brief Test whether an ADPCH code names a channel the part has.
 * @param channel ADPCH code to check.
 * @return 1 for a pin channel the part has or an internal channel.
 */
static uint8_t channel_ok(uint8_t channel)
{
    if (channel <= PIC16F188XX_FAMILY_ADC_MAX_PIN)
    {
        return 1U;
    }
    return (uint8_t)((channel >= ADC_CHANNEL_AVSS) && (channel <= ADC_CHANNEL_FVR));
}

/**
 * @brief Configure the ADC from `h` and enable the module.
 * @param h handle with clock, references, format and trigger
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    uint8_t con0;

    if (h == NULL)
    {
        return EPIC_INVALID;
    }

    EPIC_REG8(PIC_REG_ADCLK) = (uint8_t)(h->ClockDiv & PIC_ADCLK_ADCCS_MASK);
    EPIC_REG8(PIC_REG_ADREF) = (uint8_t)(((uint8_t)h->PosRef & PIC_ADREF_ADPREF_MASK) |
        (((uint8_t)h->NegRef << 4) & PIC_ADREF_ADNREF));
    EPIC_REG8(PIC_REG_ADACT) = (uint8_t)((uint8_t)h->Trigger & PIC_ADACT_ADACT_MASK);
    EPIC_REG8(PIC_REG_ADCON1) = 0x00U;
    EPIC_REG8(PIC_REG_ADCON2) = 0x00U;

    con0 = PIC_ADCON0_ADON;
    if (h->ClockSource == ADC_CLOCK_FRC)
    {
        con0 |= PIC_ADCON0_ADCS;
    }
    if (h->Format == ADC_FORMAT_RIGHT_JUSTIFIED)
    {
        con0 |= PIC_ADCON0_ADFM;
    }
    EPIC_REG8(PIC_REG_ADCON0) = con0;
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_ADC);
    return EPIC_OK;
}

/**
 * @brief Disable the ADC and return its registers to POR.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void)
{
    EPIC_REG8(PIC_REG_ADCON0) = 0x00U;
    EPIC_REG8(PIC_REG_ADCON1) = 0x00U;
    EPIC_REG8(PIC_REG_ADCON2) = 0x00U;
    EPIC_REG8(PIC_REG_ADREF) = 0x00U;
    EPIC_REG8(PIC_REG_ADCLK) = 0x00U;
    EPIC_REG8(PIC_REG_ADACT) = 0x00U;
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_ADC);
    return EPIC_OK;
}

/**
 * @brief Select the analog input channel for the next conversion.
 * @param channel ADPCH code: pin channels to
 *        PIC16F188XX_FAMILY_ADC_MAX_PIN, or one of
 *        ADC_CHANNEL_AVSS/TEMP/DAC/FVR
 * @return EPIC_OK on success, EPIC_INVALID for a channel the part lacks
 */
EPIC_StatusTypeDef EPIC_ADC_SelectChannel(uint8_t channel)
{
    if (channel_ok(channel) == 0U)
    {
        return EPIC_INVALID;
    }
    EPIC_REG8(PIC_REG_ADPCH) = (uint8_t)(channel & PIC_ADPCH_CHS_MASK);
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
 * @brief Read the last conversion result as a 10-bit value in both
 *        justification modes (Register 23-1: right justified lands in
 *        ADRESH:ADRESL, left justified in ADRESH:ADRESL<7:6>).
 * @return the 10-bit conversion result
 */
uint16_t EPIC_ADC_Read(void)
{
    uint8_t lo = EPIC_REG8(PIC_REG_ADRESL);
    uint8_t hi = EPIC_REG8(PIC_REG_ADRESH);
    uint16_t raw = (uint16_t)(((uint16_t)hi << 8) | lo);

    if ((EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_ADFM) == 0U)
    {
        raw >>= 6;
    }
    return raw;
}

/**
 * @brief ADC interrupt handler: clears ADIF (weak, override in user code).
 * @details Called from epic_dispatch_all_irqs only when ADIF and ADIE are
 * both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void ADC_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_ADC);
}
