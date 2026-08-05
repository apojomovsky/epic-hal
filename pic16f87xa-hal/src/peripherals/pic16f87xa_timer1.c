/**
 * @file    pic16f87xa_timer1.c
 * @brief   Timer1 driver, implementation (DS39582B §6.0).
 */

#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16_irq.h"

/* T1CON prescaler ratios, DS39582B Register 6-1:
 *   00 → 1:1, 01 → 1:2, 10 → 1:4, 11 → 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

/** Atomic 16-bit read. The datasheet warns that reading TMR1H:TMR1L
 *  in asynchronous counter mode can return inconsistent values
 *  (DS39582B §6.4.1). Wrap that risk here so callers don't have to. */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* Read high byte, then low byte, then high byte again; if the
     * second read differs, the low byte rolled over, so use the
     * refreshed high. Standard PIC16 idiom (DS39582B §6.4.1). */
    uint8_t hi1, lo, hi2;
    do {
        hi1 = PIC8_REG8(PIC_REG_TMR1H);
        lo  = PIC8_REG8(PIC_REG_TMR1L);
        hi2 = PIC8_REG8(PIC_REG_TMR1H);
    } while (hi1 != hi2);

    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* Per DS39582B §6.8: writing TMR1H or TMR1L clears the prescaler.
     * Write high byte first. */
    PIC8_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    PIC8_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    /* Configure the overflow interrupt. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    }

    g_t1_handle = h;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    PIC8_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    PIC8_REG8(PIC_REG_TMR1H) = 0x00U;
    PIC8_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_handle = NULL;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    EPIC_TIMER1_WriteCounter(h->ReloadValue);

    /* Program T1CON in one RMW.
     *   T1CKPS1:T1CKPS0 → bits 5:4
     *   T1OSCEN          → bit 3
     *   T1SYNC           → bit 2
     *   TMR1CS           → bit 1
     *   TMR1ON           → bit 0 (set last) */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);
    if (h->Oscillator  == TIMER1_OSCILLATOR_ON) v |= PIC_T1CON_T1OSCEN;
    if (h->ClockSync   == TIMER1_ASYNC_EXTERNAL) v |= PIC_T1CON_T1SYNC;
    if (h->ClockSource == TIMER1_CLOCK_EXTERNAL) v |= PIC_T1CON_TMR1CS;
    v |= PIC_T1CON_TMR1ON;
    PIC8_REG8(PIC_REG_T1CON) = v;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    PIC8_BIT_CLR(PIC8_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

void TIMER1_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_TMR1)) return;
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
