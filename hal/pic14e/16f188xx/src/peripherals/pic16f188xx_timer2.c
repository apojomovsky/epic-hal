/**
 * Timer2 driver, implementation (DS40001802H §29, Registers 29-1..29-4).
 * Every access names its register literally, so XC8 emits a bank select
 * per access; the handler keeps no cstack autos (ISR bank rule, family
 * MANUAL.md). The callback slot is a file-scope static written from
 * main context only, stored before the timer runs.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_timer2.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_t2_overflow_cb)(void) = NULL;

/**
 * @brief Check whether a T2HLT mode code is defined.
 * @param mode Mode code, 0..255.
 * @return 1 for 0x00..0x0F, 0x11..0x13, 0x16 and 0x17; 0 for the
 * reserved codes (Table 29-1).
 */
static uint8_t mode_ok(unsigned int mode)
{
    return ((mode <= 0x0FU) || ((mode >= 0x11U) && (mode <= 0x13U)) ||
            (mode == 0x16U) || (mode == 0x17U)) ? 1U : 0U;
}

/**
 * @brief Check every enumerated field of a handle.
 * @param h Handle to check.
 * @return 1 when each field is a defined code, 0 otherwise. Reset source
 * 1 (the Timer2 postscaled output itself) is reserved on Timer2.
 */
static uint8_t fields_ok(const TIMER2_HandleTypeDef *h)
{
    unsigned int rsel = (unsigned int)h->ResetSource;

    return (((unsigned int)h->ClockSource <= (unsigned int)TIMER2_CS_LC4OUT) &&
            ((unsigned int)h->Prescaler <= 7U) &&
            ((unsigned int)h->Postscaler <= 15U) &&
            (mode_ok((unsigned int)h->Mode) != 0U) &&
            (rsel <= (unsigned int)TIMER2_RST_LC4OUT) && (rsel != 1U)) ? 1U : 0U;
}

/**
 * @brief T2CON ratio image for a handle with the ON bit left clear.
 * @param h Validated handle.
 * @return T2CON value: CKPS and OUTPS.
 */
static uint8_t con_for(const TIMER2_HandleTypeDef *h)
{
    return (uint8_t)((((unsigned int)h->Prescaler << PIC_T2CON_CKPS_SHIFT) &
                      PIC_T2CON_CKPS_MASK) |
                     ((unsigned int)h->Postscaler & PIC_T2CON_OUTPS_MASK));
}

/**
 * @brief T2HLT image for a handle.
 * @param h Validated handle.
 * @return T2HLT value: PSYNC, CKPOL, CKSYNC and MODE.
 */
static uint8_t hlt_for(const TIMER2_HandleTypeDef *h)
{
    uint8_t hlt = (uint8_t)((unsigned int)h->Mode & PIC_T2HLT_MODE_MASK);

    if (h->SyncPrescaler != 0U)
    {
        hlt |= PIC_T2HLT_PSYNC;
    }
    if (h->FallingEdge != 0U)
    {
        hlt |= PIC_T2HLT_CKPOL;
    }
    if (h->SyncOn != 0U)
    {
        hlt |= PIC_T2HLT_CKSYNC;
    }
    return hlt;
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
 * @return Divider ratio (1..128, 2^N), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p)
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
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p)
{
    if ((unsigned int)p > 15U)
    {
        return 1U;
    }
    return (uint16_t)((unsigned int)p + 1U);
}

/**
 * @brief Configure Timer2 from the handle without starting it.
 * @param h Handle with clock, ratios, mode, reset source, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the output. */
    EPIC_REG8(PIC_REG_T2CON) = con_for(h);
    EPIC_REG8(PIC_REG_T2CLKCON) = (uint8_t)((unsigned int)h->ClockSource & PIC_T2CLKCON_CS_MASK);
    EPIC_REG8(PIC_REG_T2RST) = (uint8_t)((unsigned int)h->ResetSource & PIC_T2RST_RSEL_MASK);
    EPIC_REG8(PIC_REG_T2HLT) = hlt_for(h);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR2);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_TMR2);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR2);
    }
    /* Stored before the timer runs, so a match never calls a stale slot. */
    s_t2_overflow_cb = h->OverflowCallback;
    EPIC_TIMER2_WritePeriod(h->Period);
    return EPIC_OK;
}

/**
 * @brief Disable the TMR2 interrupt and restore the POR state.
 * @details T2CON, T2HLT, T2CLKCON, T2RST and TMR2 power up 0x00, PR2 0xFF
 * (EDC por; DS40001802H §29.1).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR2);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR2);
    EPIC_REG8(PIC_REG_T2CON) = 0U;
    EPIC_REG8(PIC_REG_T2HLT) = 0U;
    EPIC_REG8(PIC_REG_T2CLKCON) = 0U;
    EPIC_REG8(PIC_REG_T2RST) = 0U;
    EPIC_TIMER2_WriteCounter(0U);
    EPIC_TIMER2_WritePeriod(0xFFU);
    s_t2_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer2 from the handle.
 * @param h Handle holding the period, prescaler and postscaler.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* PR2 first, then a zeroed counter and one T2CON write: the first match
     * already uses the new period. */
    EPIC_TIMER2_WritePeriod(h->Period);
    EPIC_TIMER2_WriteCounter(0U);
    EPIC_REG8(PIC_REG_T2CON) = (uint8_t)(con_for(h) | PIC_T2CON_ON);
    return EPIC_OK;
}

/**
 * @brief Stop Timer2 by clearing T2CON<ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T2CON), PIC_T2CON_ON);
    return EPIC_OK;
}

/**
 * @brief Acknowledge TMR2IF, then run the match callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR2IF and TMR2IE
 * are both set. Clears TMR2IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void TIMER2_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR2);
    if (s_t2_overflow_cb != NULL)
    {
        s_t2_overflow_cb();
    }
}
