#include "core/pic16f1508_irq.h"

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
 * @param irq Interrupt source. Only PIC16F1508_IRQ_IOC is implemented.
 */
void EPIC_IRQ_Enable(PIC16F1508_IRQn irq)
{
    if (irq == PIC16F1508_IRQ_IOC)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIE);
    }
}

/**
 * @brief Disable a single interrupt source.
 * @param irq Interrupt source. Only PIC16F1508_IRQ_IOC is implemented.
 */
void EPIC_IRQ_DisableSrc(PIC16F1508_IRQn irq)
{
    if (irq == PIC16F1508_IRQ_IOC)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIE);
    }
}

/**
 * @brief Clear the interrupt request flag for a source.
 * @param irq Interrupt source. Only PIC16F1508_IRQ_IOC is implemented.
 */
void EPIC_IRQ_ClearFlag(PIC16F1508_IRQn irq)
{
    if (irq == PIC16F1508_IRQ_IOC)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIF);
    }
}

/**
 * @brief Read the interrupt request flag for a source.
 * @param irq Interrupt source. Only PIC16F1508_IRQ_IOC is implemented.
 * @return 1 when the source flag is set, 0 otherwise or for an unknown source.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16F1508_IRQn irq)
{
    if (irq == PIC16F1508_IRQ_IOC)
    {
        return (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIF) ? 1U : 0U;
    }
    return 0U;
}

/**
 * @brief No-op: this family has one interrupt vector and no priority levels.
 * @param irq Interrupt source, ignored.
 * @param prio Requested priority, ignored.
 */
void EPIC_IRQ_SetPriority(PIC16F1508_IRQn irq, EPIC_IRQ_Priority prio)
{
    (void)irq;
    (void)prio;
}
