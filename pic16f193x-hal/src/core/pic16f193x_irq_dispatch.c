/**
 * @file    pic16f193x_irq_dispatch.c
 * @brief   Fan-out from the single PIC16F193X interrupt vector to every
 *          peripheral IRQHandler. Shared by both builds.
 *
 * @details
 *   Single PIC16F193X vector (0x0004, DS41364B §4.0): the target's
 *   `__interrupt()` and the host's sim IRQ callback both call this one
 *   dispatcher. Each peripheral IRQHandler checks its own flag and
 *   returns immediately if not pending. Handlers are declared here
 *   with strong prototypes, not via their PIC8_WEAK headers, so the
 *   host linker is forced to pull every handler's object out of the
 *   static library instead of leaving an unreferenced weak symbol NULL.
 *
 *   Foundation + Timer1 + Timer2/4/6: TIMER0, TIMER1, TIMER2, TIMER4,
 *   TIMER6, and IOC (the GPIO change interrupt) have drivers. Each
 *   peripheral phase appends its handler extern and its call here, in
 *   the same shape.
 */

#include "core/pic16f193x_irq.h"

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void TIMER4_IRQHandler(void);
extern void TIMER6_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void IOC_IRQHandler(void);

void pic8_dispatch_all_irqs(void)
{
    TIMER0_IRQHandler();
    TIMER1_IRQHandler();
    TIMER2_IRQHandler();
    TIMER4_IRQHandler();
    TIMER6_IRQHandler();
    CCP1_IRQHandler();
    CCP2_IRQHandler();
    USART_TX_IRQHandler();
    USART_RX_IRQHandler();
    IOC_IRQHandler();
}
