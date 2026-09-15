/*
 * Fan-out from both vectors to this family's IRQHandlers, shared by
 * both builds. Reads INTCON once, calls only handlers whose bit is
 * set. Prototypes are strong externs (not EPIC_WEAK) so the host
 * linker keeps every handler object. Sources beyond Timer0 join as
 * their drivers land.
 */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow interrupt handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change interrupt handler. */
extern void RB_IRQHandler(void);

/**
 * @brief  Fan out from the PIC18 interrupt vectors to every peripheral
 *         IRQHandler whose flag is set. Reads INTCON once into a local
 *         and only calls a handler whose bit is set.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();
}
