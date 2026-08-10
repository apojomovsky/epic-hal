/**
 * @file    pic18_irq_dispatch.c
 * @brief   Fan-out from the PIC18 interrupt vectors to every peripheral
 *          IRQHandler. Shared by both builds.
 *
 * @details
 *   Both PIC18 interrupt vectors (0008h high, 0018h low, DS39632E §9.0)
 *   call this on target; the host harness registers it as the sim IRQ
 *   callback. Each peripheral IRQHandler still checks (and clears) its
 *   own flag internally and is safe to call from anywhere else, but
 *   this dispatcher only *calls* a handler when its bit is already
 *   known to be set: it reads INTCON/PIR1/PIR2 once each into locals
 *   and branches directly on those bits, instead of unconditionally
 *   invoking every handler and letting each one pay its own
 *   table-driven `EPIC_IRQ_GetFlag` lookup (`pic18_irq.c`) to find out
 *   it wasn't the one that fired. Same shape as the PIC16F87XA fix
 *   (`pic16_irq_dispatch.c`), adapted to this family's own register
 *   layout. Prototypes are declared here as strong externs, not via
 *   the peripheral headers' `EPIC_WEAK` declaration, which would let
 *   the host linker drop the handler's object from the static
 *   library.
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
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            /* Source disabled: drop the stale flag with the same
             * single-instruction PIR1 bit clear the CCP handlers use
             * (EPIC_BIT_CLR on PIC_REG_PIR1), not the table-driven
             * EPIC_IRQ_ClearFlag, whose lookup would itself delay the
             * swuart RX re-arm on this event. */
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
    if (pir1 & PIC_PIR1_SSPIF)  SSP_IRQHandler();
    /* TX is gated on TXIE, not just TXIF: TXIF is a read-only status
     * bit that stays set whenever TXREG is empty, so an un-gated
     * branch fires USART_TX_IRQHandler (and its callback) on every
     * ISR from any source. Same hazard and same fix shape as
     * PIC16F87XA (pic16_irq_dispatch.c). */
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
    /* EEIF is gated on EEIE and left untouched when the source is
     * disabled: EEPROM completion is often polled (epic-settings spins
     * on EEIF with EEIE off), and an unconditional dispatch would
     * clear the flag from a live ISR and hang the poller (found by the
     * combination-matrix C7 gate, 2026-08-09). No stale-flag drop:
     * the polling consumer owns EEIF. */
    if (pir2 & PIC_PIR2_EEIF) {
        if (epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
}
