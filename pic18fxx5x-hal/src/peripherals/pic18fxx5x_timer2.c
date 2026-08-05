/**
 * @file    pic18fxx5x_timer2.c
 * @brief   Timer2 driver, implementation (DS39632E §12.0, Register 12-2).
 */

#include "peripherals/pic18fxx5x_timer2.h"
#include "core/pic18_irq.h"

/* T2CON prescaler, DS39632E Register 12-2:
 *   00 -> 1:1, 01 -> 1:4, 1x -> 1:16 */
static const uint8_t pre_ratio[4] = { 1, 4, 16, 16 };

/* T2CON postscaler, DS39632E Register 12-2: 1:(N+1). */
static const uint8_t post_ratio[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

/** Per-handle storage. COPIES the caller's handle (see Timer1 for the
 *  dangling-pointer rationale). The weak ISR reads from this owned copy. */
static TIMER2_HandleTypeDef g_t2_storage;
static const TIMER2_HandleTypeDef *g_t2_handle = NULL;

uint8_t EPIC_TIMER2_ReadCounter(void)
{
    return PIC8_REG8(PIC_REG_TMR2);
}

void EPIC_TIMER2_WriteCounter(uint8_t value)
{
    PIC8_REG8(PIC_REG_TMR2) = value;
}

uint8_t EPIC_TIMER2_ReadPeriod(void)
{
    /* PR2 is in the Access Bank (0xFCB), no bank switching needed. */
    return PIC8_REG8(PIC_REG_PR2);
}

void EPIC_TIMER2_WritePeriod(uint8_t period)
{
    PIC8_REG8(PIC_REG_PR2) = period;
}

uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return pre_ratio[p];
}

uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p)
{
    if ((unsigned)p > 15U) return 1U;
    return post_ratio[p];
}

EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR2);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC18_IRQ_TMR2);
    } else {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR2);
    }

    g_t2_storage = *h;
    g_t2_handle = &g_t2_storage;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR2);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR2);
    PIC8_REG8(PIC_REG_T2CON) = PIC_T2CON_POR_VALUE;
    EPIC_TIMER2_WritePeriod(PIC_PR2_POR_VALUE);
    g_t2_handle = NULL;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Period register first (DS39632E §12.0 recommends setting PR2 before
     * enabling TMR2ON to avoid spurious matches). */
    EPIC_TIMER2_WritePeriod(h->Period);

    /* Build T2CON:
     *   T2OUTPS3:T2OUTPS0 -> bits 6:3
     *   TMR2ON            -> bit 2
     *   T2CKPS1:T2CKPS0   -> bits 1:0
     */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Postscaler & 0xFU) << 3);
    v |= PIC_T2CON_TMR2ON;
    v |= (uint8_t)(h->Prescaler & 0x3U);
    PIC8_REG8(PIC_REG_T2CON) = v;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER2_Stop(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);
    return EPIC_OK;
}

void TIMER2_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_TMR2)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR2);
    if (g_t2_handle && g_t2_handle->OverflowCallback) {
        g_t2_handle->OverflowCallback();
    }
}
