/* Shared PIC14 mid-range fan-out from the single interrupt vector
 * (0x0004, DS39582B §14.11, DS40001291H §14.11) to every peripheral
 * IRQHandler, both builds. Reads INTCON/PIR1/PIR2 once into locals and
 * branches on the bits instead of unconditionally invoking every
 * handler. Strong extern prototypes, not the EPIC_WEAK headers, force
 * the host linker to pull every handler out of the static library.
 * Per-part differences (PSP, PIR2 contents, page pin) ride on
 * PIC14MIDRANGE_HAS_* from pic14_midrange.h. */

#include "core/pic16_irq.h"
#include "pic14_midrange.h"

/** @brief Timer0 overflow ISR (weak, overridable). */
extern void TIMER0_IRQHandler(void);
/** @brief Timer1 overflow ISR (weak, overridable). */
extern void TIMER1_IRQHandler(void);
/** @brief Timer2 period-match ISR (weak, overridable). */
extern void TIMER2_IRQHandler(void);
/** @brief CCP1 capture/compare/PWM ISR (weak, overridable). */
extern void CCP1_IRQHandler(void);
#if PIC14MIDRANGE_HAS_CCP2
/** @brief CCP2 capture/compare/PWM ISR (weak, overridable). */
extern void CCP2_IRQHandler(void);
#endif
#if PIC14MIDRANGE_HAS_SSP
/** @brief SSP (SPI/I2C) ISR (weak, overridable). */
extern void SSP_IRQHandler(void);
#endif
/** @brief USART receive ISR (weak, overridable). */
extern void USART_RX_IRQHandler(void);
/** @brief USART transmit ISR (weak, overridable). */
extern void USART_TX_IRQHandler(void);
#if PIC14MIDRANGE_HAS_ADC
/** @brief ADC conversion-done ISR (weak, overridable). */
extern void ADC_IRQHandler(void);
#endif
/** @brief EEPROM write-complete ISR (weak, overridable). */
extern void EEPROM_IRQHandler(void);
#if PIC14MIDRANGE_HAS_COMP_DUAL
/** @brief Comparator C1 change ISR (weak, overridable). */
extern void COMP1_IRQHandler(void);
/** @brief Comparator C2 change ISR (weak, overridable). */
extern void COMP2_IRQHandler(void);
#else
/** @brief Comparator change ISR (weak, overridable). */
extern void COMP_IRQHandler(void);
#endif
#if PIC14MIDRANGE_HAS_ULPWU
/** @brief ULPWU wake-up ISR (weak, overridable). */
extern void ULPWU_IRQHandler(void);
#endif
#if PIC14MIDRANGE_HAS_OSF
/** @brief Oscillator fail ISR (weak, overridable). */
extern void OSF_IRQHandler(void);
#endif
/** @brief PORTB change ISR (weak, overridable). */
extern void RB_IRQHandler(void);
#if PIC14MIDRANGE_HAS_PSP
/** @brief Parallel Slave Port ISR (weak, overridable). */
extern void PSP_IRQHandler(void);
#endif

/* The dispatcher runs in the ISR. XC8 emits no PCLATH setup for the
 * handler calls (it assumes the interrupt call-graph is linked into
 * one flash page), so the dispatch and handlers must share a page; a
 * page-crossing call lands 0x800 past its target. Pin the dispatch to
 * page 1 on parts with more than one page (host build has no pages,
 * no pin; single-page parts like the 2K 882/628A need none either). */

/**
 * @brief Dispatch every pending interrupt source to its handler.
 *
 * Reads INTCON/PIR1/PIR2 once into locals and invokes only the
 * handlers whose flag is set. Runs in the ISR; on XC8 it is pinned to
 * flash page 1 on multi-page parts so the handler calls share a page.
 */
#if defined(__XC8) && PIC14MIDRANGE_FLASH_KW >= 4
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
#if PIC14MIDRANGE_HAS_SSP
    if (pir1 & PIC_PIR1_SSPIF)  SSP_IRQHandler();
#endif
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
#if PIC14MIDRANGE_HAS_ADC
    if (pir1 & PIC_PIR1_ADIF)   ADC_IRQHandler();
#endif
#if PIC14MIDRANGE_HAS_PSP
    if (pir1 & PIC_PIR1_PSPIF)  PSP_IRQHandler();
#endif
#if PIC14MIDRANGE_HAS_CM_PIR1
    if (pir1 & PIC_PIR1_CMIF)   COMP_IRQHandler();
#endif
#if PIC14MIDRANGE_HAS_EE_PIR1
    /* Same no-steal gating as the PIR2 EEIF block below, on PIE1. */
    if (pir1 & PIC_PIR1_EEIF) {
        uint8_t eeie = 0u;
        EPIC_PIE1_READ_EEIE(eeie);
        if (eeie & PIC_PIE1_EEIE) {
            EEPROM_IRQHandler();
        }
    }
#endif
#if PIC14MIDRANGE_HAS_PIR2
    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
#if PIC14MIDRANGE_HAS_CCP2
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
#endif
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
#if PIC14MIDRANGE_HAS_COMP_DUAL
    if (pir2 & PIC_PIR2_C1IF)  COMP1_IRQHandler();
    if (pir2 & PIC_PIR2_C2IF)  COMP2_IRQHandler();
#if PIC14MIDRANGE_HAS_ULPWU
    if (pir2 & PIC_PIR2_ULPWUIF) ULPWU_IRQHandler();
#endif
#if PIC14MIDRANGE_HAS_OSF
    if (pir2 & PIC_PIR2_OSFIF) OSF_IRQHandler();
#endif
#else
    if (pir2 & PIC_PIR2_CMIF)   COMP_IRQHandler();
#endif
#endif
}
