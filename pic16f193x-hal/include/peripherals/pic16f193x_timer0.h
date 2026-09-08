/**
 * PIC16F193X Timer0 driver (DS41364B §15.0): 8-bit timer/counter with
 * shared prescaler. Timer0 is configured through OPTION_REG (Register
 * 2-2): T0CS (clock source), T0SE (edge), PSA (prescaler assignment),
 * PS<2:0> (ratio). Writing TMR0 clears the prescaler (§15.0). The
 * prescaler is shared with the WDT; PSA=1 assigns it to the WDT and
 * Timer0 runs at 1:1. Interrupt flag/enable are INTCON<TMR0IF>/<TMR0IE>.
 */

#ifndef PIC16F193X_TIMER0_H
#define PIC16F193X_TIMER0_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "core/pic16f193x_irq.h"

/**
 * @brief Timer0 clock source (OPTION_REG<T0CS>, DS41364B §15.0, Reg 2-2).
 */
typedef enum {
    TIMER0_CLOCK_INTERNAL = 0x0U,   /**< Fosc/4, T0CS = 0. */
    TIMER0_CLOCK_EXTERNAL = 0x1U,   /**< RA4/T0CKI pin, T0CS = 1. */
} TIMER0_ClockSourceTypeDef;

/**
 * @brief Timer0 external-clock edge (OPTION_REG<T0SE>, DS41364B §15.0).
 *        Ignored in internal-clock mode.
 */
typedef enum {
    TIMER0_EDGE_RISING  = 0x0U,     /**< Increment on T0CKI rising edge. */
    TIMER0_EDGE_FALLING = 0x1U,     /**< Increment on T0CKI falling edge. */
} TIMER0_ClockEdgeTypeDef;

/**
 * @brief Timer0 prescaler ratio, loaded into OPTION_REG<PS2:PS0>
 *        (DS41364B Register 2-2). 000 is 1:2, NOT 1:1; "no prescaler"
 *        means PSA=1 (prescaler assigned to WDT, Timer0 at 1:1).
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

/** Driver handle (Cube-style). */
typedef struct {
    TIMER0_ClockSourceTypeDef  ClockSource;        /**< Internal or T0CKI. */
    TIMER0_ClockEdgeTypeDef    ClockEdge;          /**< T0CKI edge (or rising). */
    TIMER0_PrescalerTypeDef    Prescaler;          /**< 1:2..1:256. */
    bool                       PrescalerAssigned;  /**< true = prescaler -> TMR0. */
    uint8_t                    ReloadValue;        /**< 0..255, start from here. */
    /** Optional overflow callback, called from interrupt context. */
    void (*OverflowCallback)(void);
} TIMER0_HandleTypeDef;

/** Default initialiser: internal Fosc/4, prescaler 1:256, no callback. */
#define TIMER0_HANDLE_DEFAULT {                                         \
    .ClockSource        = TIMER0_CLOCK_INTERNAL,                        \
    .ClockEdge          = TIMER0_EDGE_RISING,                           \
    .Prescaler          = TIMER0_PRESCALER_1_256,                       \
    .PrescalerAssigned  = true,                                         \
    .ReloadValue        = 0x00U,                                        \
    .OverflowCallback   = NULL,                                          \
}
/* The ISR's owned handle storage, defined in the driver body. Declared
 * here so the inlined Init (below) can store to it from any TU. */
extern TIMER0_HandleTypeDef g_t0_storage;

/**
 * @brief  Configure Timer0 from the handle. Programs OPTION_REG and
 *         INTCON<TMR0IE>. Does not start the timer, call @ref
 *         EPIC_TIMER0_Start afterwards.
 *
 *         Static inline so the callback store lands in the caller's
 *         translation unit: epic-cc resolves a cross-context indirect
 *         call only when the stored value is a named function literal
 *         (ADR-024, the shape pic16f87xa_timer0.h uses). Out-of-line,
 *         the call site's candidate set comes back empty and the call
 *         traps.
 *
 * @param  h  handle with clock source, edge, prescaler, callback
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* Stop the timer before reconfiguring: clear T0CS (DS41364B §15.0). */
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);

    /* Clear TMR0IF; configure TMR0IE if a callback is provided. */
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR0);
    if (h->OverflowCallback) {
        EPIC_IRQ_Enable(PIC16F193X_IRQ_TMR0);
    } else {
        EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR0);
    }

    g_t0_storage.ClockSource       = h->ClockSource;
    g_t0_storage.ClockEdge         = h->ClockEdge;
    g_t0_storage.Prescaler         = h->Prescaler;
    g_t0_storage.PrescalerAssigned = h->PrescalerAssigned;
    g_t0_storage.ReloadValue       = h->ReloadValue;
    g_t0_storage.OverflowCallback  = h->OverflowCallback;
    return EPIC_OK;
}

/**
 * @brief  Disable the Timer0 interrupt, clear TMR0IF, halt the timer
 *         (clears OPTION_REG<T0CS>) and zero TMR0.
 *
 * @return EPIC_OK
 */
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
 *         Note: writing TMR0 clears the prescaler (DS41364B §15.0).
 *
 *         Static inline for the same reason as Init (ADR-024): with
 *         the handle local to the caller, clang folds every field
 *         load.
 *
 * @param  h  handle holding the reload value, prescaler and clock
 *            source/edge programmed in @ref EPIC_TIMER0_Init
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL
 */
static inline EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    /* DS41364B §15.0: writing TMR0 when the prescaler is assigned to
     * Timer0 clears the prescaler. Reload before re-enabling. */
    EPIC_REG8(PIC_REG_TMR0) = h->ReloadValue;

    /* Program prescaler assignment + ratio + clock source + edge in one
     * atomic read-modify-write. WPUEN and INTEDG are left untouched. */
    uint8_t set_mask = (uint8_t)(h->Prescaler & PIC_OPTION_PS_MASK);
    if (!h->PrescalerAssigned) set_mask |= PIC_OPTION_PSA;
    if (h->ClockSource == TIMER0_CLOCK_EXTERNAL) set_mask |= PIC_OPTION_T0CS;
    if (h->ClockEdge   == TIMER0_EDGE_FALLING)  set_mask |= PIC_OPTION_T0SE;

    uint8_t clr_mask = (uint8_t)(PIC_OPTION_PS_MASK | PIC_OPTION_PSA |
                                 PIC_OPTION_T0CS  | PIC_OPTION_T0SE);
    uint8_t opt = EPIC_REG8(PIC_REG_OPTION);
    opt = (uint8_t)((opt & (uint8_t)~clr_mask) | set_mask);
    EPIC_REG8(PIC_REG_OPTION) = opt;

    return EPIC_OK;
}

/**
 * @brief  Disable TMR0 counting (clears OPTION_REG<T0CS>, Timer0 halted).
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);

/**
 * @brief  Read the current counter value.
 *
 * @return The current TMR0 value, 0..255
 */
uint8_t EPIC_TIMER0_ReadCounter(void);

/**
 * @brief  Write `value` to the counter (also clears the prescaler).
 *
 * @param  value  counter value 0..255
 */
void EPIC_TIMER0_WriteCounter(uint8_t value);

/**
 * @brief  Convert a prescaler enum to its integer ratio (2, 4, ..., 256).
 *
 * @param  p  one of @ref TIMER0_PrescalerTypeDef
 * @return The prescaler divider ratio, or 1 for an invalid enum value
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);

#endif /* PIC16F193X_TIMER0_H */
