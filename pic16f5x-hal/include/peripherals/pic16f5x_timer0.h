/* PIC16F5x Timer0 driver (DS41213D §5.0): 8-bit timer/counter with a
 * shared prescaler. Configured through the control-space OPTION
 * register (T0CS clock source, T0SE edge, PSA prescaler assignment,
 * PS<2:0> ratio), written via the `option` instruction (EPIC_OPTION_WRITE).
 * Writing TMR0 clears the prescaler (§5.0). The prescaler is shared
 * with the WDT; PSA=1 assigns it to the WDT and Timer0 runs at 1:1.
 *
 * This core has NO interrupt path: there is no INTCON and no TMR0IF/
 * TMR0IE on the die (DS41213D §4.0), so the driver is polled and has
 * no OverflowCallback, unlike the 14-bit families' Timer0 API. */

#ifndef PIC16F5X_TIMER0_H
#define PIC16F5X_TIMER0_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"

/**
 * @brief Timer0 clock source (OPTION<T0CS>, DS41213D Register 5-1).
 */
typedef enum {
    TIMER0_CLOCK_INTERNAL = 0x0U,   /**< Fosc/4, T0CS = 0. */
    TIMER0_CLOCK_EXTERNAL = 0x1U,   /**< T0CKI pin, T0CS = 1. */
} TIMER0_ClockSourceTypeDef;

/**
 * @brief Timer0 external-clock edge (OPTION<T0SE>, DS41213D §5.0).
 *        Ignored in internal-clock mode.
 */
typedef enum {
    TIMER0_EDGE_RISING  = 0x0U,     /**< Increment on T0CKI rising edge. */
    TIMER0_EDGE_FALLING = 0x1U,     /**< Increment on T0CKI falling edge. */
} TIMER0_ClockEdgeTypeDef;

/**
 * @brief Timer0 prescaler ratio, loaded into OPTION<PS2:PS0>
 *        (DS41213D Register 5-1). 000 is 1:2, NOT 1:1; "no prescaler"
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

/** Driver handle (Cube-style). No callback field: no interrupt exists. */
typedef struct {
    TIMER0_ClockSourceTypeDef  ClockSource;        /**< Internal or T0CKI. */
    TIMER0_ClockEdgeTypeDef    ClockEdge;          /**< T0CKI edge (or rising). */
    TIMER0_PrescalerTypeDef    Prescaler;          /**< 1:2..1:256. */
    bool                       PrescalerAssigned;  /**< true = prescaler -> TMR0. */
    uint8_t                    ReloadValue;        /**< 0..255, start from here. */
} TIMER0_HandleTypeDef;

/** Default initialiser: internal Fosc/4, prescaler 1:256. */
#define TIMER0_HANDLE_DEFAULT {                                         \
    .ClockSource        = TIMER0_CLOCK_INTERNAL,                        \
    .ClockEdge          = TIMER0_EDGE_RISING,                           \
    .Prescaler          = TIMER0_PRESCALER_1_256,                       \
    .PrescalerAssigned  = true,                                         \
    .ReloadValue        = 0x00U,                                        \
}

/* Last OPTION byte written by the driver, so Start/Stop can flip
 * T0CS without a read (OPTION is write-only on this core, DS41213D
 * Table 12-1). Defined in the driver body; declared here so the
 * inlined Init stores to it from any TU. */
extern uint8_t pic16f5x_t0_option_shadow;

/**
 * @brief  Configure Timer0 from the handle: programs the OPTION
 *         control register (clock source, edge, prescaler) and TMR0.
 *         No interrupt bits exist to touch. Does not start the timer,
 *         call @ref EPIC_TIMER0_Start afterwards.
 * @param  h  handle with clock source, edge, prescaler.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
static inline EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;

    uint8_t opt = PIC_OPTION_POR_VALUE;
    opt = (uint8_t)((opt & (uint8_t)~(PIC_OPTION_T0CS | PIC_OPTION_T0SE)) |
                    ((uint8_t)h->ClockSource << 5) |
                    ((uint8_t)h->ClockEdge << 4));
    opt = (uint8_t)((opt & (uint8_t)~(PIC_OPTION_PSA | PIC_OPTION_PS_MASK)) |
                    ((uint8_t)(!h->PrescalerAssigned) << 3) |
                    ((uint8_t)h->Prescaler & PIC_OPTION_PS_MASK));
    pic16f5x_t0_option_shadow = opt;
    EPIC_OPTION_WRITE(opt);

    EPIC_REG8(PIC_REG_TMR0) = h->ReloadValue;
    return EPIC_OK;
}

/**
 * @brief  Disable Timer0 counting (clears OPTION<T0CS>) and reset
 *         TMR0 to zero. No interrupt bits to clear on this core.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void);

/**
 * @brief Start Timer0 counting from internal Fosc/4 (clears T0CS). The
 *        handle is accepted for the shared contract shape; the driver
 *        needs no per-call config (the OPTION byte was fully written
 *        by Init).
 * @param h the handle passed to @ref EPIC_TIMER0_Init (may be NULL).
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h);

/**
 * @brief Stop Timer0 counting by clearing T0CS.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);

/**
 * @brief Read the current TMR0 value.
 * @return the 8-bit counter value.
 */
uint8_t EPIC_TIMER0_ReadCounter(void);

/**
 * @brief Write the TMR0 counter (also clears the prescaler).
 * @param value the 8-bit value to load.
 */
void EPIC_TIMER0_WriteCounter(uint8_t value);

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p the prescaler enum value.
 * @return the ratio (2..256), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);

#endif /* PIC16F5X_TIMER0_H */
