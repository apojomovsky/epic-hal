/**
 * PIC16F18855 interrupt contract. Sources: IOC/Timer0 (PIR0/PIE0),
 * ADC (PIR1/PIE1 bit 0), C1/C2 (PIR2/PIE2), MSSP1/EUSART (PIR3/PIE3),
 * Timer1/Timer2 (PIR4/PIE4), CCP1..CCP5 (PIR6/PIE6), NCO1/NVM (PIR7/
 * PIE7), each gated by INTCON.PEIE; enabling one sets PEIE. Single
 * vector, no priority levels, hardware context save.
 */
#ifndef PIC16F188XX_IRQ_H
#define PIC16F188XX_IRQ_H

#include <stdint.h>
#include "pic16f188xx.h"
#include "core/epic_irq.h"

typedef enum
{
    PIC16F188XX_IRQ_IOC      = 0,
    PIC16F188XX_IRQ_TMR0     = 1,
    PIC16F188XX_IRQ_SSP1     = 2,
    PIC16F188XX_IRQ_USART_TX = 3,
    PIC16F188XX_IRQ_USART_RX = 4,
    PIC16F188XX_IRQ_TMR1     = 5,
    PIC16F188XX_IRQ_TMR2     = 6,
    PIC16F188XX_IRQ_EEPROM   = 7,
    PIC16F188XX_IRQ_NCO1     = 8,
    PIC16F188XX_IRQ_CCP1     = 9,
    PIC16F188XX_IRQ_CCP2     = 10,
    PIC16F188XX_IRQ_CCP3     = 11,
    PIC16F188XX_IRQ_CCP4     = 12,
    PIC16F188XX_IRQ_CCP5     = 13,
    PIC16F188XX_IRQ_ADC      = 14,
    PIC16F188XX_IRQ_CMP1     = 15,
    PIC16F188XX_IRQ_CMP2     = 16
} PIC16F188XX_IRQn;

/**
 * @brief Disable global interrupts.
 * @return 1 if GIE was set before the call, 0 otherwise. Pass to EPIC_IRQ_Restore.
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief Restore the global interrupt state returned by EPIC_IRQ_Disable.
 * @param prev_state 1 sets GIE, 0 clears it.
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable an interrupt source (its PIE0, PIE1, PIE2, PIE3, PIE4, PIE6 or PIE7 bit; also sets PEIE).
 * @param irq Source to enable.
 */
void EPIC_IRQ_Enable(PIC16F188XX_IRQn irq);

/**
 * @brief Disable an interrupt source (clears its enable bit; PEIE stays set).
 * @param irq Source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16F188XX_IRQn irq);

/**
 * @brief Clear a pending interrupt flag.
 * @details USART_TX and USART_RX flags are read-only status that hardware
 * clears (TXREG write, RCREG read), so clearing them is a no-op.
 * @param irq Source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16F188XX_IRQn irq);

/**
 * @brief Read a pending interrupt flag.
 * @param irq Source whose flag to read.
 * @return 1 if pending, 0 otherwise.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16F188XX_IRQn irq);

/**
 * @brief Set an interrupt priority. No-op: this core has a single vector and no priority.
 * @param irq Source (ignored).
 * @param prio Priority (ignored).
 */
void EPIC_IRQ_SetPriority(PIC16F188XX_IRQn irq, EPIC_IRQ_Priority prio);

/**
 * @brief Shared interrupt dispatcher, called from the single interrupt vector.
 * @details Runs IOC_IRQHandler when the IOC flag and enable are both set,
 * then TIMER0_IRQHandler for TMR0, ADC_IRQHandler for ADIF, CMP1_IRQHandler
 * and CMP2_IRQHandler for their PIR2 flags, then SSP_IRQHandler,
 * USART_TX_IRQHandler and USART_RX_IRQHandler for their PIR3 flags, the
 * Timer1 and Timer2 handlers for PIR4, CCP1_IRQHandler..CCP5_IRQHandler for
 * PIR6, then EEPROM_IRQHandler and NCO_IRQHandler for their PIR7 flags,
 * each gated the same way. INT and the bus-collision flag have no handler
 * and no API enables them, so no other source can reach the vector.
 */
void epic_dispatch_all_irqs(void);

#endif
