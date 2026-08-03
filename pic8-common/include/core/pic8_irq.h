/**
 * @file    core/pic8_irq.h
 * @brief   The family-blind part of the `HAL_IRQ_*` contract: the shared
 *          priority enum. `HAL_IRQ_*` itself and `HAL_IRQ_SetPriority`
 *          are declared per family (they take that family's `PIC*_IRQn`
 *          type); PIC16 has one vector and ignores priority, PIC18 has
 *          two (DS39632E §9.0) and routes by IPR/INTCON2/INTCON3 bits.
 */

#ifndef PIC8_IRQ_H
#define PIC8_IRQ_H

/**
 * @brief   Interrupt priority level. Shared vocabulary; the effect is
 *          family-specific (PIC16 ignores it, PIC18 routes the source to
 *          the high or low vector via its IPR bits).
 */
typedef enum {
    HAL_IRQ_PRIORITY_LOW  = 0,  /**< Low-priority vector (PIC18 0018h). */
    HAL_IRQ_PRIORITY_HIGH = 1   /**< High-priority vector (PIC18 0008h). */
} HAL_IRQ_Priority;

#endif /* PIC8_IRQ_H */
