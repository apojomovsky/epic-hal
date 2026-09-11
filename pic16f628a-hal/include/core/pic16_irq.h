/* PIC16F628A interrupt controller: the IRQn enum and enable/disable/
 * flag helpers. Single PIR/PIE pair (no PIR2/PIE2): comparator and
 * EEPROM sources live in PIR1 (DS40044G §14.0). The family-blind
 * dispatch contract lives in epic_harness.h. */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f628a.h"
#include "pic16f628a_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status calls.
 */
typedef enum {
    PIC16_IRQ_RB       = 0,  /**< RB<7:4> change.            */
    PIC16_IRQ_INT      = 1,  /**< External INT (RB0).        */
    PIC16_IRQ_TMR0     = 2,  /**< Timer0 overflow.           */
    PIC16_IRQ_TMR1     = 3,  /**< Timer1 overflow.           */
    PIC16_IRQ_TMR2     = 4,  /**< Timer2 == PR2 match.       */
    PIC16_IRQ_CCP1     = 5,  /**< CCP1 capture/compare.      */
    PIC16_IRQ_USART_TX = 6,  /**< USART TX shift done.       */
    PIC16_IRQ_USART_RX = 7,  /**< USART RX byte ready.       */
    PIC16_IRQ_EEPROM   = 8,  /**< EEPROM write complete.     */
    PIC16_IRQ_CMP      = 9,  /**< Comparator output change.  */
} PIC16_IRQn;

/* Bound check for the shared table body. Macro, not a const global
 * (the pinned epic-cc isel panics on scalar consts). */
#define IRQ_TABLE_SIZE 10U

/* enable / disable. */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS40044G §14.0, INTCON<7>).
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
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        the matching PIE register; PIE bits need both GIE (or PEIE for
 *        peripherals) set to actually fire.
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
