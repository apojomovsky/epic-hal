/* A/D converter driver implementation (DS39582B §11.0). */

#include "peripherals/pic16f87xa_adc.h"
#include "core/pic16_irq.h"

static const ADC_HandleTypeDef *g_adc = NULL;

/**
 * @brief Initialize the A/D converter: program ADCON0/ADCON1 from the
 *        handle and arm the conversion-complete interrupt if a callback
 *        is set.
 * @param h handle with Channel, ClockSource, ResultFormat, Reference,
 *        ConvCpltCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_adc = h;

    /* ADCON0, Bank 0, address 0x1F.
     *   bit 0    ADON
     *   bit 2    GO/DONE (clear, not started yet)
     *   bit 5:3  CHS2:CHS0
     *   bit 7:6  ADCS1:ADCS0
     */
    uint8_t adcon0 = PIC_ADCON0_ADON;
    adcon0 |= (uint8_t)((h->Channel & 0x7U) << PIC_ADCON0_CHS_POS);
    adcon0 |= (uint8_t)((h->ClockSource & 0x3U) << PIC_ADCON0_ADCS_POS);
    EPIC_REG8(0x1FU) = adcon0;

    /* ADCON1, Bank 1, address 0x9F.
     *   bit 3:0  PCFG3:PCFG0
     *   bit 6    ADCS2
     *   bit 7    ADFM
     */
    uint8_t adcon1 = h->Reference & PIC_ADCON1_PCFG_MASK;
    if (h->ClockSource >= ADC_CLOCK_FOSC_4) {
        /* ADCS2 = 1 for the four high clock modes. */
        adcon1 |= PIC_ADCON1_ADCS2;
    }
    if (h->ResultFormat == ADC_FORMAT_RIGHT) adcon1 |= PIC_ADCON1_ADFM;
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch RMW here
     * silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(ADCON1, adcon1);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9FU) = adcon1;
        pic_select_bank(prev);
    }
#endif

    /* Interrupt enable. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ADC);
    if (h->ConvCpltCallback) EPIC_IRQ_Enable(PIC16_IRQ_ADC);
    else                     EPIC_IRQ_DisableSrc(PIC16_IRQ_ADC);

    return EPIC_OK;
}

/**
 * @brief De-initialize the A/D converter: disable its interrupt, clear
 *        the pending flag, and reset ADCON0/ADCON1.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_ADC);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ADC);
    EPIC_REG8(0x1FU) = 0x00U;
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(0x9FU) = 0x00U;
        pic_select_bank(prev);
    }
    g_adc = NULL;
    return EPIC_OK;
}

/**
 * @brief Select the analog channel without starting a conversion.
 * @param ch the analog channel to select.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch)
{
    uint8_t v = EPIC_REG8(0x1FU);
    v = (uint8_t)((v & (uint8_t)~PIC_ADCON0_CHS_MASK) |
                  ((uint8_t)(ch & 0x7U) << PIC_ADCON0_CHS_POS));
    EPIC_REG8(0x1FU) = v;
}

/**
 * @brief Start a conversion by setting GO/DONE.
 * @return 0 on success, 0xFFFF if a conversion was already running.
 */
uint16_t EPIC_ADC_Start(void)
{
    uint8_t v = EPIC_REG8(0x1FU);
    if (v & PIC_ADCON0_GO_DONE) return 0xFFFFU;
    EPIC_REG8(0x1FU) = v | PIC_ADCON0_GO_DONE;
    return 0U;
}

/**
 * @brief Report whether a conversion is in progress.
 * @return 1 if GO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void)
{
    return (EPIC_REG8(0x1FU) & PIC_ADCON0_GO_DONE) ? 1U : 0U;
}

/**
 * @brief Report whether the latest conversion completed.
 * @return 1 if ADIF (PIR1<6>) is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionDone(void)
{
    /* ADIF lives in PIR1<6>. */
    return (EPIC_REG8(0x0CU) & 0x40U) ? 1U : 0U;
}

/**
 * @brief Clear the ADIF flag via the IRQ table.
 */
void EPIC_ADC_ClearITFlag(void)
{
    EPIC_IRQ_ClearFlag(PIC16_IRQ_ADC);
}

/**
 * @brief Read the latest 10-bit result (0..1023), right-justified.
 * @return the conversion result.
 */
uint16_t EPIC_ADC_Read(void)
{
    /* Read ADRESL first, then ADRESH, in the active bank. */
    uint8_t lo = 0U, adfm_raw = 0U;
#ifdef EPIC_BANK1_READ8
    /* See target/pic16f87xa_platform.h: same corruption shape, read side. */
    EPIC_BANK1_READ8(ADRESL, lo);
    EPIC_BANK1_READ8(ADCON1, adfm_raw);
#else
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    lo = EPIC_REG8(0x9EU);        /* ADRESL, Bank 1. */
    adfm_raw = EPIC_REG8(0x9FU);  /* ADCON1. */
    pic_select_bank(prev);
#endif
    uint8_t adfm = (uint8_t)(adfm_raw & 0x80U);  /* ADFM. */
    uint8_t hi = EPIC_REG8(0x1EU);  /* ADRESH, Bank 0. */
    uint16_t raw = (uint16_t)(((uint16_t)hi << 8) | lo);
    /* Right-shift if left-justified (ADFM=0) so the caller always
     * gets a 0..1023 result. */
    if (!adfm) raw = (uint16_t)(raw >> 6);
    return raw & 0x03FFU;
}

/* ISRs. */

/**
 * @brief Weak ADC conversion-complete ISR; clears ADIF and invokes the
 *        registered callback.
 */
void ADC_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). ADIF is PIR1 bit 6. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_ADIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_ADIF);
#ifndef EPIC_AT
    if (g_adc && g_adc->ConvCpltCallback) {
        g_adc->ConvCpltCallback(EPIC_ADC_Read());
    }
#else
    (void)g_adc;
#endif
}
