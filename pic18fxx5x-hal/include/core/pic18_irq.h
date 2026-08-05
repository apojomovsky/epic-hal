/**
 * @file    core/pic18_irq.h
 * @brief   PIC18F2455 family interrupt controller: IRQn enum plus
 *          enable/disable/flag/priority helpers (DS39632E §9.0), mirroring
 *          STM32Cube's `HAL_NVIC_*` and the PIC16 `EPIC_IRQ_*` API.
 *
 * @details
 *   IPEN (RCON<7>) selects single-vector PIC16-compatible mode (IPEN=0) or
 *   two-vector priority mode (IPEN=1, the default here), with GIEH/GIEL
 *   gating high/low priority sources; INT0 has no priority bit, always
 *   high. `EPIC_IRQ_Restore(1)` is the drop-in for PIC16's `GIE = 1`.
 */

#ifndef PIC18_IRQ_H
#define PIC18_IRQ_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"
#include "core/pic8_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status /
 *        priority calls.
 */
typedef enum {
    PIC18_IRQ_INT0      = 0,  /**< External INT0 (RB0), always high-prio. */
    PIC18_IRQ_INT1      = 1,  /**< External INT1 (RB1).                    */
    PIC18_IRQ_INT2      = 2,  /**< External INT2 (RB2).                    */
    PIC18_IRQ_RB        = 3,  /**< RB<7:4> change.                         */
    PIC18_IRQ_TMR0      = 4,  /**< Timer0 overflow.                        */
    PIC18_IRQ_TMR1      = 5,  /**< Timer1 overflow (PIR1<TMR1IF>).         */
    PIC18_IRQ_TMR2      = 6,  /**< Timer2 == PR2 match (PIR1<TMR2IF>).     */
    PIC18_IRQ_TMR3      = 7,  /**< Timer3 overflow (PIR2<TMR3IF>).         */
    PIC18_IRQ_CCP1      = 8,  /**< CCP1 event (PIR1<CCP1IF>).              */
    PIC18_IRQ_SSP       = 9,  /**< MSSP event (PIR1<SSPIF>).               */
    PIC18_IRQ_USART_TX  = 10, /**< USART TX shift done (PIR1<TXIF>).       */
    PIC18_IRQ_USART_RX  = 11, /**< USART RX byte ready (PIR1<RCIF>).       */
    PIC18_IRQ_ADC       = 12, /**< A/D conversion done (PIR1<ADIF>).       */
    PIC18_IRQ_CCP2      = 13, /**< CCP2 event (PIR2<CCP2IF>).              */
    PIC18_IRQ_CMP       = 14, /**< Comparator change (PIR2<CMIF>).         */
    PIC18_IRQ_EEPROM    = 15, /**< EEPROM write complete (PIR2<EEIF>).     */
#if PIC18FXX5X_FAMILY_HAS_SPP
    PIC18_IRQ_SPP       = 16, /**< Streaming Parallel Port (PIR1<SPPIF>).  */
#endif
} PIC18_IRQn;

/* ───────────────────────── enable / disable ─────────────────────── */

/**
 * @brief Globally mask all interrupts by clearing the master enable(s)
 *        (INTCON<GIEH/GIEL>, DS39632E §9.0). In priority mode both
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
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        INTCON / INTCON3 / PIE1 per the source. The master enable(s) must
 *        still be set via @ref EPIC_IRQ_Restore for the source to fire.
 */
void EPIC_IRQ_Enable(PIC18_IRQn irq);

/** Disable one interrupt source. */
void EPIC_IRQ_DisableSrc(PIC18_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS39632E §9.0).
 */
void EPIC_IRQ_ClearFlag(PIC18_IRQn irq);

/** Returns the current pending state of `irq` (1 = pending). */
uint8_t EPIC_IRQ_GetFlag(PIC18_IRQn irq);

/**
 * @brief Set the priority of `irq` (high or low vector). Writes the
 *        matching bit in INTCON2 / INTCON3 / IPR1. INT0 has no priority
 *        bit (always high); setting its priority is a no-op. Takes effect
 *        only in priority mode (IPEN = 1, which @ref EPIC_IRQ_Restore
 *        enables). Part of the shared `EPIC_IRQ_*` contract (the PIC16
 *        implementation is a no-op).
 */
void EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC18_IRQ_H */
