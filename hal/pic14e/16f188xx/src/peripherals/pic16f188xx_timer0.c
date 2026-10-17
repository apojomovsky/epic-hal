/**
 * Timer0 driver, implementation (DS40001825F §27, Registers 27-1/27-2).
 * Every access names its register literally, so XC8 emits a bank select
 * per access; the handler keeps no cstack autos (ISR bank rule, family
 * MANUAL.md). The callback slot is a file-scope static written from
 * main context only, stored before the timer runs.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_timer0.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_t0_overflow_cb)(void) = NULL;

/**
 * @brief Check the clock source and both ratio fields of a handle.
 * @param h Handle to check.
 * @return 1 when every field is in range, 0 otherwise.
 */
static uint8_t fields_ok(const TIMER0_HandleTypeDef *h)
{
    return (((unsigned int)h->ClockSource <= (unsigned int)TIMER0_CS_LC1OUT) &&
            ((unsigned int)h->Prescaler <= 15U) &&
            ((unsigned int)h->Postscaler <= 15U)) ? 1U : 0U;
}

/**
 * @brief T0ASYNC value for a clock source: set for the asynchronous rows.
 * @param cs Clock source.
 * @return 1 for T0CKIPPS, LFINTOSC, SOSC and LC1OUT, else 0.
 */
static uint8_t async_for(TIMER0_ClockSourceTypeDef cs)
{
    if ((cs == TIMER0_CS_T0CKIPPS) || (cs == TIMER0_CS_T0CKIPPS_INV) ||
        (cs == TIMER0_CS_LFINTOSC) || (cs == TIMER0_CS_SOSC) ||
        (cs == TIMER0_CS_LC1OUT))
    {
        return 1U;
    }
    return 0U;
}

/**
 * @brief T0CON1 image for a handle (T0EN lives in T0CON0, untouched here).
 * @param h Validated handle.
 * @return T0CON1 value: source, sync bit and prescaler.
 */
static uint8_t con1_for(const TIMER0_HandleTypeDef *h)
{
    uint8_t con1 = (uint8_t)(((unsigned int)h->ClockSource << PIC_T0CON1_T0CS_SHIFT) &
        PIC_T0CON1_T0CS_MASK);
    if (async_for(h->ClockSource) != 0U)
    {
        con1 |= PIC_T0CON1_T0ASYNC;
    }
    return (uint8_t)(con1 | ((uint8_t)h->Prescaler & PIC_T0CON1_T0CKPS_MASK));
}

/**
 * @brief T0CON0 image for a handle without the enable bit.
 * @param h Validated handle.
 * @return T0CON0 value: width and postscaler, T0EN clear.
 */
static uint8_t con0_for(const TIMER0_HandleTypeDef *h)
{
    uint8_t con0 = (uint8_t)((uint8_t)h->Postscaler & PIC_T0CON0_T0OUTPS_MASK);
    if (h->Is16Bit != 0U)
    {
        con0 |= PIC_T0CON0_T016BIT;
    }
    return con0;
}

/**
 * @brief Convert a prescaler enum to its integer ratio (2^N, 1..32768).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    if ((unsigned int)p > 15U)
    {
        return 1U;
    }
    return (uint16_t)(1UL << (unsigned int)p);
}

/**
 * @brief Convert a postscaler enum to its integer ratio (N+1, 1..16).
 * @param p Postscaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PostscalerToRatio(TIMER0_PostscalerTypeDef p)
{
    if ((unsigned int)p > 15U)
    {
        return 1U;
    }
    return (uint16_t)((unsigned int)p + 1U);
}

/**
 * @brief Configure Timer0 from the handle without starting it.
 * @param h Handle with source, ratios, mode, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the output. */
    EPIC_REG8(PIC_REG_T0CON0) = con0_for(h);
    EPIC_REG8(PIC_REG_T0CON1) = con1_for(h);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR0);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_TMR0);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR0);
    }
    /* Stored before the timer runs, so an overflow never calls a stale slot. */
    s_t0_overflow_cb = h->OverflowCallback;
    if (h->Is16Bit != 0U)
    {
        EPIC_TIMER0_WriteCounter(h->Period);
    }
    else
    {
        EPIC_REG8(PIC_REG_TMR0H) = (uint8_t)h->Period;
        EPIC_REG8(PIC_REG_TMR0L) = 0U;
    }
    return EPIC_OK;
}

/**
 * @brief Disable the TMR0 interrupt and restore the POR state.
 * @details T0CON0 and T0CON1 power up 0x00, TMR0L 0x00, TMR0H 0xFF
 * (EDC por; DS40001825F Table 3-14).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR0);
    EPIC_REG8(PIC_REG_T0CON0) = 0U;
    EPIC_REG8(PIC_REG_T0CON1) = 0U;
    EPIC_REG8(PIC_REG_TMR0L) = 0U;
    EPIC_REG8(PIC_REG_TMR0H) = 0xFFU;
    s_t0_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer0 from the handle.
 * @details Reloads the period, then one T0CON0 write sets T0EN: the first
 * overflow already uses the new setup. In 16-bit mode the counter
 * preloads Period (overflow after 65536 minus Period ticks); in 8-bit
 * mode TMR0H keeps the match value and only TMR0L zeroes.
 * @param h Handle holding the period, width and postscaler (clock source,
 *          prescaler and interrupt gate stay as programmed by Init).
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    if (h->Is16Bit != 0U)
    {
        EPIC_TIMER0_WriteCounter(h->Period);
    }
    else
    {
        EPIC_REG8(PIC_REG_TMR0H) = (uint8_t)h->Period;
        EPIC_REG8(PIC_REG_TMR0L) = 0U;
    }
    EPIC_REG8(PIC_REG_T0CON0) = (uint8_t)(con0_for(h) | PIC_T0CON0_T0EN);
    return EPIC_OK;
}

/**
 * @brief Stop Timer0 by clearing T0CON0<T0EN>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T0CON0), PIC_T0CON0_T0EN);
    return EPIC_OK;
}

/**
 * @brief Read the 16-bit counter (TMR0L first, which latches TMR0H on silicon).
 * @return TMR0H:TMR0L; in 8-bit mode the high byte is the match value, not time.
 */
uint16_t EPIC_TIMER0_ReadCounter(void)
{
    uint8_t lo = EPIC_REG8(PIC_REG_TMR0L);
    uint8_t hi = EPIC_REG8(PIC_REG_TMR0H);
    return (uint16_t)(((uint16_t)hi << 8) | lo);
}

/**
 * @brief Write the counter: TMR0H then TMR0L in 16-bit mode, TMR0L only in
 * 8-bit mode (TMR0H holds the match value there and must survive).
 * @param value Counter value.
 */
void EPIC_TIMER0_WriteCounter(uint16_t value)
{
    if ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T016BIT) != 0U)
    {
        EPIC_REG8(PIC_REG_TMR0H) = (uint8_t)(value >> 8);
    }
    EPIC_REG8(PIC_REG_TMR0L) = (uint8_t)value;
}

/**
 * @brief Acknowledge TMR0IF, then run the overflow callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR0IF and TMR0IE
 * are both set. Clears TMR0IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void TIMER0_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR0);
    if (s_t0_overflow_cb != NULL)
    {
        s_t0_overflow_cb();
    }
}
