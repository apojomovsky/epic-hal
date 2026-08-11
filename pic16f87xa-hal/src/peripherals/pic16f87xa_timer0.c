/* Timer0 driver implementation (DS39582B §5.0). */

#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16_irq.h"

/* Prescaler ratios, DS39582B Table 5-1: 000=1:2 ... 111=1:256. */
static const uint16_t ps_ratio[8] = { 2, 4, 8, 16, 32, 64, 128, 256 };

/* Owned copy of the caller's handle for the weak ISR (the caller's is
 * typically stack-local, out of scope by the time the ISR reads it).
 * Pinned to bank 3 (0x190) because the ISR deref bakes a constant
 * IRP=1 select (banks 2/3 only) under XC8 v4.00, so the storage must
 * live in that bank. Bank 3 keeps bank 2's 112 bytes contiguous for
 * the big module statics (the demo bundle's 64-byte taskmgr TCB would
 * not fit with the pin in bank 2, error 1250). */
static TIMER0_HandleTypeDef g_t0_storage EPIC_PLACE(0x190);
static const TIMER0_HandleTypeDef *g_t0_handle = NULL;

/** Read-modify-write helper for OPTION_REG. */
static void option_clr_set(uint8_t clr_mask, uint8_t set_mask)
{
#ifdef EPIC_BANK1_READ8
    /* Plain EPIC_REG8 RMW on Bank-1 OPTION_REG (0x81) misdirects to the
     * Bank-0 alias (0x01, TMR0) under XC8 v4.00 (see
     * target/pic16f87xa_platform.h). */
    uint8_t opt = 0u;
    EPIC_BANK1_READ8(OPTION_REG, opt);
    opt = (uint8_t)((opt & (uint8_t)~clr_mask) | set_mask);
    EPIC_BANK1_WRITE8(OPTION_REG, opt);
#else
    uint8_t opt = EPIC_REG8(PIC_REG_OPTION);
    opt = (uint8_t)((opt & (uint8_t)~clr_mask) | set_mask);
    EPIC_REG8(PIC_REG_OPTION) = opt;
#endif
}

EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    option_clr_set(PIC_OPTION_T0CS, 0u);

    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR0);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16_IRQ_TMR0);
    } else {
        EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR0);
    }

    g_t0_storage = *h;
    g_t0_handle = &g_t0_storage;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR0);
    option_clr_set(PIC_OPTION_T0CS, 0u);
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* DS39582B §5.3: writing TMR0 when the prescaler is assigned to
     * Timer0 clears the prescaler. Reload before re-enabling so the
     * first overflow happens after a clean prescaler cycle. */
    EPIC_REG8(PIC_REG_TMR0) = h->ReloadValue;

    /* Program the prescaler assignment + ratio + clock source + edge
     * in one atomic read-modify-write. */
    uint8_t set_mask = (uint8_t)((h->Prescaler & PIC_OPTION_PS_MASK));
    if (!h->PrescalerAssigned) set_mask |= PIC_OPTION_PSA;
    if (h->ClockSource == TIMER0_CLOCK_EXTERNAL) set_mask |= PIC_OPTION_T0CS;
    if (h->ClockEdge   == TIMER0_EDGE_FALLING)  set_mask |= PIC_OPTION_T0SE;

    /* Mask leaves RBPU and INTEDG untouched (DS39582B §4.2 / §14.12.4). */
    uint8_t clr_mask = (uint8_t)(PIC_OPTION_PS_MASK | PIC_OPTION_PSA |
                                 PIC_OPTION_T0CS  | PIC_OPTION_T0SE);
    option_clr_set(clr_mask, set_mask);

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    return EPIC_OK;
}

uint8_t EPIC_TIMER0_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR0);
}

void EPIC_TIMER0_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR0) = value;
}

uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    if ((unsigned)p > 7U) return 1U;
    return ps_ratio[p];
}

void TIMER0_IRQHandler(void)
{
    /* Direct flag ops, not the table-driven EPIC_IRQ_* path: the retlw
     * table clobbers PCLATH in ISR context when on another page
     * (class-F hazard; see the CCP handlers). TMR0IF is INTCON bit 2. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_TMR0IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_TMR0IF);
    if (g_t0_handle && g_t0_handle->OverflowCallback) {
        g_t0_handle->OverflowCallback();
    }
}
