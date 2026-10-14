/* PIC16F1508/1509 Timer1 driver, implementation (DS40001609E §19.0). */

#include <stddef.h>
#include "peripherals/pic16f1508_timer1.h"
#include "core/pic16f1508_irq.h"

/* T1CON prescaler, DS40001609E Register 19-1:
 * 00 -> 1:1, 01 -> 1:2, 10 -> 1:4, 11 -> 1:8. */
static const uint8_t pre_ratio[4] = { 1, 2, 4, 8 };

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
void (*g_t1_overflow_cb)(void) = NULL;

/**
 * @brief Check the clock source and prescaler fields of a handle.
 * @param h Handle to check.
 * @return 1 when the clock source is internal or external and the prescaler
 * is in range, 0 otherwise.
 */
static uint8_t fields_ok(const TIMER1_HandleTypeDef *h)
{
    uint8_t clock_ok = ((h->ClockSource == TIMER1_CLOCK_INTERNAL)
        || (h->ClockSource == TIMER1_CLOCK_EXTERNAL)) ? 1U : 0U;
    uint8_t pre_ok = ((unsigned int)h->Prescaler <= 3U) ? 1U : 0U;
    return (uint8_t)(clock_ok & pre_ok);
}

/**
 * @brief Program T1CON from a handle in one write.
 * @details Internal clock is TMR1CS 00 (FOSC/4). External clock is TMR1CS 10
 * with T1OSCEN clear (T1CKI pin, not the crystal) and T1SYNC set (the input
 * is not resynchronized), the counter-mode pairing of Register 19-1.
 * @param h Handle holding the clock source and prescaler.
 */
static void timer1_write_con(const TIMER1_HandleTypeDef *h)
{
    uint8_t con = 0U;

    if (h->ClockSource == TIMER1_CLOCK_EXTERNAL)
    {
        con = (uint8_t)(con | PIC_T1CON_T1SYNC
            | (uint8_t)((unsigned int)TIMER1_CLOCK_EXTERNAL << PIC_T1CON_TMR1CS_SHIFT));
    }
    con = (uint8_t)(con
        | (uint8_t)((unsigned int)h->Prescaler << PIC_T1CON_T1CKPS_SHIFT)
        | PIC_T1CON_TMR1ON);
    EPIC_REG8(PIC_REG_T1CON) = con;
}

/**
 * @brief Read the 16-bit counter value, low byte first.
 * @return TMR1H:TMR1L value, 0..65535.
 */
uint16_t EPIC_TIMER1_ReadCounter(void)
{
    uint8_t lo = EPIC_REG8(PIC_REG_TMR1L);
    uint8_t hi = EPIC_REG8(PIC_REG_TMR1H);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/**
 * @brief Write the 16-bit counter value, high byte first.
 * @param value Counter value, 0..65535.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value)
{
    EPIC_REG8(PIC_REG_TMR1H) = (uint8_t)(value >> 8);
    EPIC_REG8(PIC_REG_TMR1L) = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p Prescaler selection.
 * @return Divider ratio (1, 2, 4 or 8), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p)
{
    if ((unsigned int)p > 3U)
    {
        return 1U;
    }
    return pre_ratio[p];
}

/**
 * @brief Configure Timer1 from the handle without starting it.
 * @param h Handle with clock source, prescaler, reload and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the counter. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR1);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F1508_IRQ_TMR1);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_TMR1);
    }
    /* Stored before the timer runs, so an overflow never calls a stale slot. */
    g_t1_overflow_cb = h->OverflowCallback;
    EPIC_TIMER1_WriteCounter(h->ReloadValue);
    return EPIC_OK;
}

/**
 * @brief Disable the TMR1 interrupt and restore the POR state.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR1);
    /* T1CON POR is 0x00 (DS40001609E Table 3-9); the counter holds no POR. */
    EPIC_REG8(PIC_REG_T1CON) = 0U;
    EPIC_TIMER1_WriteCounter(0U);
    g_t1_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer1 from the handle.
 * @param h Handle holding the reload value, clock source and prescaler.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Reload first, then one T1CON write: the first overflow already uses
     * the new clock source and prescaler. */
    EPIC_TIMER1_WriteCounter(h->ReloadValue);
    timer1_write_con(h);
    return EPIC_OK;
}

/**
 * @brief Stop Timer1 by clearing T1CON<TMR1ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_TMR1ON);
    return EPIC_OK;
}

/**
 * @brief Acknowledge TMR1IF, then run the overflow callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR1IF and TMR1IE
 * are both set. Clears TMR1IF before the callback so the vector does not
 * re-enter.
 */
void TIMER1_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR1);
    if (g_t1_overflow_cb != NULL)
    {
        g_t1_overflow_cb();
    }
}
