/**
 * PIC16F18855 interrupt contract. Sources: interrupt-on-change (PIR0/PIE0
 * bit 4) and Timer0 overflow (PIR0/PIE0 bit 5), each gated by INTCON.PEIE
 * like every peripheral source. Enabling either also sets PEIE, which
 * exposes PIR0 to the vector; the dispatcher acknowledges enabled ones
 * (epic_dispatch_all_irqs). Single vector, no priority levels, hardware
 * context save.
 */
#ifndef PIC16F188XX_IRQ_H
#define PIC16F188XX_IRQ_H

#include <stdint.h>
#include "pic16f188xx.h"
#include "core/epic_irq.h"

typedef enum
{
    PIC16F188XX_IRQ_IOC  = 0,
    PIC16F188XX_IRQ_TMR0 = 1
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
 * @brief Enable an interrupt source (its PIE0 bit; also sets PEIE).
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
 * then TIMER0_IRQHandler when the TMR0 flag and enable are both set.
 * INT has no driver and no API enables it, so no other PIR0 source can
 * reach the vector.
 */
void epic_dispatch_all_irqs(void);

#endif
