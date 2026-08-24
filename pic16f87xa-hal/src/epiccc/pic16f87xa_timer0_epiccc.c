/* Timer0 driver implementation (DS39582B §5.0); epic-cc variant. */

#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16_irq.h"

/* Owned copy of the caller's callback for the weak ISR (the caller's
 * handle is typically stack-local, out of scope by the time the ISR
 * reads it). The ISR only needs the callback, so store the pointer
 * (1 byte) rather than a full handle copy (the same RAM saving the
 * 88X driver makes). */
static void (*g_t0_overflow_cb)(void) = NULL;

/**
 * @brief Read-modify-write helper for OPTION_REG: clear `clr_mask`
 *        bits, set `set_mask` bits, atomically.
 * @param clr_mask bits to clear.
 * @param set_mask bits to set.
 */
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

/**
 * @brief Configure Timer0: stop it, arm the overflow interrupt if a
 *        callback is given, and record the callback.
 * @param h handle with ClockSource, ClockEdge, Prescaler,
 *        PrescalerAssigned, ReloadValue, OverflowCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
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

    g_t0_overflow_cb = h->OverflowCallback;
    return EPIC_OK;
}

/**
 * @brief De-initialize Timer0: disable the interrupt, stop counting
 *        and reset TMR0.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR0);
    option_clr_set(PIC_OPTION_T0CS, 0u);
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Start Timer0 counting: reload TMR0 and program the prescaler
 *        assignment/ratio, clock source and edge.
 * @param h handle whose ReloadValue and config are applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
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

/**
 * @brief Stop Timer0 counting by clearing T0CS.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    return EPIC_OK;
}

/**
 * @brief Read the current TMR0 value.
 * @return the 8-bit counter value.
 */
uint8_t EPIC_TIMER0_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR0);
}

/**
 * @brief Write the TMR0 counter (also clears the prescaler).
 * @param value the 8-bit value to load.
 */
void EPIC_TIMER0_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR0) = value;
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p the prescaler enum value.
 * @return the ratio (2..256), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    /* Avoid const array `ps_ratio` which lowers to a flash GEP that isel
     * does not yet handle for this family. Straight-line mapping is
     * equivalent and lowers to plain branches. */
    switch (p) {
        case 0: return 2U;
        case 1: return 4U;
        case 2: return 8U;
        case 3: return 16U;
        case 4: return 32U;
        case 5: return 64U;
        case 6: return 128U;
        case 7: return 256U;
        default: return 1U;
    }
}

/**
 * @brief Weak Timer0 ISR: clears TMR0IF and fires the overflow
 *        callback. Under epic-cc the callback is via a function pointer
 *        that isel does not yet lower, so the ISR just clears the flag;
 *        the smoke's main loop polls the flag. Filed as a gap.
 */
void TIMER0_IRQHandler(void)
{
    /* Direct flag ops, not the table-driven EPIC_IRQ_* path: the retlw
     * table clobbers PCLATH in ISR context when on another page
     * (class-F hazard; see the CCP handlers). TMR0IF is INTCON bit 2. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_TMR0IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_TMR0IF);
#ifndef EPIC_AT
    if (g_t0_overflow_cb) g_t0_overflow_cb();
#else
    (void)g_t0_overflow_cb;
#endif
}
