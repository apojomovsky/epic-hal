/**
 * Timer4 driver, implementation (DS40001802H §29, Registers 29-1..29-4).
 * Every access names its register literally, so XC8 emits a bank select
 * per access; the handler keeps no cstack autos (ISR bank rule, family
 * MANUAL.md). The callback slot is a file-scope static written from
 * main context only, stored before the timer runs.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_timer4.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_t4_overflow_cb)(void) = NULL;

/**
 * @brief Check every enumerated field of a handle.
 * @param h Handle to check.
 * @return 1 when each field is a defined code, 0 otherwise.
 */
static uint8_t fields_ok(const TIMER4_HandleTypeDef *h)
{
    return (((unsigned int)h->ClockSource <= (unsigned int)TIMER4_CS_LC4OUT) &&
            ((unsigned int)h->Prescaler <= 7U) &&
            ((unsigned int)h->Postscaler <= 15U)) ? 1U : 0U;
}

/**
 * @brief T4CON ratio image for a handle with the ON bit left clear.
 * @param h Validated handle.
 * @return T4CON value: CKPS and OUTPS.
 */
static uint8_t con_for(const TIMER4_HandleTypeDef *h)
{
    return (uint8_t)((((unsigned int)h->Prescaler << PIC_T4CON_CKPS_SHIFT) &
                      PIC_T4CON_CKPS_MASK) |
                     ((unsigned int)h->Postscaler & PIC_T4CON_OUTPS_MASK));
}

/**
 * @brief Read the current TMR4 value.
 * @return TMR4 value, 0..255.
 */
uint8_t EPIC_TIMER4_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR4);
}

/**
 * @brief Write the TMR4 counter.
 * @param value Counter value, 0..255.
 */
void EPIC_TIMER4_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR4) = value;
}

/**
 * @brief Read the PR4 period register.
 * @return PR4 value, 0..255.
 */
uint8_t EPIC_TIMER4_ReadPeriod(void)
{
    return EPIC_REG8(PIC_REG_PR4);
}

/**
 * @brief Write the PR4 period register.
 * @param period Period value, 0..255.
 */
void EPIC_TIMER4_WritePeriod(uint8_t period)
{
    EPIC_REG8(PIC_REG_PR4) = period;
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p Prescaler selection.
 * @return Divider ratio (1..128, 2^N), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER4_PrescalerToRatio(TIMER4_PrescalerTypeDef p)
{
    if ((unsigned int)p > 7U)
    {
        return 1U;
    }
    return (uint16_t)(1U << (unsigned int)p);
}

/**
 * @brief Convert a postscaler enum to its integer ratio.
 * @param p Postscaler selection.
 * @return Divider ratio (N+1, 1..16), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER4_PostscalerToRatio(TIMER4_PostscalerTypeDef p)
{
    if ((unsigned int)p > 15U)
    {
        return 1U;
    }
    return (uint16_t)((unsigned int)p + 1U);
}

/**
 * @brief Configure Timer4 from the handle without starting it.
 * @param h Handle with clock, ratios, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Init(const TIMER4_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the output.
     * T4HLT and T4RST are zeroed so a stale limit mode cannot gate the count. */
    EPIC_REG8(PIC_REG_T4CON) = con_for(h);
    EPIC_REG8(PIC_REG_T4CLKCON) = (uint8_t)((unsigned int)h->ClockSource & PIC_T4CLKCON_CS_MASK);
    EPIC_REG8(PIC_REG_T4RST) = 0U;
    EPIC_REG8(PIC_REG_T4HLT) = 0U;
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR4);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_TMR4);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR4);
    }
    /* Stored before the timer runs, so a match never calls a stale slot. */
    s_t4_overflow_cb = h->OverflowCallback;
    EPIC_TIMER4_WritePeriod(h->Period);
    return EPIC_OK;
}

/**
 * @brief Disable the TMR4 interrupt and restore the POR state.
 * @details T4CON, T4HLT, T4CLKCON, T4RST and TMR4 power up 0x00, PR4 0xFF
 * (EDC por; DS40001802H §29.1).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER4_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR4);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR4);
    EPIC_REG8(PIC_REG_T4CON) = 0U;
    EPIC_REG8(PIC_REG_T4HLT) = 0U;
    EPIC_REG8(PIC_REG_T4CLKCON) = 0U;
    EPIC_REG8(PIC_REG_T4RST) = 0U;
    EPIC_TIMER4_WriteCounter(0U);
    EPIC_TIMER4_WritePeriod(0xFFU);
    s_t4_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer4 from the handle.
 * @param h Handle holding the period, prescaler and postscaler.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Start(const TIMER4_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* PR4 first, then a zeroed counter and one T4CON write: the first match
     * already uses the new period. */
    EPIC_TIMER4_WritePeriod(h->Period);
    EPIC_TIMER4_WriteCounter(0U);
    EPIC_REG8(PIC_REG_T4CON) = (uint8_t)(con_for(h) | PIC_T4CON_ON);
    return EPIC_OK;
}

/**
 * @brief Stop Timer4 by clearing T4CON<ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T4CON), PIC_T4CON_ON);
    return EPIC_OK;
}

/**
 * @brief Acknowledge TMR4IF, then run the match callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR4IF and TMR4IE
 * are both set. Clears TMR4IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void TIMER4_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR4);
    if (s_t4_overflow_cb != NULL)
    {
        s_t4_overflow_cb();
    }
}
