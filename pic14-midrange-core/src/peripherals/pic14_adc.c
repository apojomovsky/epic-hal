/* Shared A/D converter driver, classic mid-range: 87XA (DS39582B
 * section 11.0) and 88X (DS40001291H section 15.0). The families wire
 * the ADC differently: reference configuration (PCFG table vs VCFG
 * bits), channel mux width, clock sources and ANSEL gating all differ,
 * and the split is gated by PIC14MIDRANGE_HAS_ADC_PCFG and
 * PIC14MIDRANGE_HAS_ANSEL (see pic14_adc.h for the enum flavors).
 * GO/DONE sits at a different bit per family but both spell it
 * PIC_ADCON0_GO_DONE, so the conversion paths below are token-shared. */

#include "peripherals/pic14_adc.h"
#include "core/pic16_irq.h"

/* callback storage. */

/* The ISR only needs the conversion-complete callback, so store the
 * pointer rather than a full handle copy: the caller's handle is
 * typically stack-local, out of scope by the time the ISR reads it
 * (epic-common/MANUAL.md §3.3). */
static void (*g_adc_conv_cb)(uint16_t result) = NULL;

#if PIC14MIDRANGE_HAS_ANSEL
/**
 * @brief Map one analog channel to its ANSEL/ANSELH gate.
 * @param ch the analog channel.
 * @param reg_out set to PIC_REG_ANSEL or PIC_REG_ANSELH.
 * @return the bit position, or 0xFF if the channel has no pin.
 */
static uint8_t channel_ansel_bit(ADC_ChannelTypeDef ch, uint16_t *reg_out)
{
    switch (ch) {
        case ADC_CHANNEL_AN0:  *reg_out = PIC_REG_ANSEL;  return 0U;
        case ADC_CHANNEL_AN1:  *reg_out = PIC_REG_ANSEL;  return 1U;
        case ADC_CHANNEL_AN2:  *reg_out = PIC_REG_ANSEL;  return 2U;
        case ADC_CHANNEL_AN3:  *reg_out = PIC_REG_ANSEL;  return 3U;
        case ADC_CHANNEL_AN4:  *reg_out = PIC_REG_ANSEL;  return 4U;
        case ADC_CHANNEL_AN5:  *reg_out = PIC_REG_ANSEL;  return 5U;  /* 40/44-pin only. */
        case ADC_CHANNEL_AN6:  *reg_out = PIC_REG_ANSEL;  return 6U;  /* 40/44-pin only. */
        case ADC_CHANNEL_AN7:  *reg_out = PIC_REG_ANSEL;  return 7U;  /* 40/44-pin only. */
        case ADC_CHANNEL_AN8:  *reg_out = PIC_REG_ANSELH; return 0U;  /* RB2. */
        case ADC_CHANNEL_AN9:  *reg_out = PIC_REG_ANSELH; return 1U;  /* RB3. */
        case ADC_CHANNEL_AN10: *reg_out = PIC_REG_ANSELH; return 2U;  /* RB1. */
        case ADC_CHANNEL_AN11: *reg_out = PIC_REG_ANSELH; return 3U;  /* RB4. */
        case ADC_CHANNEL_AN12: *reg_out = PIC_REG_ANSELH; return 4U;  /* RB0. */
        case ADC_CHANNEL_AN13: *reg_out = PIC_REG_ANSELH; return 5U;  /* RB5. */
        default:               return 0xFFU;                          /* CVREF, VP6. */
    }
}
#endif /* PIC14MIDRANGE_HAS_ANSEL */

/**
 * @brief Initialize the A/D converter: program ADCON0/ADCON1 from the
 *        handle and arm the conversion-complete interrupt if a callback
 *        is set.
 * @param h handle with Channel, ClockSource, Reference, ResultFormat,
 *        ConvCpltCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    g_adc_conv_cb = h->ConvCpltCallback;

    /* ADCON0, Bank 0, address 0x1F: ADON, GO/DONE (clear), the channel
     * mux and ADCS1:ADCS0. The mux width and position differ per
     * family; both spell them PIC_ADCON0_CHS_MASK / _POS. */
    uint8_t adcon0 = PIC_ADCON0_ADON;
#if PIC14MIDRANGE_HAS_ADC_PCFG
    adcon0 |= (uint8_t)(((uint8_t)h->Channel & 0x7U) << PIC_ADCON0_CHS_POS);
#else
    adcon0 |= (uint8_t)(((uint8_t)h->Channel & 0xFU) << PIC_ADCON0_CHS_POS);
#endif
    adcon0 |= (uint8_t)(((uint8_t)h->ClockSource & 0x3U) << PIC_ADCON0_ADCS_POS);
    EPIC_REG8(PIC_REG_ADCON0) = adcon0;

    /* ADCON1, Bank 1, address 0x9F. The 87XA spends its bits on the
     * PCFG table plus ADCS2; the 88X on ADFM plus the VCFG pair. */
    uint8_t adcon1 = 0x00U;
    if (h->ResultFormat == ADC_FORMAT_RIGHT) adcon1 |= PIC_ADCON1_ADFM;
#if PIC14MIDRANGE_HAS_ADC_PCFG
    adcon1 |= (uint8_t)((uint8_t)h->Reference & PIC_ADCON1_PCFG_MASK);
    if (h->ClockSource >= ADC_CLOCK_FOSC_4) {
        /* ADCS2 = 1 for the four high clock modes. */
        adcon1 |= PIC_ADCON1_ADCS2;
    }
#else
    adcon1 |= (uint8_t)(((uint8_t)h->Reference & 0x3U) << 4);
#endif
#ifdef EPIC_BANK1_WRITE8
    /* See target/pic16f{87xa,88x}_platform.h: a plain bank-switch RMW
     * here silently corrupts under XC8 v4.00. */
    EPIC_BANK1_WRITE8(ADCON1, adcon1);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_ADCON1) = adcon1;
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
    EPIC_REG8(PIC_REG_ADCON0) = 0x00U;
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        EPIC_REG8(PIC_REG_ADCON1) = 0x00U;
        pic_select_bank(prev);
    }
    g_adc_conv_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Select the analog channel without starting a conversion.
 * @param ch the analog channel to select.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch)
{
    uint8_t v = EPIC_REG8(PIC_REG_ADCON0);
#if PIC14MIDRANGE_HAS_ADC_PCFG
    v = (uint8_t)((v & (uint8_t)~PIC_ADCON0_CHS_MASK) |
                  ((uint8_t)((uint8_t)ch & 0x7U) << PIC_ADCON0_CHS_POS));
#else
    v = (uint8_t)((v & (uint8_t)~PIC_ADCON0_CHS_MASK) |
                  ((uint8_t)((uint8_t)ch & 0xFU) << PIC_ADCON0_CHS_POS));
#endif
    EPIC_REG8(PIC_REG_ADCON0) = v;
}

#if PIC14MIDRANGE_HAS_ANSEL
/**
 * @brief Configure the ANSEL/ANSELH bit for one analog channel.
 * @param ch the channel whose pin to enable.
 */
void EPIC_ADC_ConfigChannel(ADC_ChannelTypeDef ch)
{
    uint16_t reg = 0U;
    uint8_t bit = channel_ansel_bit(ch, &reg);
    if (bit == 0xFFU) return;   /* CVREF / VP6: no pin. */

    uint8_t ansel  = 0u, anselh = 0u;
#ifdef EPIC_BANK3_READ8
    EPIC_BANK3_READ8(ANSEL, ansel);
    EPIC_BANK3_READ8(ANSELH, anselh);
#else
    ansel  = EPIC_REG8(PIC_REG_ANSEL);
    anselh = EPIC_REG8(PIC_REG_ANSELH);
#endif
    if (reg == PIC_REG_ANSEL) {
        ansel |= EPIC_BIT(bit);
#ifdef EPIC_BANK3_WRITE8
        EPIC_BANK3_WRITE8(ANSEL, ansel);
#else
        EPIC_REG8(PIC_REG_ANSEL) = ansel;
#endif
    } else {
        anselh |= EPIC_BIT(bit);
#ifdef EPIC_BANK3_WRITE8
        EPIC_BANK3_WRITE8(ANSELH, anselh);
#else
        EPIC_REG8(PIC_REG_ANSELH) = anselh;
#endif
    }
}
#endif /* PIC14MIDRANGE_HAS_ANSEL */

/**
 * @brief Start a conversion by setting GO/DONE.
 * @return 0 on success, 0xFFFF if a conversion was already running.
 */
uint16_t EPIC_ADC_Start(void)
{
    if (EPIC_ADC_IsConversionInProgress()) return 0xFFFFU;

    EPIC_REG8(PIC_REG_ADCON0) |= PIC_ADCON0_GO_DONE;
    return 0x0000U;
}

/**
 * @brief Report whether a conversion is in progress.
 * @return 1 if GO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void)
{
    return (EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO_DONE) ? 1U : 0U;
}

/**
 * @brief Report whether the latest conversion completed.
 * @return 1 if ADIF (PIR1<6>) is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionDone(void)
{
    return (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_ADIF) ? 1U : 0U;
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
    /* ADRESL and ADCON1 sit in Bank 1 on both families; read them
     * through the Bank-1 tokens so the bank discipline matches the
     * write path. */
    uint8_t lo = 0U, adfm_raw = 0U;
#ifdef EPIC_BANK1_READ8
    EPIC_BANK1_READ8(ADRESL, lo);
    EPIC_BANK1_READ8(ADCON1, adfm_raw);
#else
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        lo = EPIC_REG8(PIC_REG_ADRESL);
        adfm_raw = EPIC_REG8(PIC_REG_ADCON1);
        pic_select_bank(prev);
    }
#endif
    uint8_t hi = EPIC_REG8(PIC_REG_ADRESH);
    uint16_t raw = (uint16_t)(((uint16_t)hi << 8) | lo);
    /* Right-shift if left-justified (ADFM=0) so the caller always
     * gets a 0..1023 result. */
    if (!(adfm_raw & PIC_ADCON1_ADFM)) raw = (uint16_t)(raw >> 6);
    return raw & 0x03FFU;
}

/**
 * @brief Weak ADC conversion-complete ISR; clears ADIF and invokes the
 *        registered callback.
 */
void ADC_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). ADIF is PIR1 bit 6. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_ADIF)) return;
    uint16_t result = EPIC_ADC_Read();
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_ADIF);
    if (g_adc_conv_cb) g_adc_conv_cb(result);
}
