/* PIC16F63x/67x/68x interrupt controller: the IRQn enum and enable/
 * disable/flag helpers (mirrors STM32Cube HAL_NVIC_*; callers never
 * touch INTCON/PIE/PIR directly). The family-blind dispatch contract
 * lives in epic_harness.h. Sources on this ticket (16F631,
 * DS40001262F §14.0): PORTA/B change (RABIF), external INT, TMR0,
 * TMR1, EEPROM write-complete, comparators C1/C2, oscillator fail.
 * Timer2/CCP/SSP/USART/ADC rows arrive with the #152 siblings that
 * carry those peripherals. */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */
#include "core/pic14_irq_common.h"   /* shared table contract */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status calls.
 */
typedef enum {
    PIC16_IRQ_RB       = 0,  /**< PORTA/B change (IOCA/IOCB). */
    PIC16_IRQ_INT      = 1,  /**< External INT (RA2).         */
    PIC16_IRQ_TMR0     = 2,  /**< Timer0 overflow.            */
    PIC16_IRQ_TMR1     = 3,  /**< Timer1 overflow.            */
    PIC16_IRQ_EEPROM   = 4,  /**< EEPROM write complete.      */
    PIC16_IRQ_C1       = 5,  /**< Comparator C1 change.       */
    PIC16_IRQ_C2       = 6,  /**< Comparator C2 change.       */
    PIC16_IRQ_OSF      = 7,  /**< Oscillator fail.            */
} PIC16_IRQn;

/* Bound check for the shared table body. Macro, not a const global
 * (the pinned epic-cc isel turns every const global into a flash table
 * and panics on scalar ones, so a `const unsigned` here breaks the
 * epiccc gate). */
#define IRQ_TABLE_SIZE 8U

/* enable / disable. */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS40001262F §14.0, INTCON<7>).
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
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS40001262F §14.0 explicit warning).
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
 *        priority scheme, DS40001262F §14.0); declared with the shared
 *        @ref EPIC_IRQ_Priority enum so callers stay portable to PIC18,
 *        which implements it for real.
 * @param irq the interrupt source to reprioritize (ignored on PIC16).
 * @param prio the requested priority (ignored on PIC16).
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC16_IRQ_H */
