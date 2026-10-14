/**
 * PIC16F1508/1509 Timer2 driver (DS40001609E §20.0): 8-bit timer with PR2
 * period match and postscaler. TMR2 counts 0..PR2 and resets to 0 on the
 * cycle it matches PR2; TMR2IF fires every prescaler x (PR2+1) x postscaler
 * input cycles. The match output (T2_match) also feeds the CLC data inputs
 * (Table 24-1, D1 code 7 and D2 code 3) and the PWM module, so a free-running
 * TMR2 is the slow clock the CLC debouncer and the beat dimmer build on.
 */
#ifndef PIC16F1508_TIMER2_H
#define PIC16F1508_TIMER2_H

#include <stdint.h>
#include "pic16f1508.h"

/**
 * @brief Prescaler ratio (T2CON<T2CKPS1:T2CKPS0>, DS40001609E Register 20-1).
 *        All four encodings are distinct on this part: 00=1:1, 01=1:4,
 *        10=1:16, 11=1:64.
 */
typedef enum {
    TIMER2_PRESCALER_1_1  = 0x0U,
    TIMER2_PRESCALER_1_4  = 0x1U,
    TIMER2_PRESCALER_1_16 = 0x2U,
    TIMER2_PRESCALER_1_64 = 0x3U,
} TIMER2_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T2CON<T2OUTPS3:T2OUTPS0>, DS40001609E
 *        Register 20-1). Postscaler value N gives a 1:(N+1) divider.
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
    uint8_t                    Period;   /**< PR2 value, 0..255. */
    /** @brief Optional match callback (fires on TMR2IF, i.e. every
     *         prescaler x (PR2+1) x postscaler input cycles). */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

#define TIMER2_HANDLE_DEFAULT {                                         \
    .Prescaler        = TIMER2_PRESCALER_1_1,                            \
    .Postscaler       = TIMER2_POSTSCALER_1_1,                           \
    .Period           = 0xFFU,                                           \
    .OverflowCallback = NULL,                                           \
}

/* The ISR's owned callback slot, defined in the driver body. */
extern void (*g_t2_overflow_cb)(void);

/**
 * @brief Configure Timer2 from the handle: halt the timer, clear TMR2IF,
 * and enable or disable the TMR2 interrupt from OverflowCallback.
 * @details Does not start the timer. A set callback also sets INTCON.PEIE,
 * since PIE1 sources reach the vector only through PEIE. GIE is left alone.
 * @param h Handle with prescaler, postscaler, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID if h is NULL or a ratio is out of range.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h);

/**
 * @brief Disable the TMR2 interrupt, clear TMR2IF, restore T2CON to POR,
 * write PR2 back to 0xFF and drop the callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void);

/**
 * @brief Start Timer2: write PR2 before enabling TMR2ON to avoid a
 * spurious first match, zero TMR2 and program T2CON in one write.
 * @param h Handle holding the period, prescaler and postscaler.
 * @return EPIC_OK on success, EPIC_INVALID if h is NULL or a ratio is out of range.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h);

/**
 * @brief Stop Timer2 by clearing T2CON<TMR2ON>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Stop(void);

/**
 * @brief Read the current TMR2 counter value.
 * @return TMR2 value, 0..255.
 */
uint8_t EPIC_TIMER2_ReadCounter(void);

/**
 * @brief Write the TMR2 counter value.
 * @details A write also clears the prescaler and postscaler counters
 * (DS40001609E §20.1).
 * @param value Counter value, 0..255.
 */
void EPIC_TIMER2_WriteCounter(uint8_t value);

/**
 * @brief Read the PR2 period register.
 * @return PR2 value, 0..255.
 */
uint8_t EPIC_TIMER2_ReadPeriod(void);

/**
 * @brief Write the PR2 period register.
 * @param period Period value, 0..255.
 */
void EPIC_TIMER2_WritePeriod(uint8_t period);

/**
 * @brief Convert a prescaler enum to its integer ratio (1, 4, 16, 64).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);

/**
 * @brief Convert a postscaler enum to its integer ratio (1..16, linear).
 * @param p Postscaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);

/**
 * @brief Timer2 ISR: clears TMR2IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR2IF and TMR2IE
 * are both set.
 */
void TIMER2_IRQHandler(void);

#endif
