/* PIC16F5x interrupt controller: the IRQn enum and the
 * enable/disable/flag helpers. This core has NO interrupt vector, no
 * INTCON, and no interrupt source at all (DS41213D §4.0: no IRQ
 * hardware anywhere; the 2-level stack is call/return only). The
 * EPIC_IRQ_* contract is therefore a stub: every function is a no-op
 * and the IRQn enum is empty, so family-agnostic consumers that call
 * the API unconditionally build and link unchanged. Applications must
 * poll peripherals (there is no ISR path). */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        This core has none; the enum exists so the shared contract
 *        has a type to parameterize the no-op helpers with.
 */
typedef enum {
    PIC16_IRQ_NONE = 0,  /**< No interrupt sources exist on the 5x core. */
} PIC16_IRQn;

/* enable / disable. */

/**
 * @brief Globally mask all interrupts. No-op on this core: there is no
 *        global enable bit.
 * @return the previous "enabled" state; trivially 0 (nothing was
 *         enabled).
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief Restore the global interrupt enable, pair with @ref
 *        EPIC_IRQ_Disable. No-op on this core.
 * @param prev_state the state returned by @ref EPIC_IRQ_Disable.
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. No-op on this core.
 * @param irq the interrupt source to enable.
 */
void EPIC_IRQ_Enable(PIC16_IRQn irq);

/**
 * @brief Disable one interrupt source. No-op on this core.
 * @param irq the interrupt source to disable.
 */
void EPIC_IRQ_DisableSrc(PIC16_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. No-op on this core.
 * @param irq the interrupt source whose flag to clear.
 */
void EPIC_IRQ_ClearFlag(PIC16_IRQn irq);

/**
 * @brief Returns the current pending state of `irq`. Always 0 on this
 *        core.
 * @param irq the interrupt source to query.
 * @return 1 if the flag is set (pending), 0 otherwise.
 */
uint8_t EPIC_IRQ_GetFlag(PIC16_IRQn irq);

/**
 * @brief Set the priority of `irq`. No-op on this core (no vectors, no
 *        priority scheme); declared with the shared @ref
 *        EPIC_IRQ_Priority enum so callers stay portable.
 * @param irq the interrupt source (ignored).
 * @param prio the requested priority (ignored).
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC16_IRQ_H */
