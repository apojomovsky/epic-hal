/**
 * PIC16F188xx dedicated PWM6 and PWM7 driver (DS40001802D §19, Registers
 * 19-1..19-3). Each module is a 10-bit PWM on Timer2, Timer4 or Timer6
 * (CCPTMRS1 P6TSEL/P7TSEL): PRx sets the period, the duty is latched when
 * TMRx matches PRx, and the timer must be clocked from FOSC/4. Unlike the
 * CCP PWM mode there is no capture or compare function and no interrupt.
 * Full reference: MANUAL.md.
 */
#ifndef PIC16F188XX_PWM_H
#define PIC16F188XX_PWM_H

#include <stdint.h>
#include "pic16f188xx.h"

/** @brief Which dedicated PWM module a handle or call refers to (the module number). */
typedef enum {
    PWM_INSTANCE_6 = 6,   /**< PWM6, PWM6DCH/L + PWM6CON. */
    PWM_INSTANCE_7 = 7,   /**< PWM7, PWM7DCH/L + PWM7CON. */
} PWM_InstanceTypeDef;

/**
 * @brief Time base (CCPTMRS1 P6TSEL/P7TSEL, DS40001802D Register 30-6).
 *        Only Timer2 has a driver yet; the others are programmed but
 *        nothing here starts them.
 */
typedef enum {
    PWM_TIMER_2 = 1U,   /**< 01: Timer2. */
    PWM_TIMER_4 = 2U,   /**< 10: Timer4. */
    PWM_TIMER_6 = 3U,   /**< 11: Timer6. */
} PWM_TimerTypeDef;

/** Driver handle (Cube-style). One handle per module. */
typedef struct {
    PWM_InstanceTypeDef Instance;
    PWM_TimerTypeDef    Timer;       /**< Time base, CCPTMRS1. */
    uint16_t            Duty;        /**< 10-bit duty in counts, 0..1023. */
    uint8_t             ActiveLow;   /**< PWMxCON.POL: nonzero inverts the output. */
} PWM_HandleTypeDef;

#define PWM_HANDLE_DEFAULT {                                              \
    .Instance  = PWM_INSTANCE_6,                                          \
    .Timer     = PWM_TIMER_2,                                             \
    .Duty      = 0U,                                                      \
    .ActiveLow = 0U,                                                      \
}

/**
 * @brief Configure a PWM module from the handle and enable it.
 * @details Follows the datasheet order (§19.1.9): the polarity is written
 * with the module disabled, then the time base and duty, and PWMxCON.EN
 * last. The timer is not touched: start Timer2 (or 4/6) from FOSC/4 with
 * PRx holding the period.
 * @param h Handle with module, time base, duty and polarity.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle,
 *         a time base of 0 or above 3, or a duty above 1023.
 */
EPIC_StatusTypeDef EPIC_PWM_Init(const PWM_HandleTypeDef *h);

/**
 * @brief Reset one PWM module to its POR state.
 * @details Zeroes PWMxCON, PWMxDCH and PWMxDCL and restores the module's
 * CCPTMRS1 field to Timer2.
 * @param inst Module, 6 or 7.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range module.
 */
EPIC_StatusTypeDef EPIC_PWM_DeInit(PWM_InstanceTypeDef inst);

/**
 * @brief Set the duty in 10-bit counts, effective at the next period.
 * @details The duty is 1:4 of a timer step: 100% is
 * EPIC_CCP_PWMPeriodCounts(PRx) counts or more.
 * @param inst Module, 6 or 7.
 * @param duty Duty in counts, 0..1023.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range module or duty above 1023.
 */
EPIC_StatusTypeDef EPIC_PWM_SetDuty(PWM_InstanceTypeDef inst, uint16_t duty);

/**
 * @brief Read PWMxCON.OUT, the output level.
 * @param inst Module, 6 or 7.
 * @return 1 when the output reads high, 0 when low or the module is out of range.
 */
uint8_t EPIC_PWM_GetOutput(PWM_InstanceTypeDef inst);

#endif
