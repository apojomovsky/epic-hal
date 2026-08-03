/**
 * @file    peripherals/pic18fxx5x_timer3.h
 * @brief   Timer3 driver, 16-bit timer/counter.
 *
 * @details
 *   A second 16-bit timer/counter alongside Timer1 (DS39632E §14.0),
 *   mirroring `pic18fxx5x_timer1.h`'s API. Shares Timer1's T1OSC (no
 *   T3OSCEN of its own); overflow sets PIR2<TMR3IF>. T3CON<T3CCP2:T3CCP1>
 *   selects Timer1 vs Timer3 as the CCP1/CCP2 capture/compare source, left
 *   at reset (Timer1) here, the CCP/ECCP driver manages it when needed.
 */

#ifndef PIC18FXX5X_TIMER3_H
#define PIC18FXX5X_TIMER3_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief Timer3 clock source (T3CON<TMR3CS>, DS39632E Register 14-1).
 */
typedef enum {
    TIMER3_CLOCK_INTERNAL  = 0x0U,   /**< Fosc/4 (timer mode). */
    TIMER3_CLOCK_EXTERNAL  = 0x1U,   /**< External pin or T1OSC (shared w/ Timer1). */
} TIMER3_ClockSourceTypeDef;

/**
 * @brief External-clock synchronisation (T3CON<T3SYNC>, DS39632E §14.0).
 *        Ignored in timer mode (internal clock).
 */
typedef enum {
    TIMER3_SYNC_EXTERNAL   = 0x0U,   /**< T3SYNC = 0, synchronise to Fosc. */
    TIMER3_ASYNC_EXTERNAL  = 0x1U,   /**< T3SYNC = 1, free-running. */
} TIMER3_ClockSyncTypeDef;

/**
 * @brief Prescaler ratio (T3CON<T3CKPS1:T3CKPS0>, DS39632E Register 14-1).
 */
typedef enum {
    TIMER3_PRESCALER_1_1 = 0x0U,    /**< 1:1, 00. */
    TIMER3_PRESCALER_1_2 = 0x1U,    /**< 1:2, 01. */
    TIMER3_PRESCALER_1_4 = 0x2U,    /**< 1:4, 10. */
    TIMER3_PRESCALER_1_8 = 0x3U,    /**< 1:8, 11. */
} TIMER3_PrescalerTypeDef;

/** Driver handle (Cube-style). No oscillator field: Timer3 shares Timer1's
 *  T1OSC (enable it via the Timer1 driver if needed). */
typedef struct {
    TIMER3_ClockSourceTypeDef  ClockSource;
    TIMER3_ClockSyncTypeDef    ClockSync;
    TIMER3_PrescalerTypeDef    Prescaler;
    uint16_t                   ReloadValue;   /**< 16-bit initial counter. */
    /** @brief Optional overflow callback (fires on PIR2<TMR3IF>). */
    void (*OverflowCallback)(void);
} TIMER3_HandleTypeDef;

#define TIMER3_HANDLE_DEFAULT {                                         \
    .ClockSource      = TIMER3_CLOCK_INTERNAL,                          \
    .ClockSync        = TIMER3_SYNC_EXTERNAL,                           \
    .Prescaler        = TIMER3_PRESCALER_1_1,                            \
    .ReloadValue      = 0x0000U,                                        \
    .OverflowCallback = NULL,                                            \
}

HAL_StatusTypeDef HAL_TIMER3_Init(const TIMER3_HandleTypeDef *h);
HAL_StatusTypeDef HAL_TIMER3_DeInit(void);
HAL_StatusTypeDef HAL_TIMER3_Start(const TIMER3_HandleTypeDef *h);
HAL_StatusTypeDef HAL_TIMER3_Stop(void);

/** Atomically read the 16-bit counter (RD16 latches TMR3H on TMR3L read). */
uint16_t HAL_TIMER3_ReadCounter(void);

/** Atomically write the 16-bit counter (RD16 latches TMR3H on TMR3L write). */
void HAL_TIMER3_WriteCounter(uint16_t value);

/** Convert a prescaler enum to its integer ratio (1, 2, 4, 8). */
uint16_t HAL_TIMER3_PrescalerToRatio(TIMER3_PrescalerTypeDef p);

/** Weak Timer3 ISR, override in user code to add application logic. */
void TIMER3_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_TIMER3_H */
