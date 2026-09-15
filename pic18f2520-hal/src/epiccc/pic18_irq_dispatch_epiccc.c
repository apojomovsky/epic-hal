/* epic-cc dispatch, full family tier: fans out Timer0/RB plus Timer1/2/3,
 * CCP1/2, SSP, TX/RC, ADC, COMP and EEPROM (EE gated on EEIE), matching
 * the XC8 full fan-out with the same per-source gating semantics. */

#include "core/pic18_irq.h"

/** @brief Timer0 overflow IRQ handler. */
extern void TIMER0_IRQHandler(void);
/** @brief RB<7:4> change IRQ handler. */
extern void RB_IRQHandler(void);
/** @brief Timer1 overflow IRQ handler. */
extern void TIMER1_IRQHandler(void);
/** @brief Timer2 match IRQ handler. */
extern void TIMER2_IRQHandler(void);
/** @brief Timer3 overflow IRQ handler. */
extern void TIMER3_IRQHandler(void);
/** @brief CCP1 event IRQ handler. */
extern void CCP1_IRQHandler(void);
/** @brief CCP2 event IRQ handler. */
extern void CCP2_IRQHandler(void);
/** @brief MSSP transfer IRQ handler. */
extern void SSP_IRQHandler(void);
/** @brief EUSART TX IRQ handler. */
extern void USART_TX_IRQHandler(void);
/** @brief EUSART RX IRQ handler. */
extern void USART_RX_IRQHandler(void);
/** @brief ADC conversion-done handler. */
extern void ADC_IRQHandler(void);
/** @brief Comparator change handler. */
extern void COMP_IRQHandler(void);
/** @brief EEPROM write-complete handler. */
extern void EEPROM_IRQHandler(void);
/**
 * @brief  Dispatch the tier's pending interrupt sources under the same
 *         per-source gating rules as the full fan-out.
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
    if (pir2 & PIC_PIR2_CCP2IF) CCP2_IRQHandler();
    if (pir2 & PIC_PIR2_CMIF) COMP_IRQHandler();
    if (pir2 & PIC_PIR2_EEIF)
    {
        if (epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_EEIE)
        {
            EEPROM_IRQHandler();
        }
    }
}
