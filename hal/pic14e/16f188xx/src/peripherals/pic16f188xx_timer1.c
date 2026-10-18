/**
 * Timer1 driver, implementation (DS40001802H §28, Registers 28-1..28-4).
 * Every access names its register literally, so XC8 emits a bank select
 * per access; the handler keeps no cstack autos (ISR bank rule, family
 * MANUAL.md). The callback slot is a file-scope static written from
 * main context only, stored before the timer runs.
 */
#include <stddef.h>
#include "peripherals/pic16f188xx_timer1.h"
#include "core/pic16f188xx_irq.h"

/* ISR callback slot, written from main context only. A scalar, not an
 * array element: the handler references a fixed symbol. */
static void (*s_t1_overflow_cb)(void) = NULL;

/**
 * @brief Check the clock source, prescaler and gate source of a handle.
 * @param h Handle to check.
 * @return 1 when every field is a defined code, 0 otherwise. Clock code
 * 9 and gate code 2 are reserved on Timer1, as are gate codes above 24.
 */
static uint8_t fields_ok(const TIMER1_HandleTypeDef *h)
{
    unsigned int cs = (unsigned int)h->ClockSource;
    unsigned int gss = (unsigned int)h->Gate.Source;

    return ((cs <= 15U) && (cs != 9U) &&
            ((unsigned int)h->Prescaler <= 3U) &&
            (gss <= 24U) && (gss != 2U)) ? 1U : 0U;
}

/**
 * @brief T1CON image for a handle with the ON bit left clear.
 * @param h Validated handle.
 * @return T1CON value: prescaler, SYNC and RD16.
 */
static uint8_t con_for(const TIMER1_HandleTypeDef *h)
{
    uint8_t con = (uint8_t)(((unsigned int)h->Prescaler << PIC_T1CON_CKPS_SHIFT) &
        PIC_T1CON_CKPS_MASK);

    con |= PIC_T1CON_RD16;
    if (h->Async != 0U)
    {
        con |= PIC_T1CON_nSYNC;
    }
    return con;
}

/**
 * @brief T1GCON image for a gate configuration (GGO and GVAL stay clear).
 * @param g Gate configuration.
 * @return T1GCON value: GE, GPOL, GTM and GSPM.
 */
static uint8_t gcon_for(const TIMER1_GateTypeDef *g)
{
    uint8_t gcon = 0U;

    if (g->Enable != 0U)
    {
        gcon |= PIC_T1GCON_GE;
    }
    if (g->ActiveHigh != 0U)
    {
        gcon |= PIC_T1GCON_GPOL;
    }
    if (g->Toggle != 0U)
    {
        gcon |= PIC_T1GCON_GTM;
    }
    if (g->SinglePulse != 0U)
    {
        gcon |= PIC_T1GCON_GSPM;
    }
    return gcon;
}

/**
 * @brief Read the 16-bit counter atomically.
 * @details TMR1L is read first: with RD16 set that read latches TMR1H
 * into the buffer the TMR1H read returns (DS40001802H §28.4), so no
 * rollover can tear the pair. Init and Start set RD16.
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
    return (uint16_t)(1U << (unsigned int)p);
}

/**
 * @brief Configure Timer1 from the handle without starting it.
 * @param h Handle with clock, prescaler, gate, reload and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Halted while configured, so no half-written setup reaches the counter. */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_ON);
    EPIC_REG8(PIC_REG_T1CLK) = (uint8_t)((unsigned int)h->ClockSource & PIC_T1CLK_CS_MASK);
    EPIC_REG8(PIC_REG_T1GATE) = (uint8_t)((unsigned int)h->Gate.Source & PIC_T1GATE_GSS_MASK);
    EPIC_REG8(PIC_REG_T1GCON) = gcon_for(&h->Gate);
    EPIC_REG8(PIC_REG_T1CON) = con_for(h);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR1);
    if (h->OverflowCallback != NULL)
    {
        EPIC_IRQ_Enable(PIC16F188XX_IRQ_TMR1);
    }
    else
    {
        EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR1);
    }
    /* Stored before the timer runs, so an overflow never calls a stale slot. */
    s_t1_overflow_cb = h->OverflowCallback;
    EPIC_TIMER1_WriteCounter(h->ReloadValue);
    return EPIC_OK;
}

/**
 * @brief Disable the TMR1 interrupt and restore the POR state.
 * @details T1CON, T1GCON (GVAL reads x), T1GATE and T1CLK power up 0x00
 * (EDC por); the counter holds no POR value.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_TMR1);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR1);
    EPIC_REG8(PIC_REG_T1CON) = 0U;
    EPIC_REG8(PIC_REG_T1GCON) = 0U;
    EPIC_REG8(PIC_REG_T1GATE) = 0U;
    EPIC_REG8(PIC_REG_T1CLK) = 0U;
    EPIC_TIMER1_WriteCounter(0U);
    s_t1_overflow_cb = NULL;
    return EPIC_OK;
}

/**
 * @brief Start Timer1 from the handle.
 * @param h Handle holding the reload value, prescaler and Async.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h)
{
    if ((h == NULL) || (fields_ok(h) == 0U))
    {
        return EPIC_INVALID;
    }

    /* Reload first, then one T1CON write: the first overflow already uses
     * the new prescaler. */
    EPIC_TIMER1_WriteCounter(h->ReloadValue);
    EPIC_REG8(PIC_REG_T1CON) = (uint8_t)(con_for(h) | PIC_T1CON_ON);
    return EPIC_OK;
}

/**
 * @brief Stop Timer1 by clearing T1CON<ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_T1CON), PIC_T1CON_ON);
    return EPIC_OK;
}

/**
 * @brief Arm a single-pulse gate acquisition by setting GGO/DONE.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_GateArm(void)
{
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_T1GCON), PIC_T1GCON_GGO_nDONE);
    return EPIC_OK;
}

/**
 * @brief Report whether a single-pulse acquisition is still pending.
 * @return 1 while GGO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_TIMER1_GateBusy(void)
{
    return ((EPIC_REG8(PIC_REG_T1GCON) & PIC_T1GCON_GGO_nDONE) != 0U) ? 1U : 0U;
}

/**
 * @brief Read the live gate state.
 * @return 1 when GVAL is set, 0 otherwise.
 */
uint8_t EPIC_TIMER1_GateValue(void)
{
    return ((EPIC_REG8(PIC_REG_T1GCON) & PIC_T1GCON_GVAL) != 0U) ? 1U : 0U;
}

/**
 * @brief Acknowledge TMR1IF, then run the overflow callback if one is set.
 * @details Called from epic_dispatch_all_irqs only when TMR1IF and TMR1IE
 * are both set. Clears TMR1IF before the callback so the vector does not
 * re-enter. No cstack autos: every access names its register literally.
 */
void TIMER1_IRQHandler(void)
{
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR1);
    if (s_t1_overflow_cb != NULL)
    {
        s_t1_overflow_cb();
    }
}
