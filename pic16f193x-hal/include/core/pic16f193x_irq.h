/**
 * @file    core/pic16f193x_irq.h
 * @brief   PIC16F193X interrupt controller: the IRQn enum and the
 *          enable / disable / flag helpers. The family-blind dispatch
 *          contract (pic8_dispatch_all_irqs) lives in pic8_harness.h.
 *
 * @details
 *   Mirrors `HAL_NVIC_*` from STM32Cube: callers never touch
 *   INTCON/PIE1/PIE2/PIE3/PIR1/PIR2/PIR3 directly. `HAL_IRQ_*` names are
 *   shared across every 8-bit PIC family; `PIC16F193X_IRQn` and the
 *   registers behind it are PIC16F193X-specific.
 *
 *   Single interrupt vector at 0x0004, no priority (DS41364B §4.0): GIE
 *   in INTCON gates everything, PEIE gates the peripheral sources in
 *   PIE1/PIE2/PIE3. Hardware saves W/STATUS/BSR/FSR0/FSR1/PCLATH to
 *   shadow registers on entry, so no manual context save is needed
 *   (unlike classic PIC16F87XA). HAL_IRQ_SetPriority is a no-op.
 *
 *   23 sources (DS41364B §4.0, Figure 4-1/4-2 and Registers 4-1..4-7):
 *     INTCON:  IOC (RB change), INT (RB0 edge), TMR0 (overflow)
 *     PIR1/PIE1: TMR1, TMR2, CCP1, SSP, USART_TX, USART_RX, ADC, TMR1G
 *     PIR2/PIE2: CCP2, LCD, BCL, EEPROM, CMP1, CMP2, OSF
 *     PIR3/PIE3: TMR4, TMR6, CCP3, CCP4, CCP5
 */

#ifndef PIC16F193X_IRQ_H
#define PIC16F193X_IRQ_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "core/pic8_irq.h"   /* shared HAL_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status calls.
 *
 * @note  The order is stable: peripheral phases add their handlers against
 *        these values, so do not renumber existing entries.
 */
typedef enum {
    PIC16F193X_IRQ_IOC      = 0,  /**< PORTB interrupt-on-change.   */
    PIC16F193X_IRQ_INT     = 1,  /**< External INT (RB0).          */
    PIC16F193X_IRQ_TMR0    = 2,  /**< Timer0 overflow.             */
    PIC16F193X_IRQ_TMR1    = 3,  /**< Timer1 overflow.            */
    PIC16F193X_IRQ_TMR2    = 4,  /**< Timer2 == PR2 match.        */
    PIC16F193X_IRQ_CCP1    = 5,  /**< ECCP1 capture/compare/PWM.  */
    PIC16F193X_IRQ_SSP     = 6,  /**< MSSP event (SPI/I2C).       */
    PIC16F193X_IRQ_USART_TX = 7, /**< EUSART TX shift done.       */
    PIC16F193X_IRQ_USART_RX = 8, /**< EUSART RX byte ready.       */
    PIC16F193X_IRQ_ADC     = 9,  /**< A/D conversion done.        */
    PIC16F193X_IRQ_TMR1G   = 10, /**< Timer1 gate.               */
    PIC16F193X_IRQ_CCP2    = 11, /**< ECCP2 capture/compare/PWM.  */
    PIC16F193X_IRQ_LCD     = 12, /**< LCD driver frame.          */
    PIC16F193X_IRQ_BCL     = 13, /**< MSSP bus collision (I2C).  */
    PIC16F193X_IRQ_EEPROM  = 14, /**< EEPROM/Flash write done.    */
    PIC16F193X_IRQ_CMP1    = 15, /**< Comparator C1 change.      */
    PIC16F193X_IRQ_CMP2    = 16, /**< Comparator C2 change.      */
    PIC16F193X_IRQ_OSF     = 17, /**< Oscillator fail.           */
    PIC16F193X_IRQ_TMR4    = 18, /**< Timer4 == PR4 match.       */
    PIC16F193X_IRQ_TMR6    = 19, /**< Timer6 == PR6 match.       */
    PIC16F193X_IRQ_CCP3    = 20, /**< ECCP3 capture/compare/PWM.  */
    PIC16F193X_IRQ_CCP4    = 21, /**< CCP4 capture/compare.      */
    PIC16F193X_IRQ_CCP5    = 22, /**< CCP5 capture/compare.      */
} PIC16F193X_IRQn;

/* ───────────────────────── enable / disable ─────────────────────── */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS41364B §4.1, INTCON<7>).
 * @return previous GIE state (1 = was enabled).
 */
uint8_t HAL_IRQ_Disable(void);

/**
 * @brief Restore the global interrupt enable to `prev_state`, pair with
 *        @ref HAL_IRQ_Disable.
 */
void HAL_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        the matching PIE register (or INTCON for IOC/INT/TMR0); PIE bits
 *        need both GIE and PEIE set to actually fire.
 */
void HAL_IRQ_Enable(PIC16F193X_IRQn irq);

/** Disable one interrupt source. */
void HAL_IRQ_DisableSrc(PIC16F193X_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS41364B §4.1).
 */
void HAL_IRQ_ClearFlag(PIC16F193X_IRQn irq);

/** Returns the current pending state of `irq` (1 = pending). */
uint8_t HAL_IRQ_GetFlag(PIC16F193X_IRQn irq);

/**
 * @brief Set the priority of `irq`. No-op on PIC16F193X (single vector, no
 *        priority scheme, DS41364B §4.0); declared with the shared
 *        @ref HAL_IRQ_Priority enum so callers stay portable to PIC18,
 *        which implements it for real.
 */
void HAL_IRQ_SetPriority(PIC16F193X_IRQn irq, HAL_IRQ_Priority prio);

#endif /* PIC16F193X_IRQ_H */
