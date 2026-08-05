/**
 * @file    peripherals/pic16f87xa_timer2.h
 * @brief   Timer2 driver, 8-bit timer with PR2 period register and postscaler.
 *
 * @details
 *   Source: DS39582B §7.0. Full reference: MANUAL.md §12. TMR2 resets
 *   to 0 on TMR2==PR2 (never reaches PR2+1). TMR2IF fires every
 *   prescaler x postscaler x (PR2+1) cycles and drives the CCP PWM
 *   time base.
 */

#ifndef PIC16F87XA_TIMER2_H
#define PIC16F87XA_TIMER2_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief Prescaler ratio (T2CON<T2CKPS1:T2CKPS0>, DS39582B §7.0, Reg 7-1).
 */
typedef enum {
    TIMER2_PRESCALER_1_1  = 0x0U,    /**< 00. */
    TIMER2_PRESCALER_1_4  = 0x1U,    /**< 01. */
    TIMER2_PRESCALER_1_16 = 0x2U,    /**< 1x. */
} TIMER2_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T2CON<TOUTPS3:TOUTPS0>, DS39582B §7.0).
 *        Postscaler value N → 1:(N+1) divider.
 */
typedef enum {
    TIMER2_POSTSCALER_1_1  = 0x0U,
    TIMER2_POSTSCALER_1_2  = 0x1U,
    TIMER2_POSTSCALER_1_3  = 0x2U,
    TIMER2_POSTSCALER_1_4  = 0x3U,
    TIMER2_POSTSCALER_1_5  = 0x4U,
    TIMER2_POSTSCALER_1_6  = 0x5U,
    TIMER2_POSTSCALER_1_7  = 0x6U,
    TIMER2_POSTSCALER_1_8  = 0x7U,
    TIMER2_POSTSCALER_1_9  = 0x8U,
    TIMER2_POSTSCALER_1_10 = 0x9U,
    TIMER2_POSTSCALER_1_11 = 0xAU,
    TIMER2_POSTSCALER_1_12 = 0xBU,
    TIMER2_POSTSCALER_1_13 = 0xCU,
    TIMER2_POSTSCALER_1_14 = 0xDU,
    TIMER2_POSTSCALER_1_15 = 0xEU,
    TIMER2_POSTSCALER_1_16 = 0xFU,
} TIMER2_PostscalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER2_PrescalerTypeDef    Prescaler;
    TIMER2_PostscalerTypeDef   Postscaler;
    uint8_t                    Period;       /**< PR2 value, 0..255. */
    /** @brief Optional overflow callback (fires on TMR2IF, i.e. every
     *         prescaler × (PR2+1) × postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

#define TIMER2_HANDLE_DEFAULT {                                         \
    .Prescaler       = TIMER2_PRESCALER_1_1,                            \
    .Postscaler      = TIMER2_POSTSCALER_1_1,                           \
    .Period          = 0xFFU,                                           \
    .OverflowCallback = NULL,                                           \
}

EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER2_Stop(void);

uint8_t  EPIC_TIMER2_ReadCounter(void);
void     EPIC_TIMER2_WriteCounter(uint8_t value);

uint8_t  EPIC_TIMER2_ReadPeriod(void);
void     EPIC_TIMER2_WritePeriod(uint8_t period);

uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);

/** Weak Timer2 ISR, override in user code to add application logic. */
void TIMER2_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F87XA_TIMER2_H */
