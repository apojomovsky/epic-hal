/**
 * PIC16F193X Timer1 driver (DS41364B §16.0): 16-bit timer/counter.
 * T1CON (Register 16-1) holds the clock source, prescaler, and on bit;
 * T1GCON (Register 16-2) is the gate control, out of scope this phase.
 * The CCP special-event trigger (§19.0) can reset TMR1H:L; configured
 * by the CCP driver, not here. All register accesses use literal
 * PIC_REG_* tokens; XC8 auto-banks on this core. Full reference:
 * MANUAL.md §11.
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

/**
 * @brief  Configure Timer1 from the handle: halt the timer, clear
 *         TMR1IF and enable/disable the TMR1 interrupt according to the
 *         OverflowCallback. Does not start the timer.
 *
 * @param  h  handle with clock source, prescaler, reload and callback
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the clock
 *         source is not TIMER1_CLOCK_INTERNAL
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h);

/**
 * @brief  Disable the TMR1 interrupt, clear TMR1IF, restore T1CON to
 *         its POR value and clear TMR1H:TMR1L.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void);

/**
 * @brief  Start Timer1: write `h->ReloadValue` to the counter and
 *         program T1CON (prescaler plus TMR1ON) in one write.
 *
 * @param  h  handle holding the reload value and prescaler
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or the clock
 *         source is not TIMER1_CLOCK_INTERNAL
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h);

/**
 * @brief  Stop Timer1 by clearing T1CON<TMR1ON>.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void);

/**
 * @brief  Atomically read the 16-bit counter value.
 *
 * @return The current TMR1H:TMR1L value, 0..65535
 */
uint16_t EPIC_TIMER1_ReadCounter(void);

/**
 * @brief  Atomically write the 16-bit counter value.
 *
 * @param  value  counter value 0..65535 (high byte written first)
 */
void EPIC_TIMER1_WriteCounter(uint16_t value);

/**
 * @brief  Convert a prescaler enum to its integer ratio (1, 2, 4, 8).
 *
 * @param  p  one of @ref TIMER1_PrescalerTypeDef
 * @return The prescaler divider ratio, or 1 for an invalid enum value
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);

/**
 * @brief  Weak Timer1 ISR, override in user code to add application
 *         logic.
 */
void TIMER1_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_TIMER1_H */
