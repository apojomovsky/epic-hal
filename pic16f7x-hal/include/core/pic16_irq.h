/* PIC16F7x interrupt controller: the IRQn enum and enable/disable/
 * flag helpers (mirrors STM32Cube HAL_NVIC_*; callers never touch
 * INTCON/PIE/PIR directly). The family-blind dispatch contract lives
 * in epic_harness.h. Sources: the 7x has no EEPROM (the PM* program-
 * memory bank replaces it, so there is no EEIF), no comparator, and no
 * SSP bus-collision flag (its PIR2 carries only CCP2IF; DFP-verified).
 * What remains is the classic RB/INT/TMR0/TMR1/TMR2/CCP1/CCP2/SSP/
 * USART-TX/RX/ADC set plus PSP on the 40-pin parts. */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f7x.h"
#include "pic16f7x_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */
#include "core/pic14_irq_common.h"   /* shared table contract */

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
    PIC16_IRQ_CCP2     = 6,  /**< CCP2 capture/compare (40-pin & CCP2 parts). */
    PIC16_IRQ_SSP      = 7,  /**< SSP (SPI activity).        */
    PIC16_IRQ_USART_TX = 8,  /**< USART TX shift done.       */
    PIC16_IRQ_USART_RX = 9,  /**< USART RX byte ready.       */
    PIC16_IRQ_ADC      = 10, /**< A/D conversion done.       */
#if PIC16F7X_FAMILY_HAS_PSP
    PIC16_IRQ_PSP      = 11, /**< Parallel Slave Port.       */
#endif
} PIC16_IRQn;

/* Bound check for the shared table body. Macro, not a const global
 * (the pinned epic-cc isel panics on scalar consts). 11 rows plus
 * the PSP row on PSP variants. */
#define IRQ_TABLE_SIZE (11U + (unsigned)PIC16F7X_FAMILY_HAS_PSP)

/* enable / disable. */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS30325 §11.0, INTCON<7>).
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
