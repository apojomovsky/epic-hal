/**
 * @file    pic16f193x_timer1.c
 * @brief   Timer1 driver, implementation (DS41364B §16.0).
 *
 * @details
 *   Mirrors pic16f87xa_timer1.c's shape. Every T1CON bit mask and
 *   the POR value are transcribed from DS41364B Register 16-1;
 *   verify against the datasheet before relying on any literal here.
 */

#include "peripherals/pic16f193x_timer1.h"
#include "core/pic16f193x_irq.h"

/* T1CON prescaler ratios, DS41364B Register 16-1. Verify the
 * T1CKPS<1:0> -> ratio mapping against the datasheet's table
 * before relying on these values; the §4 gate will catch any
 * wrong literal. */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

/** Atomic 16-bit read. DS41364B §16.4.1 explicitly warns about
 *  TMR1H:TMR1L consistency issues; wrap that risk here. */
uint16_t HAL_TIMER1_ReadCounter(void)
{
    /* Read high byte, then low byte, then high byte again; if the
     * second read differs, the low byte rolled over, so use the
     * refreshed high. Standard PIC16 idiom (DS41364B §16.4.1). */
    uint8_t hi1, lo, hi2;
    do {
        hi1 = PIC8_REG8(PIC_REG_TMR1H);
        lo  = PIC8_REG8(PIC_REG_TMR1L);
        hi2 = PIC8_REG8(PIC_REG_TMR1H);
    } while (hi1 != hi2);

    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

void HAL_TIMER1_WriteCounter(uint16_t value)
{
    /* Per DS41364B §16.8: writing TMR1H clears the prescaler. Write
     * high byte first. */
    PIC8_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    PIC8_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

uint16_t HAL_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

HAL_StatusTypeDef HAL_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    /* External clock / T1OSC / CAPOSC sources are out of scope for
     * this phase (MANUAL.md §11 "Not in this phase"). */
    if (h->ClockSource != TIMER1_CLOCK_INTERNAL) return HAL_INVALID;

    /* Stop the timer before reconfiguring. */
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    /* Configure the overflow interrupt. */
    HAL_IRQ_ClearFlag(PIC16F193X_IRQ_TMR1);
    if (h->OverflowCallback) {
        HAL_IRQ_Enable(PIC16F193X_IRQ_TMR1);
    } else {
        HAL_IRQ_DisableSrc(PIC16F193X_IRQ_TMR1);
    }

    g_t1_handle = h;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMER1_DeInit(void)
{
    HAL_IRQ_DisableSrc(PIC16F193X_IRQ_TMR1);
    HAL_IRQ_ClearFlag(PIC16F193X_IRQ_TMR1);
    PIC8_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    PIC8_REG8(PIC_REG_TMR1H) = 0x00U;
    PIC8_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_handle = NULL;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if (!h) return HAL_INVALID;
    /* External clock / T1OSC / CAPOSC sources are out of scope for
     * this phase (MANUAL.md §11 "Not in this phase"). */
    if (h->ClockSource != TIMER1_CLOCK_INTERNAL) return HAL_INVALID;

    HAL_TIMER1_WriteCounter(h->ReloadValue);

    /* Program T1CON in one write. The order of fields and the bit
     * positions MUST be transcribed from DS41364B Register 16-1;
     * do not ship until the §4 gate confirms the literal values
     * are correct. */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);   /* T1CKPS<1:0>. */
    /* TMR1CS<1:0> = 00 (FOSC/4): both bits left at 0. T1OSCEN,
     * T1SYNC: leave at 0 until the T1GCON/T1OSC work in the next
     * spec adds them. */
    v |= PIC_T1CON_TMR1ON;                        /* set last. */
    PIC8_REG8(PIC_REG_T1CON) = v;

    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMER1_Stop(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return HAL_OK;
}

void TIMER1_IRQHandler(void)
{
    if (!HAL_IRQ_GetFlag(PIC16F193X_IRQ_TMR1)) return;
    HAL_IRQ_ClearFlag(PIC16F193X_IRQ_TMR1);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
