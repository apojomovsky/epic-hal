/**
 * Timer1 driver, implementation (DS41364B §16.0). Every T1CON bit mask
 * and the POR value are transcribed from DS41364B Register 16-1.
 */

#include "peripherals/pic16f193x_timer1.h"
#include "core/pic16f193x_irq.h"

/* T1CON prescaler ratios, DS41364B Register 16-1: T1CKPS<1:0> ->
 * ratio mapping, transcribed from the datasheet's table. */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

/**
 * @brief Atomically read the 16-bit counter value.
 *
 * DS41364B §16.4.1 explicitly warns about TMR1H:TMR1L consistency
 * issues; this wraps that risk by re-reading the high byte until two
 * consecutive reads agree.
 *
 * @return the current 16-bit counter value
 */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* Read high byte, then low byte, then high byte again; if the
     * second read differs, the low byte rolled over, so use the
     * refreshed high. Standard PIC16 idiom (DS41364B §16.4.1). */
    uint8_t hi1, lo, hi2;
    do {
        hi1 = EPIC_REG8(PIC_REG_TMR1H);
        lo  = EPIC_REG8(PIC_REG_TMR1L);
        hi2 = EPIC_REG8(PIC_REG_TMR1H);
    } while (hi1 != hi2);

    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

/**
 * @brief Atomically write the 16-bit counter value (high byte first).
 * @param value counter value to write, 0..0xFFFF
 */
void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* Per DS41364B §16.8: writing TMR1H clears the prescaler. Write
     * high byte first. */
    EPIC_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Convert a prescaler enum to its integer ratio (1, 2, 4, 8).
 * @param p prescaler selection
 * @return the divider ratio, or 1 for an out-of-range value
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief Configure Timer1 from the handle and store it for the ISR.
 * @param h handle with clock source, prescaler and callback
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the clock
 *         source is not internal
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    /* External clock / T1OSC / CAPOSC sources are out of scope for
     * this phase (MANUAL.md §11 "Not in this phase"). */
    if (h->ClockSource != TIMER1_CLOCK_INTERNAL) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    /* Configure the overflow interrupt. */
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16F193X_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR1);
    }

    g_t1_handle = h;
    return EPIC_OK;
}

/**
 * @brief Stop Timer1, disable its interrupt and restore T1CON.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR1);
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_TMR1H) = 0x00U;
    EPIC_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_handle = NULL;
    return EPIC_OK;
}

/**
 * @brief Enable TMR1 counting: reload the counter and program T1CON.
 * @param h handle with prescaler and reload value
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the clock
 *         source is not internal
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    /* External clock / T1OSC / CAPOSC sources are out of scope for
     * this phase (MANUAL.md §11 "Not in this phase"). */
    if (h->ClockSource != TIMER1_CLOCK_INTERNAL) return EPIC_INVALID;

    EPIC_TIMER1_WriteCounter(h->ReloadValue);

    /* Program T1CON in one write: fields and bit positions per
     * DS41364B Register 16-1. */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Prescaler & 0x3U) << 4);   /* T1CKPS<1:0>. */
    /* TMR1CS<1:0> = 00 (FOSC/4): both bits left at 0. T1OSCEN,
     * T1SYNC: leave at 0 until the T1GCON/T1OSC work in the next
     * spec adds them. */
    v |= PIC_T1CON_TMR1ON;                        /* set last. */
    EPIC_REG8(PIC_REG_T1CON) = v;

    return EPIC_OK;
}

/**
 * @brief Disable TMR1 counting (clears TMR1ON).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

/**
 * @brief Timer1 overflow ISR: clears TMR1IF and invokes the callback.
 */
void TIMER1_IRQHandler(void)
{
    /* Direct flag ops (class-F). TMR1IF is PIR1 bit 0. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
