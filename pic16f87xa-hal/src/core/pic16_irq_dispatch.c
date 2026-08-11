/* Fan-out from the single PIC16 interrupt vector (0x0004, DS39582B
 * §14.11) to every peripheral IRQHandler, shared by both builds. Reads
 * INTCON/PIR1/PIR2 once into locals and branches on the bits instead of
 * unconditionally invoking every handler (measured 409 of 674 ISR
 * cycles under MPLAB SIM before this fix). Strong extern prototypes,
 * not the EPIC_WEAK headers, force the host linker to pull every
 * handler out of the static library. */

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

/* The dispatcher runs in the ISR. XC8 emits no PCLATH setup for the
 * handler calls (it assumes the interrupt call-graph is linked into
 * one flash page), so the dispatch and handlers must share a page; a
 * page-crossing call lands 0x800 past its target. Pin the dispatch to
 * page 1 with the handlers (host build has no pages, no pin). */
#if defined(__XC8)
void epic_dispatch_all_irqs(void) __at(0x900)
#else
void epic_dispatch_all_irqs(void)
#endif
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();

    uint8_t pir1 = EPIC_REG8(PIC_REG_PIR1);
    /* TMR1 is gated on TMR1IE, not just TMR1IF: Timer1 free-runs with
     * its overflow interrupt disabled (epic-swuart needs the counter,
     * never the overflow), so TMR1IF latches at every 65536-cycle wrap.
     * Without the gate every later event pays the full handler cost
     * before its own dispatch; when the source is disabled the stale
     * flag is dropped so it does not re-trigger this branch. */
    if (pir1 & PIC_PIR1_TMR1IF) {
        uint8_t tmr1ie;
        EPIC_PIE1_READ_TMR1IE(tmr1ie);
        if (tmr1ie & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            /* Source disabled: drop the stale flag with the same
             * single-instruction PIR1 bit clear the CCP handlers use
             * (atomic ANDWF), not the table-driven lookup. */
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
    if (pir1 & PIC_PIR1_SSPIF)  SSP_IRQHandler();
    if (pir1 & PIC_PIR1_RCIF)   USART_RX_IRQHandler();
    /* TXIF is a read-only status bit that stays set whenever TXREG is
     * empty, so dispatch the TX handler only when the source is
     * enabled. Without the gate every ISR calls the handler's callback
     * through XC8's PC-relative function-pointer table, which requires
     * the callback to share the table's flash page; a scattered jump
     * lands in garbage and wedges interrupt delivery. */
    if (pir1 & PIC_PIR1_TXIF) {
        uint8_t txie;
        EPIC_PIE1_READ_TXIE(txie);
        if (txie & PIC_PIE1_TXIE) {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_ADIF)   ADC_IRQHandler();
#if PIC16F87XA_FAMILY_HAS_PSP
    if (pir1 & PIC_PIR1_PSPIF)  PSP_IRQHandler();
#endif

    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    /* EEIF is gated on EEIE and left untouched when the source is
     * disabled: EEPROM completion is often polled with EEIE off, so
     * clearing the flag from a live ISR would steal the completion
     * signal and hang the poller. Unlike the TMR1 stale flag, there is
     * no stale-flag drop here: the polling consumer owns EEIF. */
    if (pir2 & PIC_PIR2_EEIF) {
        uint8_t eeie = 0u;
        EPIC_PIE2_READ_EEIE(eeie);
        if (eeie & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
    if (pir2 & PIC_PIR2_CMIF)   COMP_IRQHandler();
}
