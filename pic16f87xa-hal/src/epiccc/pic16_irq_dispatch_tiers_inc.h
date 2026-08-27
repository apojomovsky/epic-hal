/* epic-cc dispatch tiers: one shared body per module class via
 * EPICCC_IRQ_* gates. Only gated sources dispatch; others are not in
 * the HAL subset and cannot vector (PIE off), so no clear scaffolding.
 * The full fan-out pulls every handler into the program, and 368-byte
 * GPR cannot pay for the unused ones. */

#include "core/pic16_irq.h"

#ifndef EPICCC_IRQ_USART
#define EPICCC_IRQ_USART 0
#endif
#ifndef EPICCC_IRQ_TMR1
#define EPICCC_IRQ_TMR1 0
#endif
#ifndef EPICCC_IRQ_TMR2
#define EPICCC_IRQ_TMR2 0
#endif
#ifndef EPICCC_IRQ_CCP1
#define EPICCC_IRQ_CCP1 0
#endif
#ifndef EPICCC_IRQ_CCP2
#define EPICCC_IRQ_CCP2 0
#endif

#if EPICCC_IRQ_TMR1
/** @brief Timer1 IRQ handler. */
extern void TIMER1_IRQHandler(void);
#endif
#if EPICCC_IRQ_TMR2
/** @brief Timer2 IRQ handler. */
extern void TIMER2_IRQHandler(void);
#endif
#if EPICCC_IRQ_CCP1
/** @brief CCP1 IRQ handler. */
extern void CCP1_IRQHandler(void);
#endif
#if EPICCC_IRQ_CCP2
/** @brief CCP2 IRQ handler. */
extern void CCP2_IRQHandler(void);
#endif
#if EPICCC_IRQ_USART
/** @brief USART RX IRQ handler. */
extern void USART_RX_IRQHandler(void);
/** @brief USART TX IRQ handler. */
extern void USART_TX_IRQHandler(void);
#endif

/**
 * @brief Dispatch the tier's pending interrupt sources.
 */
void epic_dispatch_all_irqs(void)
{
#if EPICCC_IRQ_TMR1
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) {
        uint8_t tmr1ie; EPIC_PIE1_READ_TMR1IE(tmr1ie);
        if (tmr1ie & PIC_PIE1_TMR1IE) {
            TIMER1_IRQHandler();
        } else {
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
#endif
#if EPICCC_IRQ_TMR2
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) {
        if (EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) TIMER2_IRQHandler();
        else EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR2IF);
    }
#endif
#if EPICCC_IRQ_CCP1
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_CCP1IF) {
        if (EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_CCP1IE) CCP1_IRQHandler();
        else EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_CCP1IF);
    }
#endif
#if EPICCC_IRQ_USART
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_RCIF) USART_RX_IRQHandler();
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TXIF) {
        uint8_t txie; EPIC_PIE1_READ_TXIE(txie);
        if (txie & PIC_PIE1_TXIE) USART_TX_IRQHandler();
    }
#endif
#if EPICCC_IRQ_CCP2
    if (EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_CCP2IF) {
        if (EPIC_REG8(PIC_REG_PIE2) & PIC_PIE2_CCP2IE) CCP2_IRQHandler();
        else EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CCP2IF);
    }
#endif
}
