/*
 * Fan-out from the PIC18 interrupt vectors to every peripheral IRQHandler,
 * shared by both builds (both vectors call this on target; the host
 * harness registers it as the sim IRQ callback). Reads INTCON/PIR1/PIR2
 * once into locals and only calls a handler whose bit is set, so each
 * handler does not pay its own table-driven `EPIC_IRQ_GetFlag` lookup.
 * Prototypes are strong externs here (not the headers' EPIC_WEAK), so the
 * host linker cannot drop a handler's object from the static library.
 */

#include "core/pic18_irq.h"

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void TIMER3_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void COMP_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void RB_IRQHandler(void);
#if PIC18FXX5X_FAMILY_HAS_SPP
extern void SPP_IRQHandler(void);
#endif

void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = epic_sfr_read8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF)   RB_IRQHandler();

    uint8_t pir1 = epic_sfr_read8(PIC_REG_PIR1);
    /* Gate TMR1 on TMR1IE, not TMR1IF: Timer1 free-runs with its
     * overflow IRQ disabled (epic-swuart needs the counter but never
     * the overflow), so TMR1IF latches at every 65536-cycle wrap and
     * stays set. Dispatching on the stale flag would pay the full
     * table-driven handler cost on unrelated events; when the source
     * is disabled the stale flag is dropped so it does not re-trigger
     * this branch on every later event. */
    if (pir1 & PIC_PIR1_TMR1IF) {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            /* Source disabled: drop the stale flag via the direct
             * EPIC_BIT_CLR the CCP handlers use, not the table-driven
             * EPIC_IRQ_ClearFlag, whose lookup would delay the swuart
             * RX re-arm on this event. */
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
    if (pir1 & PIC_PIR1_SSPIF)  SSP_IRQHandler();
    /* Gate TX on TXIE, not TXIF: TXIF is a read-only status bit that
     * stays set whenever TXREG is empty, so an un-gated branch fires
     * USART_TX_IRQHandler (and its callback) on every ISR from any
     * source. */
    if (pir1 & PIC_PIR1_TXIF) {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TXIE) {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_RCIF)   USART_RX_IRQHandler();
    if (pir1 & PIC_PIR1_ADIF)   ADC_IRQHandler();
#if PIC18FXX5X_FAMILY_HAS_SPP
    if (pir1 & PIC_PIR1_SPPIF)  SPP_IRQHandler();
#endif

    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_TMR3IF) TIMER3_IRQHandler();
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    if (pir2 & PIC_PIR2_CMIF)   COMP_IRQHandler();
    /* Gate EEIF on EEIE and leave it untouched when the source is
     * disabled: EEPROM completion is often polled (epic-settings spins
     * on EEIF with EEIE off), and an unconditional dispatch would clear
     * the flag from a live ISR and hang the poller. No stale-flag drop:
     * the polling consumer owns EEIF. */
    if (pir2 & PIC_PIR2_EEIF) {
        if (epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
}
