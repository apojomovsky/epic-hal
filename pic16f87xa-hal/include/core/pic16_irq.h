/**
 * @file    core/pic16_irq.h
 * @brief   PIC16F87XA interrupt controller: the IRQn enum and the
 *          enable / disable / flag helpers. The family-blind dispatch
 *          contract (epic_dispatch_all_irqs) lives in epic_harness.h.
 *
 * @details
 *   Mirrors `HAL_NVIC_*` from STM32Cube: callers never touch
 *   INTCON/PIE1/PIE2/PIR1/PIR2 directly. `EPIC_IRQ_*` names are shared
 *   across every 8-bit PIC family; `PIC16_IRQn` and the registers behind
 *   it are PIC16F87XA-specific.
 *
 *   Sources (15 on 40/44-pin, 14 on 28-pin) follow DS39582B Figure 14-10
 *   "Interrupt Logic" and §14.11 "Interrupts":
 *     - INTCON<RBIF>  RB port change   (§14.11.3)
 *     - INTCON<INTF>  INT external     (§14.11.1)
 *     - INTCON<TMR0IF> Timer0 overflow (§14.11.2)
 *     - PIR1<PSPIF>   Parallel Slave   (40/44-pin, §8.x PSP)
 *     - PIR1<ADIF>    A/D completion   (§11.x)
 *     - PIR1<RCIF>    USART RX         (§10.x)
 *     - PIR1<TXIF>    USART TX         (§10.x)
 *     - PIR1<SSPIF>   SSP event        (§9.x)
 *     - PIR1<CCP1IF>  CCP1 event       (§8.x)
 *     - PIR1<TMR2IF>  Timer2 match     (§7.x)
 *     - PIR1<TMR1IF>  Timer1 overflow  (§6.x)
 *     - PIR2<CCP2IF>  CCP2 event
 *     - PIR2<BCLIF>   SSP bus collision (I²C)
 *     - PIR2<EEIF>    EEPROM write done (§3.0)
 *     - PIR2<CMIF>    Comparator change (§12.x)
 */

#ifndef PIC16_IRQ_H
#define PIC16_IRQ_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
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
    PIC16_IRQ_CCP2     = 6,  /**< CCP2 capture/compare.      */
    PIC16_IRQ_SSP      = 7,  /**< SSP (TX/RX/I²C activity).  */
    PIC16_IRQ_BCL      = 8,  /**< SSP bus collision.         */
    PIC16_IRQ_USART_TX = 9,  /**< USART TX shift done.       */
    PIC16_IRQ_USART_RX = 10, /**< USART RX byte ready.       */
    PIC16_IRQ_ADC      = 11, /**< A/D conversion done.       */
    PIC16_IRQ_EEPROM   = 12, /**< EEPROM write complete.     */
    PIC16_IRQ_CMP      = 13, /**< Comparator output change.  */
#if PIC16F87XA_FAMILY_HAS_PSP
    PIC16_IRQ_PSP      = 14, /**< Parallel Slave Port.       */
#endif
} PIC16_IRQn;

/* ───────────────────────── enable / disable ─────────────────────── */

/**
 * @brief Globally mask all interrupts by clearing the GIE bit
 *        (DS39582B §14.11, INTCON<7>).
 * @return previous GIE state (1 = was enabled).
 */
uint8_t EPIC_IRQ_Disable(void);

/**
 * @brief Restore the global interrupt enable to `prev_state`, pair with
 *        @ref EPIC_IRQ_Disable.
 */
void EPIC_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        the matching PIE register; PIE bits need both GIE (or PEIE for
 *        peripherals) set to actually fire.
 */
void EPIC_IRQ_Enable(PIC16_IRQn irq);

/** Disable one interrupt source. */
void EPIC_IRQ_DisableSrc(PIC16_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS39582B §14.11 explicit warning).
 */
void EPIC_IRQ_ClearFlag(PIC16_IRQn irq);

/**
 * @brief Returns the current pending state of `irq` (1 = pending).
 */
uint8_t EPIC_IRQ_GetFlag(PIC16_IRQn irq);

/**
 * @brief Set the priority of `irq`. No-op on PIC16 (single vector, no
 *        priority scheme, DS39582B §14.11); declared with the shared
 *        @ref EPIC_IRQ_Priority enum so callers stay portable to PIC18,
 *        which implements it for real.
 */
void EPIC_IRQ_SetPriority(PIC16_IRQn irq, EPIC_IRQ_Priority prio);

#endif /* PIC16_IRQ_H */
