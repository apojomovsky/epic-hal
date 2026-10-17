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
 * @details The source is enabled in PIE0 and also sets INTCON.PEIE,
 * because PIR0 sources reach the vector only through PEIE. GIE is left
 * to EPIC_IRQ_Restore.
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
}

/**
 * @brief Clear the interrupt request flag for a source.
 * @details Constant mask so the clear compiles to a single bit-clear on the
 * flag register, not a read-modify-write with a runtime mask.
 * @param irq Interrupt source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16F188XX_IRQn irq)
{
    switch (irq)
    {
    case PIC16F188XX_IRQ_IOC:  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR0), PIC_PIR0_IOCIF); break;
    case PIC16F188XX_IRQ_TMR0: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR0), PIC_PIR0_TMR0IF); break;
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
