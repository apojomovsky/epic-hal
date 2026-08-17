/* PIC16F88X interrupt controller: the IRQn enum and enable/disable/
 * flag helpers (mirrors STM32Cube HAL_NVIC_*; callers never touch
 * INTCON/PIE/PIR directly). The family-blind dispatch contract lives
 * in epic_harness.h. Sources (17) follow DS40001291H Figure 14-10 /
 * §14.11. */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status calls.
 */
typedef enum {
    PIC16_IRQ_RB       = 0,  /**< RB<7:0> change (IOCB).      */
    PIC16_IRQ_INT      = 1,  /**< External INT (RB0).         */
    PIC16_IRQ_TMR0     = 2,  /**< Timer0 overflow.            */
    PIC16_IRQ_TMR1     = 3,  /**< Timer1 overflow.            */
    PIC16_IRQ_TMR2     = 4,  /**< Timer2 == PR2 match.        */
    PIC16_IRQ_CCP1     = 5,  /**< ECCP1 capture/compare.      */
    PIC16_IRQ_CCP2     = 6,  /**< CCP2 capture/compare.       */
    PIC16_IRQ_SSP      = 7,  /**< MSSP (TX/RX/I²C activity).  */
    PIC16_IRQ_BCL      = 8,  /**< MSSP bus collision.         */
    PIC16_IRQ_USART_TX = 9,  /**< EUSART TX shift done.       */
    PIC16_IRQ_USART_RX = 10, /**< EUSART RX byte ready.       */
    PIC16_IRQ_ADC      = 11, /**< A/D conversion done.        */
    PIC16_IRQ_EEPROM   = 12, /**< EEPROM write complete.      */
    PIC16_IRQ_C1       = 13, /**< Comparator C1 change.       */
    PIC16_IRQ_C2       = 14, /**< Comparator C2 change.       */
    PIC16_IRQ_ULPWU    = 15, /**< Ultra low-power wake-up.    */
    PIC16_IRQ_OSF      = 16, /**< Oscillator fail.            */
} PIC16_IRQn;

/* enable / disable. */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS40001291H §14.11, INTCON<7>).
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
 *        (DS40001291H §14.11 explicit warning).
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
 *        priority scheme, DS40001291H §14.11); declared with the shared
 *        @ref EPIC_IRQ_Priority enum so callers stay portable to PIC18,
 *        which implements it for real.
 * @param irq the interrupt source to reprioritize (ignored on PIC16).
 * @param prio the requested priority (ignored on PIC16).
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC16_IRQ_H */
