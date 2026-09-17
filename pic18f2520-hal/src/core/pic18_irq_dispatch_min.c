/*
 * Fan-out from both vectors, minimal baseline: Timer0 + RB change only,
 * for variants whose hal_sources baseline links just GPIO + Timer0 (the
 * blink/irq-smoke examples need nothing else). Timer1-3/CCP/SSP/EUSART/
 * ADC/COMP/EEPROM handlers are not referenced here, so their drivers
 * stay unlinked (epic-hal#155: the full pic18_irq_dispatch.c's strong
 * externs force every handler object into the link, which a 4 KB-flash
 * part cannot afford just to build blink). Prototypes are strong
 * externs, matching pic18_irq_dispatch.c's own rationale.
 */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow interrupt handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change interrupt handler. */
extern void RB_IRQHandler(void);

/**
 * @brief  Fan out from the PIC18 interrupt vectors to the Timer0/RB
 *         IRQHandlers whose flag is set. Reads INTCON once into a local
 *         and only calls a handler whose bit is set.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();
}
