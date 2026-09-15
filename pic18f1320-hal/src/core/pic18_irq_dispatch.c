/*
 * Fan-out from the PIC18 interrupt vectors to every peripheral
 * IRQHandler with a linked driver (both vectors call this on target;
 * the host harness registers it as the sim IRQ callback). Foundation
 * phase: only Timer0 and RB<7:4> change have a driver; the rest of
 * `core/pic18_irq.h`'s IRQn enum grows this fan-out per ticket.
 */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow interrupt handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change interrupt handler. */
extern void RB_IRQHandler(void);

/**
 * @brief  Fan out from the PIC18 interrupt vectors to every peripheral
 *         IRQHandler whose flag is set and whose driver is linked in.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();
}
