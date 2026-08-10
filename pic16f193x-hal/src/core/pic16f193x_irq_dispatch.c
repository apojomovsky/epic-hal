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
 *   with strong prototypes, not via their EPIC_WEAK headers, so the
 *   host linker is forced to pull every handler's object out of the
 *   static library instead of leaving an unreferenced weak symbol NULL.
 *
 *   Foundation + Timer1 + Timer2/4/6: TIMER0, TIMER1, TIMER2, TIMER4,
 *   TIMER6, and IOC (the GPIO change interrupt) have drivers. Each
 *   peripheral phase appends its handler extern and its call here, in
 *   the same shape.
 *
 *   Each peripheral IRQHandler still checks (and clears) its own flag
 *   internally and is safe to call from anywhere else, but this
 *   dispatcher only *calls* a handler when its bit is already known
 *   to be set: it reads INTCON/PIR1/PIR2/PIR3 once each into locals
 *   and branches directly on those bits, instead of unconditionally
 *   invoking every handler and letting each one pay its own
 *   table-driven `EPIC_IRQ_GetFlag` lookup (`pic16f193x_irq.c`) to
 *   find out it wasn't the one that fired. Same shape as the
 *   PIC16F87XA fix (`pic16_irq_dispatch.c`), adapted to this family's
 *   own three-PIR-bank register layout.
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
extern void SSP_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void CMP1_IRQHandler(void);
extern void CMP2_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void CCP3_IRQHandler(void);
extern void CCP4_IRQHandler(void);
extern void CCP5_IRQHandler(void);
extern void LCD_IRQHandler(void);
extern void IOC_IRQHandler(void);

void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_IOCIF)  IOC_IRQHandler();

    uint8_t pir1 = EPIC_REG8(PIC_REG_PIR1);
    /* TMR1 is flag-gated on TMR1IE, not just TMR1IF: Timer1 free-runs
     * with its overflow interrupt disabled (epic-swuart needs the
     * counter but never the overflow), so TMR1IF latches at every
     * 65536-cycle wrap and stays set. Without this check the next CCP
     * event would pay TIMER1_IRQHandler's full table-driven cost
     * (~250 cycles) before its own dispatch, blowing the swuart RX
     * re-arm margin (same hazard as PIC16F87XA, see
     * docs/superpowers/plans/probe-swuart-rx-hotpath.md). When the
     * source is disabled the stale flag is dropped so it does not
     * re-trigger this branch on every later event. */
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
    /* TX is gated on TXIE, not just TXIF: TXIF is a read-only status
     * bit that stays set whenever TXREG is empty, so an un-gated
     * branch fires USART_TX_IRQHandler on every ISR from any source.
     * Same hazard and same fix shape as PIC16F87XA (the handler is an
     * empty stub today, but the every-ISR call is already wasted and
     * any real handler inherits the defect). */
    if (pir1 & PIC_PIR1_TXIF) {
        uint8_t txie;
        EPIC_PIE1_READ_TXIE(txie);
        if (txie & PIC_PIE1_TXIE) {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_RCIF)   USART_RX_IRQHandler();
    if (pir1 & PIC_PIR1_ADIF)   ADC_IRQHandler();

    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    if (pir2 & PIC_PIR2_LCDIF)  LCD_IRQHandler();
    /* EEIF is gated on EEIE and left untouched when the source is
     * disabled: EEPROM completion is often polled (epic-settings spins
     * on EEIF with EEIE off), and an unconditional dispatch would
     * clear the flag from a live ISR and hang the poller (found by the
     * combination-matrix C7 gate, 2026-08-09). No stale-flag drop:
     * the polling consumer owns EEIF. */
    if (pir2 & PIC_PIR2_EEIF) {
        uint8_t eeie = 0u;
        EPIC_PIE2_READ_EEIE(eeie);
        if (eeie & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
    if (pir2 & PIC_PIR2_C1IF)   CMP1_IRQHandler();
    if (pir2 & PIC_PIR2_C2IF)   CMP2_IRQHandler();

    uint8_t pir3 = EPIC_REG8(PIC_REG_PIR3);
    if (pir3 & PIC_PIR3_TMR4IF) TIMER4_IRQHandler();
    if (pir3 & PIC_PIR3_TMR6IF) TIMER6_IRQHandler();
    if (pir3 & PIC_PIR3_CCP3IF) CCP3_IRQHandler();
    if (pir3 & PIC_PIR3_CCP4IF) CCP4_IRQHandler();
    if (pir3 & PIC_PIR3_CCP5IF) CCP5_IRQHandler();
}
