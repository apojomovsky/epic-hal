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

/* The dispatcher runs in the ISR. XC8 emits no PCLATH setup for the
 * handler calls below (it assumes the interrupt call-graph is linked
 * into one flash page), so the dispatch and the handlers must share a
 * page. Best-fit placement scatters them (the dispatch has linked into
 * page 0 while the handlers sit in page 1, and each call then lands
 * 0x800 past its target, executing garbage); pin the dispatch into
 * page 1 with the handlers, which are already co-located there (this
 * build's layout was verified page-sensitive: the identical dispatch
 * logic passes or fails the sim gates purely on the linker's
 * placement). Host build: no pages, no pin. */
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
    /* TXIF is a read-only status bit that stays set whenever TXREG is
     * empty, so dispatch the TX handler only when the source is
     * actually enabled (same flag-gating shape as the TMR1 branch
     * above). Without the TXIE gate every ISR calls the TX handler and
     * its callback, which goes through XC8's PC-relative
     * function-pointer table: the callback must share the table's
     * flash page, and when the linker scatters it elsewhere the jump
     * lands in garbage, corrupting the ISR and wedging interrupt
     * delivery (epic-tick's sim-target gate froze with exactly this
     * signature once s_tx_cplt landed in a different page than the
     * handlers). */
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
    /* EEIF is gated on EEIE, not dispatched unconditionally, and the
     * flag is left untouched when the source is disabled: EEPROM
     * completion is often POLLED (epic-settings spins on EEIF with
     * EEIE off), so clearing it from a live ISR would steal the
     * completion signal and hang the poller (found by the
     * combination-matrix C7 gate, 2026-08-09). Unlike the TMR1 stale
     * flag, there is no stale-flag drop here: the polling consumer
     * owns EEIF. */
    if (pir2 & PIC_PIR2_EEIF) {
        uint8_t pie2 = 0u;
        EPIC_BANK1_READ8(PIE2, pie2);
        if (pie2 & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
    if (pir2 & PIC_PIR2_CMIF)   COMP_IRQHandler();
}
