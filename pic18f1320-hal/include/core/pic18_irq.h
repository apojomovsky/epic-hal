/*
 * PIC18F1320 interrupt controller (DS39605F §9.0): IRQn enum plus
 * enable/disable/flag/priority helpers. Foundation phase: only sources
 * with a real dispatch entry in pic18_irq_dispatch.c are named here (an
 * enabled source with no dispatch entry never clears and re-vectors
 * forever); the rest join as each ticket lands its dispatch entry.
 */

#ifndef PIC18_IRQ_H
#define PIC18_IRQ_H

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source with a real
 *        dispatch entry. Used as the parameter for enable / disable /
 *        clear / status / priority calls.
 */
typedef enum {
    PIC18_IRQ_RB        = 0,  /**< RB<7:4> change.                         */
    PIC18_IRQ_TMR0      = 1,  /**< Timer0 overflow.                        */
} PIC18_IRQn;

/**
 * @brief Globally mask all interrupts by clearing the master enable(s)
 *        (INTCON<GIEH/GIEL>, DS39605F §9.0). In priority mode both
 *        GIEH and GIEL are cleared.
 * @return 1 if any master enable was set (interrupts were on), else 0.
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief Restore the master interrupt enable(s). `prev_state` is the value
 *        returned by @ref EPIC_IRQ_Disable. Restoring to "on" also ensures
 *        IPEN = 1 (priority mode) so the two-vector scheme is active. Pair
 *        with @ref EPIC_IRQ_Disable. `EPIC_IRQ_Restore(1)` enables all
 *        interrupts (the drop-in for PIC16's `GIE = 1`).
 * @param prev_state the value returned by @ref EPIC_IRQ_Disable; 1 enables
 *        all interrupts, 0 keeps them masked.
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        INTCON per the source. The master enable(s) must still be set
 *        via @ref EPIC_IRQ_Restore for the source to fire.
 * @param irq the interrupt source to enable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_Enable(PIC18_IRQn irq);

/**
 * @brief Disable one interrupt source.
 * @param irq the interrupt source to disable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_DisableSrc(PIC18_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS39605F §9.0).
 * @param irq the interrupt source whose flag is cleared.
 */
void EPIC_IRQ_ClearFlag(PIC18_IRQn irq);

/**
 * @brief Returns the current pending state of `irq` (1 = pending).
 * @param irq the interrupt source to query (a @ref PIC18_IRQn value).
 * @return 1 if the interrupt flag is set (pending), else 0.
 */
uint8_t EPIC_IRQ_GetFlag(PIC18_IRQn irq);

/**
 * @brief Set the priority of `irq` (high or low vector). Writes the
 *        matching bit in INTCON2. Takes effect only in priority mode
 *        (IPEN = 1, which @ref EPIC_IRQ_Restore enables). Part of the
 *        shared `EPIC_IRQ_*` contract (the PIC16 implementation is a
 *        no-op).
 * @param irq the interrupt source to configure (a @ref PIC18_IRQn value).
 * @param prio the priority to assign (EPIC_IRQ_PRIORITY_HIGH or
 *        EPIC_IRQ_PRIORITY_LOW).
 */
void EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC18_IRQ_H */
