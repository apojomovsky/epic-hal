/* Stub bodies of the IRQ contract for the PIC16F5x core (DS41213D
 * section 4.0: no interrupt vector, no INTCON, no interrupt source).
 * Every function is a no-op, the honest model for a core with no IRQ
 * hardware, kept so family-agnostic consumers compile and link
 * unchanged. The contracts live in core/pic16_irq.h. */

#include "core/pic16_irq.h"

/**
 * @brief  Mask all interrupts.
 * @return the prior interrupt state (0 on a core with none).
 */
uint8_t EPIC_IRQ_Disable(void)
{
    return 0U;
}

/**
 * @brief  Restore the interrupt state from a prior Disable.
 * @param prev_state the state to restore (ignored, no IRQs exist).
 */
void EPIC_IRQ_Restore(uint8_t prev_state)
{
    (void)prev_state;
}

/**
 * @brief  Enable one interrupt source.
 * @param irq the source to enable (no source exists on this core).
 */
void EPIC_IRQ_Enable(PIC16_IRQn irq)
{
    (void)irq;
}

/**
 * @brief  Disable one interrupt source.
 * @param irq the source to disable (no source exists on this core).
 */
void EPIC_IRQ_DisableSrc(PIC16_IRQn irq)
{
    (void)irq;
}

/**
 * @brief  Clear an interrupt flag.
 * @param irq the flag to clear (no flags exist on this core).
 */
void EPIC_IRQ_ClearFlag(PIC16_IRQn irq)
{
    (void)irq;
}

/**
 * @brief  Read an interrupt flag.
 * @param irq the flag to read (no flags exist on this core).
 * @return 0, always.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16_IRQn irq)
{
    (void)irq;
    return 0U;
}

/**
 * @brief  Set the priority of an interrupt source.
 * @param irq the source (none exist on this core).
 * @param prio the priority to set (ignored).
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio)
{
    (void)irq;
    (void)prio;
}
