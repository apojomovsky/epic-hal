/*
 * Fan-out from the PIC18 interrupt vectors to the peripheral IRQHandlers
 * that exist in this family tree, shared by both builds (both vectors call
 * this on target; the host harness registers it as the sim IRQ callback).
 * Reads INTCON once into a local and only calls a handler whose bit is
 * set. Prototypes are strong externs here (not the headers' EPIC_WEAK),
 * so the host linker cannot drop a handler's object from the static
 * library. Foundation covers only the sources that exist this phase;
 * Timer1-3, CCP, MSSP, EUSART, ADC, comparator and EEPROM handlers join
 * as their drivers land.
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
