/*
 * Fan-out from the PIC18 interrupt vectors to every peripheral
 * IRQHandler with a linked driver (both vectors call this on target;
 * the host harness registers it as the sim IRQ callback). Reads
 * INTCON/PIRx once, calls only handlers whose bit is set. Complete
 * family phase: Timer0-3, ECCP1, USART TX/RX, ADC and EEPROM all
 * dispatch from here.
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
/** @brief ECCP1 event interrupt handler. */
extern void CCP1_IRQHandler(void);
/** @brief EUSART TX interrupt handler. */
extern void USART_TX_IRQHandler(void);
/** @brief EUSART RX interrupt handler. */
extern void USART_RX_IRQHandler(void);
/** @brief ADC conversion-done handler. */
extern void ADC_IRQHandler(void);
/** @brief EEPROM write-complete handler. */
extern void EEPROM_IRQHandler(void);

/**
 * @brief  Fan out from the PIC18 interrupt vectors to every peripheral
 *         IRQHandler whose flag is set and whose driver is linked in.
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
    /* Gate TX on TXIE, not TXIF: TXIF is read-only status, stays set
     * whenever TXREG is empty (same as 4550). */
    if (pir1 & PIC_PIR1_TXIF)
    {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TXIE)
        {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_RCIF) USART_RX_IRQHandler();
    if (pir1 & PIC_PIR1_ADIF) ADC_IRQHandler();

    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_TMR3IF) TIMER3_IRQHandler();
    /* Gate EEIF on EEIE, leave untouched when disabled: pollers own it. */
    if (pir2 & PIC_PIR2_EEIF)
    {
        if (epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_EEIE)
        {
            EEPROM_IRQHandler();
        }
    }
}
