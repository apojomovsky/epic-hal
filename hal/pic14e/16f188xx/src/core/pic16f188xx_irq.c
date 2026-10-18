#include "core/pic16f188xx_irq.h"

/**
 * @brief Clear the global interrupt enable (INTCON.GIE) and report its prior state.
 * @return 1 if GIE was set before the call, 0 otherwise.
 */
uint8_t EPIC_IRQ_Disable(void)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE) ? 1U : 0U;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
    return prev;
}

/**
 * @brief Restore the global interrupt enable saved by EPIC_IRQ_Disable.
 * @param prev_state Value returned by EPIC_IRQ_Disable. Nonzero re-enables GIE.
 */
void EPIC_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state != 0U)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
    }
    else
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
    }
}

/**
 * @brief Enable a single interrupt source.
 * @details The source is enabled in PIE0, PIE3 or PIE7 and also sets
 * INTCON.PEIE, because peripheral sources reach the vector only through
 * PEIE. GIE is left to EPIC_IRQ_Restore.
 * @param irq Interrupt source to enable.
 */
void EPIC_IRQ_Enable(PIC16F188XX_IRQn irq)
{
    if (irq == PIC16F188XX_IRQ_IOC)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE0), PIC_PIE0_IOCIE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR0)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE0), PIC_PIE0_TMR0IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_SSP1)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_SSP1IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_USART_TX)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_TXIE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_USART_RX)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_RCIE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR1)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR1IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR2)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR2IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR4)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR4IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_EEPROM)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE7), PIC_PIE7_NVMIE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_NCO1)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE7), PIC_PIE7_NCO1IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP1)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP1IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP2)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP2IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP3)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP3IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP4)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP4IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP5)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP5IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_ADC)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE1), PIC_PIE1_ADIE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CMP1)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE2), PIC_PIE2_C1IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (irq == PIC16F188XX_IRQ_CMP2)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE2), PIC_PIE2_C2IE);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
}

/**
 * @brief Disable a single interrupt source.
 * @details Clears the source's enable bit. PEIE is left as set, since other
 * peripheral sources may still rely on it.
 * @param irq Interrupt source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16F188XX_IRQn irq)
{
    if (irq == PIC16F188XX_IRQ_IOC)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE0), PIC_PIE0_IOCIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR0)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE0), PIC_PIE0_TMR0IE);
    }
    else if (irq == PIC16F188XX_IRQ_SSP1)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_SSP1IE);
    }
    else if (irq == PIC16F188XX_IRQ_USART_TX)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_TXIE);
    }
    else if (irq == PIC16F188XX_IRQ_USART_RX)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE3), PIC_PIE3_RCIE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR1)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR1IE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR2)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR2IE);
    }
    else if (irq == PIC16F188XX_IRQ_TMR4)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE4), PIC_PIE4_TMR4IE);
    }
    else if (irq == PIC16F188XX_IRQ_EEPROM)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE7), PIC_PIE7_NVMIE);
    }
    else if (irq == PIC16F188XX_IRQ_NCO1)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE7), PIC_PIE7_NCO1IE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP1)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP1IE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP2)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP2IE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP3)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP3IE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP4)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP4IE);
    }
    else if (irq == PIC16F188XX_IRQ_CCP5)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE6), PIC_PIE6_CCP5IE);
    }
    else if (irq == PIC16F188XX_IRQ_ADC)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE1), PIC_PIE1_ADIE);
    }
    else if (irq == PIC16F188XX_IRQ_CMP1)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE2), PIC_PIE2_C1IE);
    }
    else if (irq == PIC16F188XX_IRQ_CMP2)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE2), PIC_PIE2_C2IE);
    }
}

/**
 * @brief Clear the interrupt request flag for a source.
 * @details Constant mask so the clear compiles to a single bit-clear on the
 * flag register, not a read-modify-write with a runtime mask. TXIF and RCIF
 * are read-only status that hardware clears, so those two sources do nothing.
 * @param irq Interrupt source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16F188XX_IRQn irq)
{
    switch (irq)
    {
    case PIC16F188XX_IRQ_IOC:  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR0), PIC_PIR0_IOCIF); break;
    case PIC16F188XX_IRQ_TMR0: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR0), PIC_PIR0_TMR0IF); break;
    case PIC16F188XX_IRQ_SSP1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_SSP1IF); break;
    case PIC16F188XX_IRQ_TMR1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR4), PIC_PIR4_TMR1IF); break;
    case PIC16F188XX_IRQ_TMR2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR4), PIC_PIR4_TMR2IF); break;
    case PIC16F188XX_IRQ_TMR4: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR4), PIC_PIR4_TMR4IF); break;
    case PIC16F188XX_IRQ_EEPROM: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR7), PIC_PIR7_NVMIF); break;
    case PIC16F188XX_IRQ_NCO1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR7), PIC_PIR7_NCO1IF); break;
    case PIC16F188XX_IRQ_CCP1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP1IF); break;
    case PIC16F188XX_IRQ_CCP2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP2IF); break;
    case PIC16F188XX_IRQ_CCP3: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP3IF); break;
    case PIC16F188XX_IRQ_CCP4: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP4IF); break;
    case PIC16F188XX_IRQ_CCP5: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR6), PIC_PIR6_CCP5IF); break;
    case PIC16F188XX_IRQ_ADC: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_ADIF); break;
    case PIC16F188XX_IRQ_CMP1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C1IF); break;
    case PIC16F188XX_IRQ_CMP2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C2IF); break;
    default:                   break;
    }
}

/**
 * @brief Read the interrupt request flag for a source.
 * @param irq Interrupt source whose flag to read.
 * @return 1 when the source flag is set, 0 otherwise or for an unknown source.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16F188XX_IRQn irq)
{
    if (irq == PIC16F188XX_IRQ_IOC)
    {
        return (EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_IOCIF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_TMR0)
    {
        return (EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_SSP1)
    {
        return (EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_SSP1IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_USART_TX)
    {
        return (EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_TXIF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_USART_RX)
    {
        return (EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_RCIF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_TMR1)
    {
        return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_TMR2)
    {
        return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_TMR4)
    {
        return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR4IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_EEPROM)
    {
        return (EPIC_REG8(PIC_REG_PIR7) & PIC_PIR7_NVMIF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_NCO1)
    {
        return (EPIC_REG8(PIC_REG_PIR7) & PIC_PIR7_NCO1IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CCP1)
    {
        return (EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CCP2)
    {
        return (EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP2IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CCP3)
    {
        return (EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP3IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CCP4)
    {
        return (EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP4IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CCP5)
    {
        return (EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP5IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_ADC)
    {
        return (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_ADIF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CMP1)
    {
        return (EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_C1IF) ? 1U : 0U;
    }
    if (irq == PIC16F188XX_IRQ_CMP2)
    {
        return (EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_C2IF) ? 1U : 0U;
    }
    return 0U;
}

/**
 * @brief No-op: this family has one interrupt vector and no priority levels.
 * @param irq Interrupt source, ignored.
 * @param prio Requested priority, ignored.
 */
void EPIC_IRQ_SetPriority(PIC16F188XX_IRQn irq, EPIC_IRQ_Priority prio)
{
    (void)irq;
    (void)prio;
}
