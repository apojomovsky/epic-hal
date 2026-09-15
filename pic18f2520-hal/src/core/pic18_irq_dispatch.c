/*
 * Fan-out from both vectors to this family's IRQHandlers, shared by
 * both builds. Reads INTCON/PIRx once, calls only handlers whose bit
 * is set. Prototypes are strong externs (not EPIC_WEAK) so the host
 * linker keeps every handler object. Timer0-3 all dispatch from here;
 * later phases add PIR1/PIR2 peripheral sources.
 */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow interrupt handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change interrupt handler. */
extern void RB_IRQHandler(void);
/** @brief Timer1 overflow interrupt handler. */
extern void TIMER1_IRQHandler(void);
/** @brief Timer2 match interrupt handler. */
extern void TIMER2_IRQHandler(void);
/** @brief Timer3 overflow interrupt handler. */
extern void TIMER3_IRQHandler(void);

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
    uint8_t pir1 = epic_sfr_read8(PIC_REG_PIR1);
    if (pir1 & PIC_PIR1_TMR1IF) {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_TMR3IF) TIMER3_IRQHandler();
}
