/**
 * PIC16F193X Timer2/4/6 driver (DS41364B §17.0): three instances of the
 * same 8-bit timer with PR-match reset and postscaler. Unlike
 * Timer0/Timer1 (raw free-running overflow at 0xFF/0xFFFF), TMRx counts
 * 0..PRx and resets to 0 on the cycle it would exceed PRx (never reaches
 * PRx+1); TMRxIF fires once every prescaler x (PRx+1) x postscaler input
 * cycles. One driver covers all three instances; every SFR access
 * branches on the instance before touching any register (literal
 * PIC_REG_* tokens, the proven shape from pic18fxx5x_ccp.c). Full
 * reference: MANUAL.md.
 */

#ifndef PIC16F193X_TIMER246_H
#define PIC16F193X_TIMER246_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief Which Timer2/4/6 instance a handle or call refers to. Values
 *        equal the timer number for readability (mirrors
 *        CCP_InstanceTypeDef's CCP_INSTANCE_1/2 = 1/2 convention).
 */
typedef enum {
    TIMER246_INSTANCE_2 = 2,   /**< Timer2, TMR2/PR2/T2CON. */
    TIMER246_INSTANCE_4 = 4,   /**< Timer4, TMR4/PR4/T4CON. */
    TIMER246_INSTANCE_6 = 6,   /**< Timer6, TMR6/PR6/T6CON. */
} TIMER246_InstanceTypeDef;

/**
 * @brief Prescaler ratio (T*CON<T*CKPS1:T*CKPS0>, DS41364B §17.0).
 *        Only 3 of the 4 encodings are distinct: 00=1:1, 01=1:4,
 *        1x=1:16 (both 10 and 11 give 1:16). No separate enum value
 *        for 0x3; EPIC_TIMER246_PrescalerToRatio's lookup table has all
 *        4 entries, the enum only exposes the 3 distinct ratios.
 */
typedef enum {
    TIMER246_PRESCALER_1_1  = 0x0U,
    TIMER246_PRESCALER_1_4  = 0x1U,
    TIMER246_PRESCALER_1_16 = 0x2U,
} TIMER246_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T*CON<TOUTPS3:TOUTPS0>, DS41364B §17.0).
 *        Postscaler value N -> 1:(N+1) divider, linear, all 16
 *        encodings distinct.
 */
typedef enum {
    TIMER246_POSTSCALER_1_1  = 0x0U,
    TIMER246_POSTSCALER_1_2  = 0x1U,
    TIMER246_POSTSCALER_1_3  = 0x2U,
    TIMER246_POSTSCALER_1_4  = 0x3U,
    TIMER246_POSTSCALER_1_5  = 0x4U,
    TIMER246_POSTSCALER_1_6  = 0x5U,
    TIMER246_POSTSCALER_1_7  = 0x6U,
    TIMER246_POSTSCALER_1_8  = 0x7U,
    TIMER246_POSTSCALER_1_9  = 0x8U,
    TIMER246_POSTSCALER_1_10 = 0x9U,
    TIMER246_POSTSCALER_1_11 = 0xAU,
    TIMER246_POSTSCALER_1_12 = 0xBU,
    TIMER246_POSTSCALER_1_13 = 0xCU,
    TIMER246_POSTSCALER_1_14 = 0xDU,
    TIMER246_POSTSCALER_1_15 = 0xEU,
    TIMER246_POSTSCALER_1_16 = 0xFU,
} TIMER246_PostscalerTypeDef;

/** Driver handle (Cube-style). One handle per instance; the caller
 *  owns three of these if it uses all three timers. */
typedef struct {
    TIMER246_InstanceTypeDef    Instance;
    TIMER246_PrescalerTypeDef   Prescaler;
    TIMER246_PostscalerTypeDef  Postscaler;
    uint8_t                     Period;   /**< PRx value, 0..255. */
    /** @brief Optional overflow callback (fires on TMRxIF, i.e. every
     *         prescaler x (PRx+1) x postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER246_HandleTypeDef;

#define TIMER246_HANDLE_DEFAULT {                                      \
    .Instance         = TIMER246_INSTANCE_2,                           \
    .Prescaler        = TIMER246_PRESCALER_1_1,                        \
    .Postscaler       = TIMER246_POSTSCALER_1_1,                       \
    .Period           = 0xFFU,                                         \
    .OverflowCallback = NULL,                                          \
}

/**
 * @brief  Configure one Timer2/4/6 instance from the handle: halt the
 *         timer, clear the TMRxIF flag and enable/disable the TMRx
 *         interrupt according to the OverflowCallback. Does not start
 *         the timer.
 *
 * @param  h  handle with Instance, prescaler, postscaler and callback
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the
 *         instance is not Timer2/4/6
 */
EPIC_StatusTypeDef EPIC_TIMER246_Init(const TIMER246_HandleTypeDef *h);

/**
 * @brief  Disable the TMRx interrupt, clear its flag, restore T*CON to
 *         the POR value, zero TMRx, restore PRx to 0xFF and drop the
 *         stored handle for the instance.
 *
 * @param  inst  TIMER246_INSTANCE_2, _4 or _6
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_TIMER246_DeInit(TIMER246_InstanceTypeDef inst);

/**
 * @brief  Start the instance: write PRx before enabling TMRxON (to
 *         avoid a spurious first match), zero TMRx and program T*CON
 *         with postscaler, TMRxON and prescaler.
 *
 * @param  h  handle holding the instance, period, prescaler and
 *            postscaler
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the
 *         instance is not Timer2/4/6
 */
EPIC_StatusTypeDef EPIC_TIMER246_Start(const TIMER246_HandleTypeDef *h);

/**
 * @brief  Stop the instance by clearing T*CON<TMRxON>.
 *
 * @param  inst  TIMER246_INSTANCE_2, _4 or _6
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_TIMER246_Stop(TIMER246_InstanceTypeDef inst);

/**
 * @brief  Read the current counter value of the instance.
 *
 * @param  inst  TIMER246_INSTANCE_2, _4 or _6
 * @return The current TMRx value, 0..255
 */
uint8_t  EPIC_TIMER246_ReadCounter(TIMER246_InstanceTypeDef inst);

/**
 * @brief  Write the counter value of the instance.
 *
 * @param  inst   TIMER246_INSTANCE_2, _4 or _6
 * @param  value  counter value 0..255
 */
void     EPIC_TIMER246_WriteCounter(TIMER246_InstanceTypeDef inst, uint8_t value);

/**
 * @brief  Read the period register (PRx) of the instance.
 *
 * @param  inst  TIMER246_INSTANCE_2, _4 or _6
 * @return The current PRx value, 0..255
 */
uint8_t  EPIC_TIMER246_ReadPeriod(TIMER246_InstanceTypeDef inst);

/**
 * @brief  Write the period register (PRx) of the instance.
 *
 * @param  inst    TIMER246_INSTANCE_2, _4 or _6
 * @param  period  period value 0..255
 */
void     EPIC_TIMER246_WritePeriod(TIMER246_InstanceTypeDef inst, uint8_t period);

/**
 * @brief  Convert a prescaler enum to its integer ratio (1, 4, 16).
 *
 * @param  p  one of @ref TIMER246_PrescalerTypeDef
 * @return The prescaler divider ratio, or 1 for an invalid enum value
 */
uint16_t EPIC_TIMER246_PrescalerToRatio(TIMER246_PrescalerTypeDef p);

/**
 * @brief  Convert a postscaler enum to its integer ratio (1..16).
 *
 * @param  p  one of @ref TIMER246_PostscalerTypeDef
 * @return The postscaler divider ratio, or 1 for an invalid enum value
 */
uint16_t EPIC_TIMER246_PostscalerToRatio(TIMER246_PostscalerTypeDef p);

/** @brief Weak Timer2 ISR, override in user code. */
void TIMER2_IRQHandler(void) EPIC_WEAK;

/** @brief Weak Timer4 ISR, override in user code. */
void TIMER4_IRQHandler(void) EPIC_WEAK;

/** @brief Weak Timer6 ISR, override in user code. */
void TIMER6_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_TIMER246_H */
