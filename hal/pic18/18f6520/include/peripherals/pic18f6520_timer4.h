/*
 * Timer4 driver, 8-bit timer with PR4 period register and postscaler
 * (DS39609B §15.0, Register 15-1). Same shape as Timer2 (T4CON 0xF76,
 * PR4 0xF77, TMR4 0xF78); TMR4IF is PIR3<3>. The ECAN quads have no
 * Timer4 and carry CAN registers at those addresses: every call there
 * returns EPIC_INVALID (accessors read 0 / write nothing).
 */

#ifndef PIC18F6520_TIMER4_H
#define PIC18F6520_TIMER4_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"

/**
 * @brief Prescaler ratio (T4CON<T4CKPS1:T4CKPS0>, DS39609B Register 15-1).
 */
typedef enum {
    TIMER4_PRESCALER_1_1  = 0x0U,    /**< 00. */
    TIMER4_PRESCALER_1_4  = 0x1U,    /**< 01. */
    TIMER4_PRESCALER_1_16 = 0x2U,    /**< 1x. */
} TIMER4_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T4CON<T4OUTPS3:T4OUTPS0>, DS39609B Register 15-1).
 *        Postscaler value N -> 1:(N+1) divider.
 */
typedef enum {
    TIMER4_POSTSCALER_1_1  = 0x0U,
    TIMER4_POSTSCALER_1_2  = 0x1U,
    TIMER4_POSTSCALER_1_3  = 0x2U,
    TIMER4_POSTSCALER_1_4  = 0x3U,
    TIMER4_POSTSCALER_1_5  = 0x4U,
    TIMER4_POSTSCALER_1_6  = 0x5U,
    TIMER4_POSTSCALER_1_7  = 0x6U,
    TIMER4_POSTSCALER_1_8  = 0x7U,
    TIMER4_POSTSCALER_1_9  = 0x8U,
    TIMER4_POSTSCALER_1_10 = 0x9U,
    TIMER4_POSTSCALER_1_11 = 0xAU,
    TIMER4_POSTSCALER_1_12 = 0xBU,
    TIMER4_POSTSCALER_1_13 = 0xCU,
    TIMER4_POSTSCALER_1_14 = 0xDU,
    TIMER4_POSTSCALER_1_15 = 0xEU,
    TIMER4_POSTSCALER_1_16 = 0xFU,
} TIMER4_PostscalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER4_PrescalerTypeDef    Prescaler;
    TIMER4_PostscalerTypeDef   Postscaler;
    uint8_t                    Period;       /**< PR4 value, 0..255. */
    /** @brief Optional overflow callback (fires on TMR4IF, i.e. every
     *         prescaler x (PR4+1) x postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER4_HandleTypeDef;

#define TIMER4_HANDLE_DEFAULT {                                         \
    .Prescaler        = TIMER4_PRESCALER_1_1,                            \
    .Postscaler       = TIMER4_POSTSCALER_1_1,                           \
    .Period           = 0xFFU,                                           \
    .OverflowCallback = NULL,                                           \
}

/**
 * @brief  Configure Timer4 from the handle: stop the timer, clear TMR4IF
 *         and optionally enable the overflow interrupt.
 * @param h the Timer4 handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the part
 *         has no Timer4 (ECAN quads).
 */
EPIC_StatusTypeDef EPIC_TIMER4_Init(const TIMER4_HandleTypeDef *h);

/**
 * @brief  Disable Timer4 counting, clear TMR4IF and restore T4CON/PR4 to
 *         their power-on values.
 * @return EPIC_OK on success, EPIC_INVALID on parts without Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_DeInit(void);

/**
 * @brief  Enable Timer4 counting: loads PR4, then programs T4CON and
 *         sets TMR4ON.
 * @param h the handle used to configure the timer.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the part
 *         has no Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Start(const TIMER4_HandleTypeDef *h);

/**
 * @brief  Disable Timer4 counting. Clears T4CON<TMR4ON>.
 * @return EPIC_OK on success, EPIC_INVALID on parts without Timer4.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Stop(void);

/**
 * @brief  Read the current Timer4 counter value (TMR4).
 * @return the current 8-bit TMR4 value (0 on parts without Timer4).
 */
uint8_t  EPIC_TIMER4_ReadCounter(void);

/**
 * @brief  Write a new value into the Timer4 counter (TMR4).
 * @param value the 8-bit value to load into TMR4.
 */
void     EPIC_TIMER4_WriteCounter(uint8_t value);

/**
 * @brief  Read the Timer4 period register (PR4).
 * @return the current 8-bit PR4 value (0 on parts without Timer4).
 */
uint8_t  EPIC_TIMER4_ReadPeriod(void);

/**
 * @brief  Write a new period into PR4 (TMR4 matches -> TMR4IF).
 * @param period the 8-bit period value, 0..255.
 */
void     EPIC_TIMER4_WritePeriod(uint8_t period);

/**
 * @brief  Convert a prescaler enum to its integer ratio (1, 4, 16).
 * @param p the prescaler enum value.
 * @return the integer division ratio (1, 4 or 16).
 */
uint16_t EPIC_TIMER4_PrescalerToRatio(TIMER4_PrescalerTypeDef p);

/**
 * @brief  Convert a postscaler enum to its integer ratio (1..16).
 * @param p the postscaler enum value.
 * @return the integer division ratio (1..16).
 */
uint16_t EPIC_TIMER4_PostscalerToRatio(TIMER4_PostscalerTypeDef p);

/**
 * @brief Weak Timer4 ISR, override in user code to add application logic.
 */
void TIMER4_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F6520_TIMER4_H */
