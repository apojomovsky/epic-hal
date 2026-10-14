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
 * @brief Map a CLC source to its PIE3 enable bit.
 * @param irq Interrupt source.
 * @return PIE3 mask for a CLC source, 0 for any other source.
 */
static uint8_t clc_enable_mask(PIC16F1508_IRQn irq)
{
    switch (irq)
    {
    case PIC16F1508_IRQ_CLC1: return PIC_PIE3_CLC1IE;
    case PIC16F1508_IRQ_CLC2: return PIC_PIE3_CLC2IE;
    case PIC16F1508_IRQ_CLC3: return PIC_PIE3_CLC3IE;
    case PIC16F1508_IRQ_CLC4: return PIC_PIE3_CLC4IE;
    default:                  return 0U;
    }
}

/**
 * @brief Map a CLC source to its PIR3 flag bit.
 * @param irq Interrupt source.
 * @return PIR3 mask for a CLC source, 0 for any other source.
 */
static uint8_t clc_flag_mask(PIC16F1508_IRQn irq)
{
    switch (irq)
    {
    case PIC16F1508_IRQ_CLC1: return PIC_PIR3_CLC1IF;
    case PIC16F1508_IRQ_CLC2: return PIC_PIR3_CLC2IF;
    case PIC16F1508_IRQ_CLC3: return PIC_PIR3_CLC3IF;
    case PIC16F1508_IRQ_CLC4: return PIC_PIR3_CLC4IF;
    default:                  return 0U;
    }
}

/**
 * @brief Map a timer source to its PIE1 enable bit.
 * @param irq Interrupt source.
 * @return PIE1 mask for a timer source, 0 for any other source.
 */
static uint8_t tmr_enable_mask(PIC16F1508_IRQn irq)
{
    switch (irq)
    {
    case PIC16F1508_IRQ_TMR1: return PIC_PIE1_TMR1IE;
    case PIC16F1508_IRQ_TMR2: return PIC_PIE1_TMR2IE;
    default:                  return 0U;
    }
}

/**
 * @brief Map a timer source to its PIR1 flag bit.
 * @param irq Interrupt source.
 * @return PIR1 mask for a timer source, 0 for any other source.
 */
static uint8_t tmr_flag_mask(PIC16F1508_IRQn irq)
{
    switch (irq)
    {
    case PIC16F1508_IRQ_TMR1: return PIC_PIR1_TMR1IF;
    case PIC16F1508_IRQ_TMR2: return PIC_PIR1_TMR2IF;
    default:                  return 0U;
    }
}

/**
 * @brief Enable a single interrupt source.
 * @details IOC is enabled in INTCON. A CLC source is enabled in PIE3 and a
 * timer source in PIE1; both also set INTCON.PEIE, because PIR1 and PIR3
 * sources reach the vector only through PEIE. GIE is left to EPIC_IRQ_Restore.
 * @param irq Interrupt source to enable.
 */
void EPIC_IRQ_Enable(PIC16F1508_IRQn irq)
{
    uint8_t ie_mask = clc_enable_mask(irq);
    uint8_t tmr_mask = tmr_enable_mask(irq);

    if (irq == PIC16F1508_IRQ_IOC)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIE);
    }
    else if (ie_mask != 0U)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE3), ie_mask);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
    else if (tmr_mask != 0U)
    {
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_PIE1), tmr_mask);
        EPIC_BIT_SET(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
    }
}

/**
 * @brief Disable a single interrupt source.
 * @details Clears the source's enable bit. PEIE is left as set, since other
 * peripheral sources may still rely on it.
 * @param irq Interrupt source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16F1508_IRQn irq)
{
    uint8_t ie_mask = clc_enable_mask(irq);
    uint8_t tmr_mask = tmr_enable_mask(irq);

    if (irq == PIC16F1508_IRQ_IOC)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIE);
    }
    else if (ie_mask != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE3), ie_mask);
    }
    else if (tmr_mask != 0U)
    {
        EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIE1), tmr_mask);
    }
}

/**
 * @brief Clear the interrupt request flag for a source.
 * @details Each case uses a constant mask so the clear compiles to a single
 * bit-clear on the flag register, not a read-modify-write with a runtime mask.
 * @param irq Interrupt source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16F1508_IRQn irq)
{
    switch (irq)
    {
    case PIC16F1508_IRQ_IOC:  EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_IOCIF); break;
    case PIC16F1508_IRQ_CLC1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CLC1IF); break;
    case PIC16F1508_IRQ_CLC2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CLC2IF); break;
    case PIC16F1508_IRQ_CLC3: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CLC3IF); break;
    case PIC16F1508_IRQ_CLC4: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR3), PIC_PIR3_CLC4IF); break;
    case PIC16F1508_IRQ_TMR1: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF); break;
    case PIC16F1508_IRQ_TMR2: EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR2IF); break;
    default:                  break;
    }
}

/**
 * @brief Read the interrupt request flag for a source.
 * @param irq Interrupt source whose flag to read.
 * @return 1 when the source flag is set, 0 otherwise or for an unknown source.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16F1508_IRQn irq)
{
    uint8_t flag_mask = clc_flag_mask(irq);
    uint8_t tmr_mask = tmr_flag_mask(irq);

    if (irq == PIC16F1508_IRQ_IOC)
    {
        return (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIF) ? 1U : 0U;
    }
    if (flag_mask != 0U)
    {
        return (EPIC_REG8(PIC_REG_PIR3) & flag_mask) ? 1U : 0U;
    }
    if (tmr_mask != 0U)
    {
        return (EPIC_REG8(PIC_REG_PIR1) & tmr_mask) ? 1U : 0U;
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
