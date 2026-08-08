/**
 * @file    pic16_irq_dispatch.c
 * @brief   Fan-out from the single PIC16 interrupt vector to every
 *          peripheral IRQHandler. Shared by both builds.
 *
 * @details
 *   Single PIC16 vector (0x0004, DS39582B §14.11): the target's
 *   `__interrupt()` and the host's sim IRQ callback both call this one
 *   dispatcher. Each peripheral IRQHandler still checks (and clears)
 *   its own flag internally and is safe to call from anywhere else,
 *   but this dispatcher only *calls* a handler when its bit is already
 *   known to be set: it reads INTCON/PIR1/PIR2 once each into locals
 *   and branches directly on those bits, instead of unconditionally
 *   invoking every handler and letting each one pay its own
 *   table-driven `EPIC_IRQ_GetFlag` lookup (`pic16_irq.c`) to find out
 *   it wasn't the one that fired. Measured on real hardware under
 *   MPLAB SIM: 409 of 674 ISR cycles on PIC16F87XA were spent on the
 *   old unconditional fan-out before this fix, see git commit
 *   b679e21's message for the full measurement.
 *   Handlers are declared here with strong prototypes, not via their
 *   EPIC_WEAK headers, so the host linker is forced to pull every
 *   handler's object out of the static library instead of leaving an
 *   unreferenced weak symbol NULL.
 */

#include "core/pic16_irq.h"

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void COMP_IRQHandler(void);
extern void RB_IRQHandler(void);
#if PIC16F87XA_FAMILY_HAS_PSP
extern void PSP_IRQHandler(void);
#endif

void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();

    uint8_t pir1 = EPIC_REG8(PIC_REG_PIR1);
    /* TMR1 is flag-gated on TMR1IE, not just TMR1IF: Timer1 free-runs
     * with its overflow interrupt disabled (epic-swuart needs the
     * counter but never the overflow), so TMR1IF latches at every
     * 65536-cycle wrap and stays set. Without this check the next CCP
     * event would pay TIMER1_IRQHandler's full table-driven cost
     * (~250 cycles) before its own dispatch, blowing the swuart RX
     * re-arm margin (see docs/superpowers/plans/probe-swuart-rx-hotpath.md).
     * When the source is disabled the stale flag is dropped so it does
     * not re-trigger this branch on every later event. */
    if (pir1 & PIC_PIR1_TMR1IF) {
        uint8_t tmr1ie;
        EPIC_PIE1_READ_TMR1IE(tmr1ie);
        if (tmr1ie & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            /* Source disabled: drop the stale flag with the same
             * single-instruction PIR1 bit clear the CCP handlers use
             * (EPIC_BIT_CLR on PIC_REG_PIR1, atomic ANDWF), not the
             * table-driven EPIC_IRQ_ClearFlag, whose lookup would
             * itself delay the swuart RX re-arm on this event. */
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
    if (pir1 & PIC_PIR1_SSPIF)  SSP_IRQHandler();
    if (pir1 & PIC_PIR1_RCIF)   USART_RX_IRQHandler();
    if (pir1 & PIC_PIR1_TXIF)   USART_TX_IRQHandler();
    if (pir1 & PIC_PIR1_ADIF)   ADC_IRQHandler();
#if PIC16F87XA_FAMILY_HAS_PSP
    if (pir1 & PIC_PIR1_PSPIF)  PSP_IRQHandler();
#endif

    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    if (pir2 & PIC_PIR2_EEIF)   EEPROM_IRQHandler();
    if (pir2 & PIC_PIR2_CMIF)   COMP_IRQHandler();
}
