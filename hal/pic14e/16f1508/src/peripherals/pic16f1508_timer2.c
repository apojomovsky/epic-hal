/* PIC16F1508/1509 Timer2 driver, implementation (DS40001609E §20.0). */

#include <stddef.h>
#include "peripherals/pic16f1508_timer2.h"
#include "core/pic16f1508_irq.h"

/* T2CON prescaler, DS40001609E Register 20-1:
 * 00 -> 1:1, 01 -> 1:4, 10 -> 1:16, 11 -> 1:64. */
static const uint8_t pre_ratio[4] = { 1, 4, 16, 64 };

/* T2CON postscaler, DS40001609E Register 20-1: 1:(N+1). */
static const uint8_t post_ratio[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
void (*g_t2_overflow_cb)(void) = NULL;

/**
 * @brief Check the prescaler and postscaler fields of a handle.
 * @param h Handle to check.
 * @return 1 when both ratios are in range, 0 otherwise.
 */
static uint8_t ratios_ok(const TIMER2_HandleTypeDef *h)
{
    return (((unsigned int)h->Prescaler <= 3U) && ((unsigned int)h->Postscaler <= 15U)) ? 1U : 0U;
}

/**
 * @brief Read the current TMR2 value.
 * @return TMR2 value, 0..255.
 */
uint8_t EPIC_TIMER2_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR2);
}

/**
 * @brief Write the TMR2 counter.
 * @param value Counter value, 0..255.
 */
void EPIC_TIMER2_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR2) = value;
}

/**
 * @brief Read the PR2 period register.
 * @return PR2 value, 0..255.
 */
uint8_t EPIC_TIMER2_ReadPeriod(void)
{
    return EPIC_REG8(PIC_REG_PR2);
}

/**
 * @brief Write the PR2 period register.
 * @param period Period value, 0..255.
 */
void EPIC_TIMER2_WritePeriod(uint8_t period)
{
    EPIC_REG8(PIC_REG_PR2) = period;
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p Prescaler selection.
 * @return Divider ratio (1, 4, 16 or 64), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p)
{
    if ((unsigned int)p > 3U)
    {
        return 1U;
    }
    return pre_ratio[p];
}

/**
 * @brief Convert a postscaler enum to its integer ratio.
 * @param p Postscaler selection.
 * @return Divider ratio (1..16), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p)
{
    if ((unsigned int)p > 15U)
    {
        return 1U;
    }
    return post_ratio[p];
}

/**
 * @brief Configure Timer2 from the handle without starting it.
 * @param h Handle with prescaler, postscaler, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h)
{
    if ((h == NULL) || (ratios_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the output. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR2);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F1508_IRQ_TMR2);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_TMR2);
    }
    /* Stored before the timer runs, so a match never calls a stale slot. */
    g_t2_overflow_cb = h->OverflowCallback;
    EPIC_TIMER2_WritePeriod(h->Period);
    return EPIC_OK;
}

/**
 * @brief Disable the TMR2 interrupt and restore the POR state.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_TMR2);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR2);
    /* T2CON POR is 0x00, PR2 POR is 0xFF (DS40001609E §20.1 and Table 3-9). */
    EPIC_REG8(PIC_REG_T2CON) = 0U;
    EPIC_TIMER2_WritePeriod(0xFFU);
    g_t2_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer2 from the handle.
 * @param h Handle holding the period, prescaler and postscaler.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h)
{
    uint8_t con;

    if ((h == NULL) || (ratios_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* PR2 first, then a zeroed counter and one T2CON write: the first match
     * already uses the new period. */
    EPIC_TIMER2_WritePeriod(h->Period);
    EPIC_TIMER2_WriteCounter(0U);
    con = (uint8_t)(((uint8_t)((unsigned int)h->Postscaler << PIC_T2CON_T2OUTPS_SHIFT))
        | PIC_T2CON_TMR2ON | (uint8_t)((unsigned int)h->Prescaler & PIC_T2CON_T2CKPS_MASK));
    EPIC_REG8(PIC_REG_T2CON) = con;
    return EPIC_OK;
}

/**
 * @brief Stop Timer2 by clearing T2CON<TMR2ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);
    return EPIC_OK;
}

/**
 * @brief Acknowledge TMR2IF, then run the match callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR2IF and TMR2IE
 * are both set. Clears TMR2IF before the callback so the vector does not
 * re-enter.
 */
void TIMER2_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR2);
    if (g_t2_overflow_cb != NULL)
    {
        g_t2_overflow_cb();
    }
}
