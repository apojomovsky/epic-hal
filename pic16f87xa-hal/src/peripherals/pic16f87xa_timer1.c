/* Timer1 driver implementation (DS39582B §6.0). */

#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16_irq.h"

/* T1CON prescaler ratios, DS39582B Register 6-1:
 *   00 → 1:1, 01 → 1:2, 10 → 1:4, 11 → 1:8 */
static const uint16_t ps_ratio[4] = { 1, 2, 4, 8 };

static const TIMER1_HandleTypeDef *g_t1_handle = NULL;

/**
 * @brief Atomically read the 16-bit counter. The datasheet warns that
 *        reading TMR1H:TMR1L in asynchronous counter mode can return
 *        inconsistent values (DS39582B §6.4.1). Wrap that risk here so
 *        callers don't have to.
 * @return the current 16-bit TMR1H:L value.
 */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    /* Read high byte, then low byte, then high byte again; if the
     * second read differs, the low byte rolled over, so use the
     * refreshed high. Standard PIC16 idiom (DS39582B §6.4.1). */
    uint8_t hi1, lo, hi2;
    do {
        hi1 = EPIC_REG8(PIC_REG_TMR1H);
        lo  = EPIC_REG8(PIC_REG_TMR1L);
        hi2 = EPIC_REG8(PIC_REG_TMR1H);
    } while (hi1 != hi2);

    return (uint16_t)(((uint16_t)hi2 << 8) | lo);
}

/**
 * @brief Atomically write the 16-bit counter (high byte first; writing
 *        either byte clears the prescaler).
 * @param value the 16-bit value to load.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    /* Per DS39582B §6.8: writing TMR1H or TMR1L clears the prescaler.
     * Write high byte first. */
    EPIC_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p the prescaler enum value.
 * @return the ratio (1, 2, 4 or 8), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief Configure Timer1: stop it, arm the overflow interrupt if a
 *        callback is given, and record the handle.
 * @param h handle with ClockSource, ClockSync, Oscillator, Prescaler,
 *        ReloadValue, OverflowCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);

    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16_IRQ_TMR1);
    } else {
        EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    }

    g_t1_handle = h;
    return EPIC_OK;
}

/**
 * @brief De-initialize Timer1: disable the interrupt and restore T1CON
 *        and the counter to reset values.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR1);
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_POR_VALUE;
    EPIC_REG8(PIC_REG_TMR1H) = 0x00U;
    EPIC_REG8(PIC_REG_TMR1L) = 0x00U;
    g_t1_handle = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer1 counting: reload the counter and program T1CON
 *        (prescaler, oscillator, sync, clock source), setting TMR1ON.
 * @param h handle whose ReloadValue and config are applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
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
    EPIC_REG8(PIC_REG_T1CON) = v;

    return EPIC_OK;
}

/**
 * @brief Stop Timer1 counting by clearing TMR1ON.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

/**
 * @brief Weak Timer1 ISR: clears TMR1IF and fires the overflow
 *        callback.
 */
void TIMER1_IRQHandler(void)
{
    /* Direct flag ops (class-F: the table route clobbers PCLATH in ISR
     * context; see the CCP handlers). TMR1IF is PIR1 bit 0. */
    if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
    if (g_t1_handle && g_t1_handle->OverflowCallback) {
        g_t1_handle->OverflowCallback();
    }
}
