/**
 * @file    pic18fxx5x_timer1.c
 * @brief   Timer1 driver, implementation (DS39632E §12.0, Register 12-1).
 */

#include "peripherals/pic18fxx5x_timer1.h"
#include "core/pic18_irq.h"

/* T1CON prescaler ratios, DS39632E Register 12-1:
 *   00 -> 1:1, 01 -> 1:2, 10 -> 1:4, 11 -> 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

/** Per-handle storage. COPIES the caller's handle (the caller's
 *  `TIMER1_HandleTypeDef` is typically a stack-local that is out of scope by
 *  the time the ISR reads it back, so storing a pointer to it would dangle).
 *  The weak ISR reads from this owned copy. */
static TIMER1_HandleTypeDef g_t1_storage;
static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* With RD16 set, reading TMR1L latches TMR1H into a shadow (DS39632E
     * §12.0); read low then high for a consistent 16-bit value. */
    uint8_t lo = epic_sfr_read8(PIC_REG_TMR1L);
    uint8_t hi = epic_sfr_read8(PIC_REG_TMR1H);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* With RD16 set, writing TMR1L latches TMR1H (DS39632E §12.0); write
     * high byte first via the shadow, then low to commit both. */
    epic_sfr_write8(PIC_REG_TMR1H, (uint8_t)(value >> 8));
    epic_sfr_write8(PIC_REG_TMR1L, (uint8_t)(value & 0xFFU));
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
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC18_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR1);
    }

    /* Enable 16-bit read/write mode (RD16) so the atomic 16-bit idiom works
     * the same way it always does on PIC16. */
    PIC8_BIT_SET(PIC8_REG8(PIC_REG_T1CON), PIC_T1CON_RD16);

    g_t1_storage = *h;
    g_t1_handle = &g_t1_storage;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
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

    /* Program T1CON in one write. RD16 stays set (from Init), T1RUN is
     * read-only and left clear.
     *   T1CKPS1:T1CKPS0 -> bits 5:4
     *   T1OSCEN          -> bit 3
     *   T1SYNC           -> bit 2
     *   TMR1CS           -> bit 1
     *   TMR1ON           -> bit 0 (set last) */
    uint8_t v = PIC_T1CON_RD16;     /* keep 16-bit mode */
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);
    if (h->Oscillator  == TIMER1_OSCILLATOR_ON)   v |= PIC_T1CON_T1OSCEN;
    if (h->ClockSync   == TIMER1_ASYNC_EXTERNAL) v |= PIC_T1CON_T1SYNC;
    if (h->ClockSource == TIMER1_CLOCK_EXTERNAL)  v |= PIC_T1CON_TMR1CS;
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
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_TMR1)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR1);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
