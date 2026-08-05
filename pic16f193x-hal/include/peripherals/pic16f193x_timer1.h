/**
 * @file    peripherals/pic16f193x_timer1.h
 * @brief   Timer1 driver, 16-bit timer/counter.
 *
 * @details
 *   Source: DS41364B §16.0. Full reference: MANUAL.md §11. T1CON
 *   (DS41364B Register 16-1) holds the clock source, prescaler, and
 *   on bit; T1GCON (DS41364B Register 16-2) is the gate control,
 *   out of scope for this phase. The CCP special-event trigger
 *   (DS41364B §19.0) can reset TMR1H:L; configured by the CCP
 *   driver, not here.
 *
 *   All register accesses use literal PIC_REG_* tokens; XC8
 *   auto-banks on this core (plan §6 codegen probe clean).
 */

#ifndef PIC16F193X_TIMER1_H
#define PIC16F193X_TIMER1_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief Timer1 clock source (T1CON<TMR1CS1:TMR1CS0>, DS41364B
 *        Register 16-1).
 */
typedef enum {
    TIMER1_CLOCK_INTERNAL  = 0x0U,   /**< Fosc/4 (timer mode). */
    /** External pin or T1OSC. Not implemented this phase: EPIC_TIMER1_Init/
     *  Start return EPIC_INVALID for this value (MANUAL.md §11 "Not in
     *  this phase"). */
    TIMER1_CLOCK_EXTERNAL  = 0x1U,
} TIMER1_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T1CON<T1CKPS1:T1CKPS0>, DS41364B Register
 *        16-1). Implementer: transcribe the bit positions and
 *        ratio mapping from the datasheet, do not copy from
 *        pic16f87xa_timer1.h without verifying.
 */
typedef enum {
    TIMER1_PRESCALER_1_1 = 0x0U,
    TIMER1_PRESCALER_1_2 = 0x1U,
    TIMER1_PRESCALER_1_4 = 0x2U,
    TIMER1_PRESCALER_1_8 = 0x3U,
} TIMER1_PrescalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER1_ClockSourceTypeDef  ClockSource;
    TIMER1_PrescalerTypeDef    Prescaler;
    uint16_t                   ReloadValue;   /**< 16-bit initial counter. */
    /** @brief Optional overflow callback (fires on TMR1IF). */
    void (*OverflowCallback)(void);
} TIMER1_HandleTypeDef;

#define TIMER1_HANDLE_DEFAULT {                                         \
    .ClockSource      = TIMER1_CLOCK_INTERNAL,                          \
    .Prescaler        = TIMER1_PRESCALER_1_1,                           \
    .ReloadValue      = 0x0000U,                                        \
    .OverflowCallback = NULL,                                           \
}

EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void);
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void);

/** Atomically read the 16-bit counter value. */
uint16_t EPIC_TIMER1_ReadCounter(void);

/** Atomically write the 16-bit counter value. */
void EPIC_TIMER1_WriteCounter(uint16_t value);

/** Convert a prescaler enum to its integer ratio (1, 2, 4, 8). */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);

/** Weak Timer1 ISR, override in user code to add application logic. */
void TIMER1_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F193X_TIMER1_H */
