/*
 * IRQ driver: every SFR names a compile-time-constant PIC_REG_* token,
 * never a runtime address (on PIC18 a runtime address compiles to the
 * table mechanism, not a data access; see the family README). GIEH/GIEL
 * act as one switch; enabling sets IPEN (RCON<7>). No SPP source
 * (DS39631E Table 1-1); cases stop at EEPROM.
 */

#include "core/pic18_irq.h"

/* `reg` must be a literal `PIC_REG_*` token so it stays a compile-time
 * constant through `epic_sfr_read8`/`write8` (see file header). */
#define SFR_SET_BIT(reg, mask) \
    epic_sfr_write8((reg), (uint8_t)(epic_sfr_read8(reg) | (mask)))
#define SFR_CLR_BIT(reg, mask) \
    epic_sfr_write8((reg), (uint8_t)(epic_sfr_read8(reg) & (uint8_t)~(mask)))

/**
 * @brief  Globally mask all interrupts by clearing the master enable(s)
 *         (INTCON<GIEH/GIEL>, DS39631E §9.0). In priority mode both GIEH
 *         and GIEL are cleared.
 * @return 1 if any master enable was set (interrupts were on), else 0.
 */
uint8_t EPIC_IRQ_Disable(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    uint8_t prev = (intcon & (PIC_INTCON_GIEH | PIC_INTCON_GIEL)) ? 1U : 0U;
    SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
    SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    return prev;
}

/**
 * @brief  Restore the master interrupt enable(s). `prev_state` is the
 *         value returned by @ref EPIC_IRQ_Disable. Restoring to "on"
 *         also ensures IPEN = 1 (priority mode) so the two-vector scheme
 *         is active.
 * @param prev_state 1 to enable all interrupts, 0 to keep them masked.
 */
void EPIC_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state) {
        /* Activate the two-vector priority scheme (DS39631E §9.0, RCON<7>)
         * before enabling the masters, so high/low routing is in effect. */
        SFR_SET_BIT(PIC_REG_RCON, PIC_RCON_IPEN);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    } else {
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    }
}

/**
 * @brief  Enable one interrupt source. The peripheral enable bit lives in
 *         INTCON per the source. The master enable(s) must still be set
 *         via @ref EPIC_IRQ_Restore for the source to fire.
 * @param irq the interrupt source to enable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_Enable(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_RB:   SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0: SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_TMR0IE); break;
    default: break;
    }
}

/**
 * @brief  Disable one interrupt source. The peripheral enable bit lives
 *         in INTCON per the source.
 * @param irq the interrupt source to disable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_DisableSrc(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_RB:   SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0: SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_TMR0IE); break;
    default: break;
    }
}

/**
 * @brief  Clear the interrupt flag of `irq`. MUST be called inside the
 *         ISR before re-enabling interrupts to avoid an infinite
 *         re-entry (DS39631E §9.0).
 * @param irq the interrupt source whose flag is cleared.
 */
void EPIC_IRQ_ClearFlag(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_RB:   SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_RBIF);   break;
    case PIC18_IRQ_TMR0: SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_TMR0IF); break;
    default: break;
    }
}

/**
 * @brief  Return the current pending state of `irq`.
 * @param irq the interrupt source to query (a @ref PIC18_IRQn value).
 * @return 1 if the source's flag is set (pending), else 0.
 */
uint8_t EPIC_IRQ_GetFlag(PIC18_IRQn irq)
{
    switch (irq) {
    case PIC18_IRQ_RB:   return (epic_sfr_read8(PIC_REG_INTCON) & PIC_INTCON_RBIF)   ? 1U : 0U;
    case PIC18_IRQ_TMR0: return (epic_sfr_read8(PIC_REG_INTCON) & PIC_INTCON_TMR0IF) ? 1U : 0U;
    default: return 0U;
    }
}

/**
 * @brief  Set the priority of `irq` (high or low vector). Writes the
 *         matching bit in INTCON2. Takes effect only in priority mode
 *         (IPEN = 1, which @ref EPIC_IRQ_Restore enables).
 * @param irq  the interrupt source.
 * @param prio the desired priority (high or low vector).
 */
void EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio)
{
    uint8_t high = (prio == EPIC_IRQ_PRIORITY_HIGH) ? 1U : 0U;
    switch (irq) {
    case PIC18_IRQ_RB:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        break;
    case PIC18_IRQ_TMR0:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        break;
    default: break;
    }
}
