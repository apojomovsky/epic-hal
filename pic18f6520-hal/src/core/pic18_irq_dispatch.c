/*
 * Fan-out from both vectors to this family's IRQHandlers, shared by
 * both builds. Reads INTCON/PIRx once, calls only handlers whose bit
 * is set. Prototypes are strong externs (not EPIC_WEAK) so the host
 * linker keeps every handler object. Phase-3 tier: Timer0-3, CCP1-5,
 * SSP, EUSART1/2 (TX gated on TXIE/TX2IE, RX direct). TMR4 and the
 * analog peripherals join in #185.
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
/** @brief CCP1 event interrupt handler. */
extern void CCP1_IRQHandler(void);
/** @brief CCP2 event interrupt handler. */
extern void CCP2_IRQHandler(void);
/** @brief CCP3 event interrupt handler. */
extern void CCP3_IRQHandler(void);
/** @brief CCP4 event interrupt handler. */
extern void CCP4_IRQHandler(void);
/** @brief CCP5 event interrupt handler. */
extern void CCP5_IRQHandler(void);
/** @brief MSSP transfer interrupt handler. */
extern void SSP_IRQHandler(void);
/** @brief EUSART1 TX interrupt handler. */
extern void USART_TX_IRQHandler(void);
/** @brief EUSART1 RX interrupt handler. */
extern void USART_RX_IRQHandler(void);
/** @brief EUSART2 TX interrupt handler. */
extern void USART2_TX_IRQHandler(void);
/** @brief EUSART2 RX interrupt handler. */
extern void USART2_RX_IRQHandler(void);

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
    if (pir1 & PIC_PIR1_TMR1IF)
    {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE)
        {
            TIMER1_IRQHandler();
        }
        else
        {
            EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
        }
    }
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_CCP1IF) CCP1_IRQHandler();
    if (pir1 & PIC_PIR1_SSPIF) SSP_IRQHandler();
    /* Gate EUSART1 TX on TXIE, not TXIF: TXIF is read-only status, stays
     * set whenever TXREG is empty (same as 4550). */
    if (pir1 & PIC_PIR1_TXIF)
    {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TXIE)
        {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_RCIF) USART_RX_IRQHandler();
    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_TMR3IF) TIMER3_IRQHandler();
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    uint8_t pir3 = epic_sfr_read8(PIC_REG_PIR3);
    if (pir3 & PIC_PIR3_CCP3IF) CCP3_IRQHandler();
    if (pir3 & PIC_PIR3_CCP4IF) CCP4_IRQHandler();
    if (pir3 & PIC_PIR3_CCP5IF) CCP5_IRQHandler();
    /* Gate EUSART2 TX on TX2IE, same read-only-TXIF rationale (PIR3). */
    if (pir3 & PIC_PIR3_TX2IF)
    {
        if (epic_sfr_read8(PIC_REG_PIE3) & PIC_PIE3_TX2IE)
        {
            USART2_TX_IRQHandler();
        }
    }
    if (pir3 & PIC_PIR3_RC2IF) USART2_RX_IRQHandler();
}
