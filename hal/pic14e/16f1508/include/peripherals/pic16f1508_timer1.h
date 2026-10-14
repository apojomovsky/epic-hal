/**
 * PIC16F1508/1509 Timer1 driver (DS40001609E §19.0): 16-bit timer/counter.
 * Covers the free-running timer path (FOSC/4) and the external T1CKI counter
 * the quadrature assist feeds; the gate stays at POR. T1_overflow feeds the
 * CLC data inputs (Table 24-1, D1 code 6 and D2 code 2), so a preloaded TMR1
 * is the beat dimmer second pulse source: 255 to overflow vs TMR2's 256.
 */
#ifndef PIC16F1508_TIMER1_H
#define PIC16F1508_TIMER1_H

#include <stdint.h>
#include "pic16f1508.h"

/**
 * @brief Timer1 clock source (T1CON<TMR1CS1:TMR1CS0>, DS40001609E
 *        Register 19-1).
 */
typedef enum {
    TIMER1_CLOCK_INTERNAL = 0x0U, /**< FOSC/4 (timer mode). */
    TIMER1_CLOCK_EXTERNAL = 0x2U, /**< T1CKI pin rising edge (counter mode). */
} TIMER1_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T1CON<T1CKPS1:T1CKPS0>, DS40001609E Register 19-1).
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
    .OverflowCallback = NULL,                                          \
}

/* The ISR's owned callback slot, defined in the driver body. */
extern void (*g_t1_overflow_cb)(void);

/**
 * @brief Configure Timer1 from the handle: halt the timer, clear TMR1IF,
 * and enable or disable the TMR1 interrupt from OverflowCallback.
 * @details Does not start the timer. A set callback also sets INTCON.PEIE,
 * since PIE1 sources reach the vector only through PEIE. GIE is left alone.
 * External clock selects the T1CKI pin (T1OSCEN kept clear) with the input
 * unsynchronized (T1SYNC set), the counter-mode pairing of Register 19-1.
 * @param h Handle with clock source, prescaler, reload and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle or a clock
 * source or prescaler out of range.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h);

/**
 * @brief Disable the TMR1 interrupt, clear TMR1IF, restore T1CON to POR,
 * zero the counter and drop the callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void);

/**
 * @brief Start Timer1: write ReloadValue to the counter, then program
 * T1CON (clock source, prescaler, TMR1ON) in one write.
 * @param h Handle holding the reload value, clock source and prescaler.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle or a clock
 * source or prescaler out of range.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h);

/**
 * @brief Stop Timer1 by clearing T1CON<TMR1ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void);

/**
 * @brief Read the 16-bit counter value without tearing on overflow.
 * @return TMR1H:TMR1L value, 0..65535.
 */
uint16_t EPIC_TIMER1_ReadCounter(void);

/**
 * @brief Write the 16-bit counter value, high byte first.
 * @param value Counter value, 0..65535.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value);

/**
 * @brief Convert a prescaler enum to its integer ratio (1, 2, 4, 8).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);

/**
 * @brief Timer1 ISR: clears TMR1IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR1IF and TMR1IE
 * are both set.
 */
void TIMER1_IRQHandler(void);

#endif
