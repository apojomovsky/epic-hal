/**
 * @file    pic16f87xa_adc.c
 * @brief   A/D converter driver, implementation (DS39582B §11.0).
 */

#include "peripherals/pic16f87xa_adc.h"
#include "core/pic16_irq.h"

static const ADC_HandleTypeDef *g_adc = NULL;

HAL_StatusTypeDef HAL_ADC_Init(const ADC_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
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
    PIC8_REG8(0x1FU) = adcon0;

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
#ifdef PIC8_BANK1_WRITE8
    /* See target/pic16f87xa_platform.h: a plain bank-switch RMW here
     * silently corrupts under XC8 v4.00. */
    PIC8_BANK1_WRITE8(ADCON1, adcon1);
#else
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(0x9FU) = adcon1;
        pic_select_bank(prev);
    }
#endif

    /* Interrupt enable. */
    HAL_IRQ_ClearFlag(PIC16_IRQ_ADC);
    if (h->ConvCpltCallback) HAL_IRQ_Enable(PIC16_IRQ_ADC);
    else                     HAL_IRQ_DisableSrc(PIC16_IRQ_ADC);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16_IRQ_ADC);
    HAL_IRQ_ClearFlag(PIC16_IRQ_ADC);
    PIC8_REG8(0x1FU) = 0x00U;
    {
        uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC8_REG8(0x9FU) = 0x00U;
        pic_select_bank(prev);
    }
    g_adc = NULL;
    return HAL_OK;
}

void HAL_ADC_SelectChannel(ADC_ChannelTypeDef ch)
{
    uint8_t v = PIC8_REG8(0x1FU);
    v = (uint8_t)((v & (uint8_t)~PIC_ADCON0_CHS_MASK) |
                  ((uint8_t)(ch & 0x7U) << PIC_ADCON0_CHS_POS));
    PIC8_REG8(0x1FU) = v;
}

uint16_t HAL_ADC_Start(void)
{
    uint8_t v = PIC8_REG8(0x1FU);
    if (v & PIC_ADCON0_GO_DONE) return 0xFFFFU;
    PIC8_REG8(0x1FU) = v | PIC_ADCON0_GO_DONE;
    return 0U;
}

uint8_t HAL_ADC_IsConversionInProgress(void)
{
    return (PIC8_REG8(0x1FU) & PIC_ADCON0_GO_DONE) ? 1U : 0U;
}

uint8_t HAL_ADC_IsConversionDone(void)
{
    /* ADIF lives in PIR1<6>. */
    return (PIC8_REG8(0x0CU) & 0x40U) ? 1U : 0U;
}

void HAL_ADC_ClearITFlag(void)
{
    HAL_IRQ_ClearFlag(PIC16_IRQ_ADC);
}

uint16_t HAL_ADC_Read(void)
{
    /* Read ADRESL first, then ADRESH, in the active bank. */
    uint8_t lo = 0U, adfm_raw = 0U;
#ifdef PIC8_BANK1_READ8
    /* See target/pic16f87xa_platform.h: same corruption shape, read side. */
    PIC8_BANK1_READ8(ADRESL, lo);
    PIC8_BANK1_READ8(ADCON1, adfm_raw);
#else
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    lo = PIC8_REG8(0x9EU);        /* ADRESL, Bank 1. */
    adfm_raw = PIC8_REG8(0x9FU);  /* ADCON1. */
    pic_select_bank(prev);
#endif
    uint8_t adfm = (uint8_t)(adfm_raw & 0x80U);  /* ADFM. */
    uint8_t hi = PIC8_REG8(0x1EU);  /* ADRESH, Bank 0. */
    uint16_t raw = (uint16_t)(((uint16_t)hi << 8) | lo);
    /* Right-shift if left-justified (ADFM=0) so the caller always
     * gets a 0..1023 result. */
    if (!adfm) raw = (uint16_t)(raw >> 6);
    return raw & 0x03FFU;
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void ADC_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16_IRQ_ADC)) return;
    HAL_IRQ_ClearFlag(PIC16_IRQ_ADC);
    if (g_adc && g_adc->ConvCpltCallback) {
        g_adc->ConvCpltCallback(HAL_ADC_Read());
    }
}
