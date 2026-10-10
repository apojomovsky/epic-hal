/**
 * PIC16F1508/1509 interrupt contract. Only interrupt-on-change is wired: it
 * is gated by INTCON alone, so this family needs no peripheral-interrupt
 * (PIE) handling.
 */
#ifndef PIC16F1508_IRQ_H
#define PIC16F1508_IRQ_H

#include <stdint.h>
#include "pic16f1508.h"
#include "core/epic_irq.h"

typedef enum
{
    PIC16F1508_IRQ_IOC = 0
} PIC16F1508_IRQn;

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
 * @brief Enable an interrupt source (sets its INTCON enable bit).
 * @param irq Source to enable.
 */
void EPIC_IRQ_Enable(PIC16F1508_IRQn irq);

/**
 * @brief Disable an interrupt source (clears its INTCON enable bit).
 * @param irq Source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16F1508_IRQn irq);

/**
 * @brief Clear a pending interrupt flag.
 * @param irq Source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16F1508_IRQn irq);

/**
 * @brief Read a pending interrupt flag.
 * @param irq Source whose flag to read.
 * @return 1 if pending, 0 otherwise.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16F1508_IRQn irq);

/**
 * @brief Set an interrupt priority. No-op: this core has a single vector and no priority.
 * @param irq Source (ignored).
 * @param prio Priority (ignored).
 */
void EPIC_IRQ_SetPriority(PIC16F1508_IRQn irq, EPIC_IRQ_Priority prio);

/**
 * @brief Shared interrupt dispatcher, called from the single interrupt vector.
 * @details Runs IOC_IRQHandler when IOCIF and IOCIE are both set.
 */
void epic_dispatch_all_irqs(void);

#endif
