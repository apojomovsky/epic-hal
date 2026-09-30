/*
 * IRQ driver: every SFR names a compile-time-constant PIC_REG_* token,
 * so each branch of the `switch` on `irq` stays a literal address
 * through epic_sfr_read8/write8. A runtime SFR address on PIC18 is the
 * known XC8 program-memory-table misdirection shape
 * (docs/adding-a-device.md §4 item 9); branching before any SFR touch
 * keeps every access literal (same shape as pic18f2520's backend).
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
 *         (INTCON<GIEH/GIEL>, DS39609B §9.0). In priority mode both GIEH
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
    if (prev_state)
    {
        /* Activate the two-vector priority scheme (DS39609B §9.0, RCON<7>)
         * before enabling the masters, so high/low routing is in effect. */
        SFR_SET_BIT(PIC_REG_RCON, PIC_RCON_IPEN);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_SET_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    }
    else
    {
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEH);
        SFR_CLR_BIT(PIC_REG_INTCON, PIC_INTCON_GIEL);
    }
}

/**
 * @brief  Enable one interrupt source. The peripheral enable bit lives in
 *         INTCON / INTCON3 / PIE1-3 per the source. The master enable(s)
 *         must still be set via @ref EPIC_IRQ_Restore for the source to
 *         fire.
 * @param irq the interrupt source to enable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_Enable(PIC18_IRQn irq)
{
    switch (irq)
    {
    case PIC18_IRQ_INT0:     SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IE);  break;
    case PIC18_IRQ_INT1:     SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IE); break;
    case PIC18_IRQ_INT2:     SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IE); break;
    case PIC18_IRQ_INT3:     SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT3IE); break;
    case PIC18_IRQ_RB:       SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0:     SFR_SET_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IE); break;
    case PIC18_IRQ_TMR1:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR1IE);   break;
    case PIC18_IRQ_TMR2:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR2IE);   break;
    case PIC18_IRQ_TMR3:     SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_TMR3IE);   break;
    case PIC18_IRQ_TMR4:     SFR_SET_BIT(PIC_REG_PIE3,    PIC_PIE3_TMR4IE);   break;
    case PIC18_IRQ_CCP1:     SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_CCP1IE);   break;
    case PIC18_IRQ_CCP2:     SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_CCP2IE);   break;
    case PIC18_IRQ_CCP3:     SFR_SET_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP3IE);   break;
    case PIC18_IRQ_CCP4:     SFR_SET_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP4IE);   break;
    case PIC18_IRQ_CCP5:     SFR_SET_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP5IE);   break;
    case PIC18_IRQ_SSP:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_SSPIE);    break;
    case PIC18_IRQ_USART1_TX: SFR_SET_BIT(PIC_REG_PIE1,   PIC_PIE1_TXIE);     break;
    case PIC18_IRQ_USART1_RX: SFR_SET_BIT(PIC_REG_PIE1,   PIC_PIE1_RCIE);     break;
    case PIC18_IRQ_USART2_TX: SFR_SET_BIT(PIC_REG_PIE3,   PIC_PIE3_TX2IE);    break;
    case PIC18_IRQ_USART2_RX: SFR_SET_BIT(PIC_REG_PIE3,   PIC_PIE3_RC2IE);    break;
    case PIC18_IRQ_ADC:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_ADIE);     break;
    case PIC18_IRQ_CMP:      SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_CMIE);     break;
    case PIC18_IRQ_EEPROM:   SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_EEIE);     break;
    case PIC18_IRQ_LVD:      SFR_SET_BIT(PIC_REG_PIE2,    PIC_PIE2_LVDIE);    break;
    case PIC18_IRQ_PSP:      SFR_SET_BIT(PIC_REG_PIE1,    PIC_PIE1_PSPIE);    break;
    default: break;
    }
}

/**
 * @brief  Disable one interrupt source. The peripheral enable bit lives
 *         in INTCON / INTCON3 / PIE1-3 per the source.
 * @param irq the interrupt source to disable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_DisableSrc(PIC18_IRQn irq)
{
    switch (irq)
    {
    case PIC18_IRQ_INT0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IE);  break;
    case PIC18_IRQ_INT1:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IE); break;
    case PIC18_IRQ_INT2:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IE); break;
    case PIC18_IRQ_INT3:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT3IE); break;
    case PIC18_IRQ_RB:       SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIE);   break;
    case PIC18_IRQ_TMR0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IE); break;
    case PIC18_IRQ_TMR1:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR1IE);   break;
    case PIC18_IRQ_TMR2:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_TMR2IE);   break;
    case PIC18_IRQ_TMR3:     SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_TMR3IE);   break;
    case PIC18_IRQ_TMR4:     SFR_CLR_BIT(PIC_REG_PIE3,    PIC_PIE3_TMR4IE);   break;
    case PIC18_IRQ_CCP1:     SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_CCP1IE);   break;
    case PIC18_IRQ_CCP2:     SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_CCP2IE);   break;
    case PIC18_IRQ_CCP3:     SFR_CLR_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP3IE);   break;
    case PIC18_IRQ_CCP4:     SFR_CLR_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP4IE);   break;
    case PIC18_IRQ_CCP5:     SFR_CLR_BIT(PIC_REG_PIE3,    PIC_PIE3_CCP5IE);   break;
    case PIC18_IRQ_SSP:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_SSPIE);    break;
    case PIC18_IRQ_USART1_TX: SFR_CLR_BIT(PIC_REG_PIE1,   PIC_PIE1_TXIE);     break;
    case PIC18_IRQ_USART1_RX: SFR_CLR_BIT(PIC_REG_PIE1,   PIC_PIE1_RCIE);     break;
    case PIC18_IRQ_USART2_TX: SFR_CLR_BIT(PIC_REG_PIE3,   PIC_PIE3_TX2IE);    break;
    case PIC18_IRQ_USART2_RX: SFR_CLR_BIT(PIC_REG_PIE3,   PIC_PIE3_RC2IE);    break;
    case PIC18_IRQ_ADC:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_ADIE);     break;
    case PIC18_IRQ_CMP:      SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_CMIE);     break;
    case PIC18_IRQ_EEPROM:   SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_EEIE);     break;
    case PIC18_IRQ_LVD:      SFR_CLR_BIT(PIC_REG_PIE2,    PIC_PIE2_LVDIE);    break;
    case PIC18_IRQ_PSP:      SFR_CLR_BIT(PIC_REG_PIE1,    PIC_PIE1_PSPIE);    break;
    default: break;
    }
}

/**
 * @brief  Clear the interrupt flag of `irq`. MUST be called inside the
 *         ISR before re-enabling interrupts to avoid an infinite
 *         re-entry (DS39609B §9.0).
 * @param irq the interrupt source whose flag is cleared.
 */
void EPIC_IRQ_ClearFlag(PIC18_IRQn irq)
{
    switch (irq)
    {
    case PIC18_IRQ_INT0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_INT0IF);  break;
    case PIC18_IRQ_INT1:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IF); break;
    case PIC18_IRQ_INT2:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IF); break;
    case PIC18_IRQ_INT3:     SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT3IF); break;
    case PIC18_IRQ_RB:       SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_RBIF);   break;
    case PIC18_IRQ_TMR0:     SFR_CLR_BIT(PIC_REG_INTCON,  PIC_INTCON_TMR0IF); break;
    case PIC18_IRQ_TMR1:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_TMR1IF);   break;
    case PIC18_IRQ_TMR2:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_TMR2IF);   break;
    case PIC18_IRQ_TMR3:     SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_TMR3IF);   break;
    case PIC18_IRQ_TMR4:     SFR_CLR_BIT(PIC_REG_PIR3,    PIC_PIR3_TMR4IF);   break;
    case PIC18_IRQ_CCP1:     SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_CCP1IF);   break;
    case PIC18_IRQ_CCP2:     SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_CCP2IF);   break;
    case PIC18_IRQ_CCP3:     SFR_CLR_BIT(PIC_REG_PIR3,    PIC_PIR3_CCP3IF);   break;
    case PIC18_IRQ_CCP4:     SFR_CLR_BIT(PIC_REG_PIR3,    PIC_PIR3_CCP4IF);   break;
    case PIC18_IRQ_CCP5:     SFR_CLR_BIT(PIC_REG_PIR3,    PIC_PIR3_CCP5IF);   break;
    case PIC18_IRQ_SSP:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_SSPIF);    break;
    case PIC18_IRQ_USART1_TX: SFR_CLR_BIT(PIC_REG_PIR1,   PIC_PIR1_TXIF);     break;
    case PIC18_IRQ_USART1_RX: SFR_CLR_BIT(PIC_REG_PIR1,   PIC_PIR1_RCIF);     break;
    case PIC18_IRQ_USART2_TX: SFR_CLR_BIT(PIC_REG_PIR3,   PIC_PIR3_TX2IF);    break;
    case PIC18_IRQ_USART2_RX: SFR_CLR_BIT(PIC_REG_PIR3,   PIC_PIR3_RC2IF);    break;
    case PIC18_IRQ_ADC:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_ADIF);     break;
    case PIC18_IRQ_CMP:      SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_CMIF);     break;
    case PIC18_IRQ_EEPROM:   SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_EEIF);     break;
    case PIC18_IRQ_LVD:      SFR_CLR_BIT(PIC_REG_PIR2,    PIC_PIR2_LVDIF);    break;
    case PIC18_IRQ_PSP:      SFR_CLR_BIT(PIC_REG_PIR1,    PIC_PIR1_PSPIF);    break;
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
    switch (irq)
    {
    case PIC18_IRQ_INT0:     return (epic_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_INT0IF)  ? 1U : 0U;
    case PIC18_IRQ_INT1:     return (epic_sfr_read8(PIC_REG_INTCON3) & PIC_INTCON3_INT1IF) ? 1U : 0U;
    case PIC18_IRQ_INT2:     return (epic_sfr_read8(PIC_REG_INTCON3) & PIC_INTCON3_INT2IF) ? 1U : 0U;
    case PIC18_IRQ_INT3:     return (epic_sfr_read8(PIC_REG_INTCON3) & PIC_INTCON3_INT3IF) ? 1U : 0U;
    case PIC18_IRQ_RB:       return (epic_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_RBIF)    ? 1U : 0U;
    case PIC18_IRQ_TMR0:     return (epic_sfr_read8(PIC_REG_INTCON)  & PIC_INTCON_TMR0IF)  ? 1U : 0U;
    case PIC18_IRQ_TMR1:     return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_TMR1IF)    ? 1U : 0U;
    case PIC18_IRQ_TMR2:     return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_TMR2IF)    ? 1U : 0U;
    case PIC18_IRQ_TMR3:     return (epic_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_TMR3IF)    ? 1U : 0U;
    case PIC18_IRQ_TMR4:     return (epic_sfr_read8(PIC_REG_PIR3)    & PIC_PIR3_TMR4IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP1:     return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_CCP1IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP2:     return (epic_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_CCP2IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP3:     return (epic_sfr_read8(PIC_REG_PIR3)    & PIC_PIR3_CCP3IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP4:     return (epic_sfr_read8(PIC_REG_PIR3)    & PIC_PIR3_CCP4IF)    ? 1U : 0U;
    case PIC18_IRQ_CCP5:     return (epic_sfr_read8(PIC_REG_PIR3)    & PIC_PIR3_CCP5IF)    ? 1U : 0U;
    case PIC18_IRQ_SSP:      return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_SSPIF)     ? 1U : 0U;
    case PIC18_IRQ_USART1_TX: return (epic_sfr_read8(PIC_REG_PIR1)   & PIC_PIR1_TXIF)      ? 1U : 0U;
    case PIC18_IRQ_USART1_RX: return (epic_sfr_read8(PIC_REG_PIR1)   & PIC_PIR1_RCIF)      ? 1U : 0U;
    case PIC18_IRQ_USART2_TX: return (epic_sfr_read8(PIC_REG_PIR3)   & PIC_PIR3_TX2IF)     ? 1U : 0U;
    case PIC18_IRQ_USART2_RX: return (epic_sfr_read8(PIC_REG_PIR3)   & PIC_PIR3_RC2IF)     ? 1U : 0U;
    case PIC18_IRQ_ADC:      return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_ADIF)      ? 1U : 0U;
    case PIC18_IRQ_CMP:      return (epic_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_CMIF)      ? 1U : 0U;
    case PIC18_IRQ_EEPROM:   return (epic_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_EEIF)      ? 1U : 0U;
    case PIC18_IRQ_LVD:      return (epic_sfr_read8(PIC_REG_PIR2)    & PIC_PIR2_LVDIF)     ? 1U : 0U;
    case PIC18_IRQ_PSP:      return (epic_sfr_read8(PIC_REG_PIR1)    & PIC_PIR1_PSPIF)     ? 1U : 0U;
    default: return 0U;
    }
}

/**
 * @brief  Set the priority of `irq` (high or low vector). Writes the
 *         matching bit in INTCON2 / INTCON3 / IPR1-3. INT0 has no
 *         priority bit (always high); setting its priority is a no-op.
 *         Takes effect only in priority mode (IPEN = 1, which @ref
 *         EPIC_IRQ_Restore enables).
 * @param irq  the interrupt source.
 * @param prio the desired priority (high or low vector).
 */
void EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio)
{
    uint8_t high = (prio == EPIC_IRQ_PRIORITY_HIGH) ? 1U : 0U;
    switch (irq)
    {
    case PIC18_IRQ_INT0:     break; /* always high, no bit to set. */
    case PIC18_IRQ_INT1:
        if (high) SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT1IP);
        break;
    case PIC18_IRQ_INT2:
        if (high) SFR_SET_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON3, PIC_INTCON3_INT2IP);
        break;
    case PIC18_IRQ_INT3:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_INT3IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_INT3IP);
        break;
    case PIC18_IRQ_RB:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_RBIP);
        break;
    case PIC18_IRQ_TMR0:
        if (high) SFR_SET_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        else      SFR_CLR_BIT(PIC_REG_INTCON2, PIC_INTCON2_TMR0IP);
        break;
    case PIC18_IRQ_TMR1:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TMR1IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TMR1IP);
        break;
    case PIC18_IRQ_TMR2:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TMR2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TMR2IP);
        break;
    case PIC18_IRQ_TMR3:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_TMR3IP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_TMR3IP);
        break;
    case PIC18_IRQ_TMR4:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_TMR4IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_TMR4IP);
        break;
    case PIC18_IRQ_CCP1:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_CCP1IP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_CCP1IP);
        break;
    case PIC18_IRQ_CCP2:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_CCP2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_CCP2IP);
        break;
    case PIC18_IRQ_CCP3:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_CCP3IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_CCP3IP);
        break;
    case PIC18_IRQ_CCP4:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_CCP4IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_CCP4IP);
        break;
    case PIC18_IRQ_CCP5:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_CCP5IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_CCP5IP);
        break;
    case PIC18_IRQ_SSP:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_SSPIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_SSPIP);
        break;
    case PIC18_IRQ_USART1_TX:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_TXIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_TXIP);
        break;
    case PIC18_IRQ_USART1_RX:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_RCIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_RCIP);
        break;
    case PIC18_IRQ_USART2_TX:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_TX2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_TX2IP);
        break;
    case PIC18_IRQ_USART2_RX:
        if (high) SFR_SET_BIT(PIC_REG_IPR3, PIC_IPR3_RC2IP);
        else      SFR_CLR_BIT(PIC_REG_IPR3, PIC_IPR3_RC2IP);
        break;
    case PIC18_IRQ_ADC:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_ADIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_ADIP);
        break;
    case PIC18_IRQ_CMP:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_CMIP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_CMIP);
        break;
    case PIC18_IRQ_EEPROM:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_EEIP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_EEIP);
        break;
    case PIC18_IRQ_LVD:
        if (high) SFR_SET_BIT(PIC_REG_IPR2, PIC_IPR2_LVDIP);
        else      SFR_CLR_BIT(PIC_REG_IPR2, PIC_IPR2_LVDIP);
        break;
    case PIC18_IRQ_PSP:
        if (high) SFR_SET_BIT(PIC_REG_IPR1, PIC_IPR1_PSPIP);
        else      SFR_CLR_BIT(PIC_REG_IPR1, PIC_IPR1_PSPIP);
        break;
    default: break;
    }
}
