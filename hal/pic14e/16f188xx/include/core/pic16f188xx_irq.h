/**
 * PIC16F18855 interrupt contract. Sources: IOC (PIR0/PIE0 bit 4), Timer0
 * (PIR0/PIE0 bit 5), Timer1 and Timer2 (PIR4/PIE4 bits 0 and 1), each
 * gated by INTCON.PEIE. Enabling one also sets PEIE; the dispatcher
 * (epic_dispatch_all_irqs) acknowledges enabled ones. Single vector, no
 * priority levels, hardware context save.
 */
#ifndef PIC16F188XX_IRQ_H
#define PIC16F188XX_IRQ_H

#include <stdint.h>
#include "pic16f188xx.h"
#include "core/epic_irq.h"

typedef enum
{
    PIC16F188XX_IRQ_IOC  = 0,
    PIC16F188XX_IRQ_TMR0 = 1,
    PIC16F188XX_IRQ_TMR1 = 2,
    PIC16F188XX_IRQ_TMR2 = 3
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
 * @brief Enable an interrupt source (its PIE0 or PIE4 bit; also sets PEIE).
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
 * then TIMER0_IRQHandler, TIMER1_IRQHandler and TIMER2_IRQHandler when
 * their flag and enable are both set. INT has no driver and no API enables it, so no other PIR0 source can
 * reach the vector.
 */
void epic_dispatch_all_irqs(void);

#endif
