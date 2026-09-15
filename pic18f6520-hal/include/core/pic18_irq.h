/*
 * PIC18F6520 interrupt controller (DS39609B §9.0): IRQn enum plus
 * enable/disable/flag/priority helpers on the EPIC_IRQ_* contract.
 * Two-vector priority mode (IPEN=1) with GIEH/GIEL gating; INT0 has no
 * priority bit (always high). Richer than the 2455/2520 families: INT3,
 * TMR4, CCP3-5, USART2, PSP and LVD sources exist on this 64-pin part
 * (DS39609B §9.0, Table 1-1).
 */

#ifndef PIC18_IRQ_H
#define PIC18_IRQ_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "core/epic_irq.h"   /* shared EPIC_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status /
 *        priority calls.
 */
typedef enum {
    PIC18_IRQ_INT0      = 0,  /**< External INT0 (RB0), always high-prio. */
    PIC18_IRQ_INT1      = 1,  /**< External INT1 (RB1).                    */
    PIC18_IRQ_INT2      = 2,  /**< External INT2 (RB2).                    */
    PIC18_IRQ_INT3      = 3,  /**< External INT3 (RB3), INTCON3<INT3IF>.   */
    PIC18_IRQ_RB        = 4,  /**< RB<7:4> change.                         */
    PIC18_IRQ_TMR0      = 5,  /**< Timer0 overflow.                        */
    PIC18_IRQ_TMR1      = 6,  /**< Timer1 overflow (PIR1<TMR1IF>).         */
    PIC18_IRQ_TMR2      = 7,  /**< Timer2 == PR2 match (PIR1<TMR2IF>).     */
    PIC18_IRQ_TMR3      = 8,  /**< Timer3 overflow (PIR2<TMR3IF>).         */
    PIC18_IRQ_TMR4      = 9,  /**< Timer4 == PR4 match (PIR3<TMR4IF>).     */
    PIC18_IRQ_CCP1      = 10, /**< CCP1 event (PIR1<CCP1IF>).              */
    PIC18_IRQ_CCP2      = 11, /**< CCP2 event (PIR2<CCP2IF>).              */
    PIC18_IRQ_CCP3      = 12, /**< CCP3 event (PIR3<CCP3IF>).              */
    PIC18_IRQ_CCP4      = 13, /**< CCP4 event (PIR3<CCP4IF>).              */
    PIC18_IRQ_CCP5      = 14, /**< CCP5 event (PIR3<CCP5IF>).              */
    PIC18_IRQ_SSP       = 15, /**< MSSP event (PIR1<SSPIF>).               */
    PIC18_IRQ_USART1_TX = 16, /**< USART1 TX shift done (PIR1<TXIF>).      */
    PIC18_IRQ_USART1_RX = 17, /**< USART1 RX byte ready (PIR1<RCIF>).      */
    PIC18_IRQ_USART2_TX = 18, /**< USART2 TX shift done (PIR3<TX2IF>).     */
    PIC18_IRQ_USART2_RX = 19, /**< USART2 RX byte ready (PIR3<RC2IF>).     */
    PIC18_IRQ_ADC       = 20, /**< A/D conversion done (PIR1<ADIF>).       */
    PIC18_IRQ_CMP       = 21, /**< Comparator change (PIR2<CMIF>).          */
    PIC18_IRQ_EEPROM    = 22, /**< EEPROM write complete (PIR2<EEIF>).     */
    PIC18_IRQ_LVD       = 23, /**< Low-voltage detect (PIR2<LVDIF>).        */
    PIC18_IRQ_PSP       = 24, /**< Parallel Slave Port (PIR1<PSPIF>).      */
} PIC18_IRQn;

/**
 * @brief  Globally mask all interrupts by clearing the master enable(s)
 *        (INTCON<GIEH/GIEL>, DS39609B §9.0). In priority mode both
 *        GIEH and GIEL are cleared.
 * @return 1 if any master enable was set (interrupts were on), else 0.
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief  Restore the master interrupt enable(s). `prev_state` is the value
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
 *        INTCON / INTCON3 / PIE1-3 per the source. The master enable(s)
 *        must still be set via @ref EPIC_IRQ_Restore for the source to
 *        fire.
 * @param irq the interrupt source to enable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_Enable(PIC18_IRQn irq);

/**
 * @brief Disable one interrupt source.
 * @param irq the interrupt source to disable (a @ref PIC18_IRQn value).
 */
void EPIC_IRQ_DisableSrc(PIC18_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. MUST be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS39609B §9.0).
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
 *        matching bit in INTCON2 / INTCON3 / IPR1-3. INT0 has no priority
 *        bit (always high); setting its priority is a no-op. Takes effect
 *        only in priority mode (IPEN = 1, which @ref EPIC_IRQ_Restore
 *        enables).
 * @param irq  the interrupt source.
 * @param prio the desired priority (high or low vector).
 */
void EPIC_IRQ_SetPriority(PIC18_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC18_IRQ_H */
