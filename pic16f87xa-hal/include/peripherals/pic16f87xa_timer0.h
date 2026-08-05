/**
 * @file    peripherals/pic16f87xa_timer0.h
 * @brief   Timer0 driver, 8-bit timer/counter with shared prescaler.
 *
 * @details
 *   Source: DS39582B §5.0, §5.3. Full reference: MANUAL.md §10.
 *   Writing TMR0 clears the prescaler (§5.3 Note). The driver never
 *   touches PSA while WDT is active; switching the shared prescaler
 *   from TMR0 to WDT needs the exact sequence in §5.3 footnote 1 to
 *   avoid a spurious reset.
 */

#ifndef PIC16F87XA_TIMER0_H
#define PIC16F87XA_TIMER0_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief Timer0 clock source (OPTION_REG<T0CS>, DS39582B §5.0, Reg 5-1).
 */
typedef enum {
    TIMER0_CLOCK_INTERNAL = 0x0U,   /**< Fosc/4, T0CS = 0. */
    TIMER0_CLOCK_EXTERNAL = 0x1U,   /**< RA4/T0CKI pin, T0CS = 1. */
} TIMER0_ClockSourceTypeDef;

/**
 * @brief Timer0 external-clock edge (OPTION_REG<T0SE>, DS39582B §5.2).
 *        Ignored in internal-clock mode.
 */
typedef enum {
    TIMER0_EDGE_RISING  = 0x0U,     /**< Increment on T0CKI rising edge. */
    TIMER0_EDGE_FALLING = 0x1U,     /**< Increment on T0CKI falling edge. */
} TIMER0_ClockEdgeTypeDef;

/**
 * @brief Timer0 prescaler ratio, loaded into OPTION_REG<PS2:PS0>
 *        (DS39582B Table 5-1). 000 is 1:2, NOT 1:1; "no prescaler"
 *        means PSA=1 (prescaler assigned to WDT instead).
 */
typedef enum {
    TIMER0_PRESCALER_1_2    = 0x0U,  /**< 1:2, PS2:PS0 = 000. */
    TIMER0_PRESCALER_1_4    = 0x1U,  /**< 1:4, PS2:PS0 = 001. */
    TIMER0_PRESCALER_1_8    = 0x2U,  /**< 1:8, PS2:PS0 = 010. */
    TIMER0_PRESCALER_1_16   = 0x3U,  /**< 1:16, PS2:PS0 = 011. */
    TIMER0_PRESCALER_1_32   = 0x4U,  /**< 1:32, PS2:PS0 = 100. */
    TIMER0_PRESCALER_1_64   = 0x5U,  /**< 1:64, PS2:PS0 = 101. */
    TIMER0_PRESCALER_1_128  = 0x6U,  /**< 1:128, PS2:PS0 = 110. */
    TIMER0_PRESCALER_1_256  = 0x7U,  /**< 1:256, PS2:PS0 = 111. */
} TIMER0_PrescalerTypeDef;

/**
 * @brief  Driver handle (Cube-style).
 */
typedef struct {
    TIMER0_ClockSourceTypeDef  ClockSource;     /**< Internal or T0CKI. */
    TIMER0_ClockEdgeTypeDef    ClockEdge;       /**< T0CKI edge (or rising). */
    TIMER0_PrescalerTypeDef    Prescaler;       /**< 1:2..1:256. */
    bool                       PrescalerAssigned; /**< true = prescaler → TMR0. */
    uint8_t                    ReloadValue;    /**< 0..255, start counting from here. */
    /** @brief  Optional overflow callback. Called from interrupt context
     *         on every TMR0 → 0x00 rollover. */
    void (*OverflowCallback)(void);
} TIMER0_HandleTypeDef;

/**
 * @brief  Default initialiser: internal Fosc/4, prescaler 1:256, no callback.
 */
#define TIMER0_HANDLE_DEFAULT {                                         \
    .ClockSource        = TIMER0_CLOCK_INTERNAL,                        \
    .ClockEdge          = TIMER0_EDGE_RISING,                           \
    .Prescaler          = TIMER0_PRESCALER_1_256,                       \
    .PrescalerAssigned  = true,                                         \
    .ReloadValue        = 0x00U,                                        \
    .OverflowCallback   = NULL,                                         \
}

/**
 * @brief  Configure Timer0 from the handle. Programs OPTION_REG and
 *         INTCON<TMR0IE>. Does not start the timer, call @ref
 *         EPIC_TIMER0_Start afterwards.
 *
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void);

/**
 * @brief  Timer0 weak ISR. Forward-declared so user code can override it
 *         (Cube-style). When the user provides an `OverflowCallback`
 *         through `EPIC_TIMER0_Init`, the default implementation invokes
 *         it; otherwise it just clears TMR0IF and returns.
 */
void TIMER0_IRQHandler(void) EPIC_WEAK;

/**
 * @brief  Enable TMR0 counting. Sets OPTION_REG<T0CS> accordingly and
 *         writes `h->ReloadValue` into TMR0.
 *
 *         Note: writing TMR0 clears the prescaler (DS39582B §5.3 Note).
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h);

/** Disable TMR0 counting. Clears OPTION_REG<T0CS> → TMR0 halted. */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);

/** Read the current counter value. */
uint8_t EPIC_TIMER0_ReadCounter(void);

/** Write `value` to the counter (also clears the prescaler). */
void EPIC_TIMER0_WriteCounter(uint8_t value);

/**
 * @brief  Convert a prescaler enum to its integer ratio (1, 2, 4, ..., 256).
 *         Used by callers that need the ratio to compute overflow periods.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);

#endif /* PIC16F87XA_TIMER0_H */
