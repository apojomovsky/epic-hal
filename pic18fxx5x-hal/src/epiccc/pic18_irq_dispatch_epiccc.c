/* epic-cc dispatch, serial + tick + EEPROM tier (the PIC18 combo
 * firmwares): USART RX/TX, the TIMER2 timebase and the EEPROM
 * write-complete dispatch, with the same gating semantics as the full
 * fan-out: TX dispatches only under TXIE (TXIF is a status bit that
 * stays set while TXREG is empty) and EEIF is left to its polling
 * consumer when EEIE is off. Sources outside the slice are not listed
 * in the HAL subset and cannot vector (PIE off). */

#include "core/pic18_irq.h"

/** @brief Timer2 match IRQ handler. */
extern void TIMER2_IRQHandler(void);
/** @brief USART TX shift-done IRQ handler. */
extern void USART_TX_IRQHandler(void);
/** @brief USART RX byte-ready IRQ handler. */
extern void USART_RX_IRQHandler(void);
/** @brief EEPROM write-complete IRQ handler. */
extern void EEPROM_IRQHandler(void);

/**
 * @brief  Dispatch the tier's pending interrupt sources under the same
 *         per-source gating rules as the full fan-out.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t pir1 = epic_sfr_read8(PIC_REG_PIR1);
    if (pir1 & PIC_PIR1_TMR2IF) TIMER2_IRQHandler();
    if (pir1 & PIC_PIR1_TXIF) {
        if (epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_TXIE) {
            USART_TX_IRQHandler();
        }
    }
    if (pir1 & PIC_PIR1_RCIF) USART_RX_IRQHandler();

    uint8_t pir2 = epic_sfr_read8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_EEIF) {
        if (epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_EEIE) {
            EEPROM_IRQHandler();
        }
    }
}
