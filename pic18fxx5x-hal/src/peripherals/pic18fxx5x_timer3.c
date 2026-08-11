/*
 * Timer3 driver, implementation (DS39632E §14.0, Register 14-1).
 */

#include "peripherals/pic18fxx5x_timer3.h"
#include "core/pic18_irq.h"

/* T3CON prescaler ratios, DS39632E Register 14-1:
 *   00 -> 1:1, 01 -> 1:2, 10 -> 1:4, 11 -> 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

/** Per-handle storage. COPIES the caller's handle (dangling-pointer
 *  rationale, see Timer1). The weak ISR reads from this owned copy. */
static TIMER3_HandleTypeDef g_t3_storage;
static const TIMER3_HandleTypeDef *g_t3_handle = NULL;

uint16_t EPIC_TIMER3_ReadCounter(void)
{
    /* With RD16 set, reading TMR3L latches TMR3H (DS39632E §14.0). */
    uint8_t lo = epic_sfr_read8(PIC_REG_TMR3L);
    uint8_t hi = epic_sfr_read8(PIC_REG_TMR3H);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

void EPIC_TIMER3_WriteCounter(uint16_t value)
{
    epic_sfr_write8(PIC_REG_TMR3H, (uint8_t)(value >> 8));
    epic_sfr_write8(PIC_REG_TMR3L, (uint8_t)(value & 0xFFU));
}

uint16_t EPIC_TIMER3_PrescalerToRatio(TIMER3_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

EPIC_StatusTypeDef EPIC_TIMER3_Init(const TIMER3_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T3CON), PIC_T3CON_TMR3ON);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR3);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC18_IRQ_TMR3);
    } else {
        EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR3);
    }

    /* Enable 16-bit read/write mode (RD16). Leave T3CCP2:T3CCP1 at reset
     * (00 = Timer1 is the CCP time base); the CCP/ECCP driver manages them. */
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_T3CON), PIC_T3CON_RD16);

    g_t3_storage = *h;
    g_t3_handle = &g_t3_storage;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER3_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC18_IRQ_TMR3);
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR3);
    EPIC_REG8(PIC_REG_T3CON) = PIC_T3CON_POR_VALUE;
    EPIC_REG8(PIC_REG_TMR3H) = 0x00U;
    EPIC_REG8(PIC_REG_TMR3L) = 0x00U;
    g_t3_handle = NULL;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER3_Start(const TIMER3_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    EPIC_TIMER3_WriteCounter(h->ReloadValue);

    /* Program T3CON in one write. RD16 stays set (from Init). T3CCP2:T3CCP1
     * (bits 6,3) are left at 0 (Timer1 as CCP time base).
     *   T3CKPS1:T3CKPS0 -> bits 5:4
     *   T3SYNC           -> bit 2
     *   TMR3CS           -> bit 1
     *   TMR3ON           -> bit 0 (set last) */
    uint8_t v = PIC_T3CON_RD16;     /* keep 16-bit mode */
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);
    if (h->ClockSync   == TIMER3_ASYNC_EXTERNAL) v |= PIC_T3CON_T3SYNC;
    if (h->ClockSource == TIMER3_CLOCK_EXTERNAL) v |= PIC_T3CON_TMR3CS;
    v |= PIC_T3CON_TMR3ON;
    EPIC_REG8(PIC_REG_T3CON) = v;

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER3_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T3CON), PIC_T3CON_TMR3ON);
    return EPIC_OK;
}

void TIMER3_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC18_IRQ_TMR3)) return;
    EPIC_IRQ_ClearFlag(PIC18_IRQ_TMR3);
    if (g_t3_handle && g_t3_handle->OverflowCallback) {
        g_t3_handle->OverflowCallback();
    }
}
