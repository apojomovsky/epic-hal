/* epic-cc dispatch tiers: one shared body, compiled per module class by
 * defining EPICCC_IRQ_* before the include. Only the gated sources
 * dispatch; every other source is not in the module's HAL subset and
 * cannot vector (PIE bit off), so no flag-clear scaffolding is needed.
 * Rationale: the full fan-out pulls every peripheral handler into the
 * whole program, and the 368-byte GPR parts cannot pay for the unused
 * ones. Handler-less TMR1IF/TMR2IF would latch while the peripheral
 * free-runs, but that peripheral is not compiled in for tiers that do
 * not set the gate. */

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
extern void TIMER1_IRQHandler(void);
#endif
#if EPICCC_IRQ_TMR2
extern void TIMER2_IRQHandler(void);
#endif
#if EPICCC_IRQ_CCP1
extern void CCP1_IRQHandler(void);
#endif
#if EPICCC_IRQ_CCP2
extern void CCP2_IRQHandler(void);
#endif
#if EPICCC_IRQ_USART
extern void USART_RX_IRQHandler(void);
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
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
#endif
#if EPICCC_IRQ_CCP1
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
#endif
#if EPICCC_IRQ_USART
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_RCIF) USART_RX_IRQHandler();
    if (EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TXIF) {
        uint8_t txie; EPIC_PIE1_READ_TXIE(txie);
        if (txie & PIC_PIE1_TXIE) USART_TX_IRQHandler();
    }
#endif
#if EPICCC_IRQ_CCP2
    if (EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
#endif
}
