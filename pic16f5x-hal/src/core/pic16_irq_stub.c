/* Stub bodies of the IRQ contract for the PIC16F5x core (DS41213D
 * §4.0: no interrupt vector, no INTCON, no interrupt source). Every
 * function is a no-op, the honest model for a core with no IRQ
 * hardware, kept so family-agnostic consumers compile and link
 * unchanged. */

#include "core/pic16_irq.h"

uint8_t EPIC_IRQ_Disable(void)
{
    return 0U;
}

void EPIC_IRQ_Restore(uint8_t prev_state)
{
    (void)prev_state;
}

void EPIC_IRQ_Enable(PIC16_IRQn irq)
{
    (void)irq;
}

void EPIC_IRQ_DisableSrc(PIC16_IRQn irq)
{
    (void)irq;
}

void EPIC_IRQ_ClearFlag(PIC16_IRQn irq)
{
    (void)irq;
}

uint8_t EPIC_IRQ_GetFlag(PIC16_IRQn irq)
{
    (void)irq;
    return 0U;
}

void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio)
{
    (void)irq;
    (void)prio;
}
