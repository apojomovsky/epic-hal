/* PIC16F83/84/84A interrupt controller: the IRQn enum and
 * enable/disable/flag helpers. No PIR/PIE pair on this family: the
 * EEPROM write-complete enable (EEIE) is INTCON<6> and its flag
 * (EEIF) is EECON1<4> (DS35007B §3.0, §14.11). Consumed by the shared
 * pic14-midrange-core pic14_irq.c body. */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f83_84_hal.h"
#include "pic16f83_84_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status calls.
 */
typedef enum {
    PIC16_IRQ_RB       = 0,  /**< RB<7:4> change.            */
    PIC16_IRQ_INT      = 1,  /**< External INT (RB0).        */
    PIC16_IRQ_TMR0     = 2,  /**< Timer0 overflow.           */
    PIC16_IRQ_EEPROM   = 3,  /**< EEPROM write complete.     */
} PIC16_IRQn;

/* Bound check for the shared table body. Macro, not a const global
 * (the pinned epic-cc isel panics on scalar consts). */
#define IRQ_TABLE_SIZE 4U

/* enable / disable. */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS35007B §14.11, INTCON<7>).
 * @return previous GIE state (1 = was enabled).
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief Restore the global interrupt enable to `prev_state`, pair with
 *        @ref EPIC_IRQ_Disable.
 * @param prev_state the GIE state returned by @ref EPIC_IRQ_Disable.
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source (sets the matching INTCON bit).
 * @param irq the interrupt source to enable.
 */
void EPIC_IRQ_Enable(PIC16_IRQn irq);

/**
 * @brief Disable one interrupt source.
 * @param irq the interrupt source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry.
 * @param irq the interrupt source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16_IRQn irq);

/**
 * @brief Returns the current pending state of `irq` (1 = pending).
 * @param irq the interrupt source to query.
 * @return 1 if the flag is set (pending), 0 otherwise.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16_IRQn irq);

/**
 * @brief Set the priority of `irq`. No-op on PIC16 (single vector, no
 *        priority scheme); declared with the shared @ref EPIC_IRQ_Priority
 *        enum so callers stay portable to PIC18.
 * @param irq the interrupt source (ignored on PIC16).
 * @param prio the requested priority (ignored on PIC16).
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC16_IRQ_H */
