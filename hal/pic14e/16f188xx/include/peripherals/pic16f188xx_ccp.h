/**
 * PIC16F188xx CCP1..CCP5 driver (DS40001802D §30, Registers 30-1..30-6).
 * Capture, compare and 10-bit PWM modes. Capture and compare count the
 * Timer1/3/5 couple the module is tied to; PWM uses the matching
 * Timer2/4/6 as its time base (period in PR2) and needs it clocked from
 * FOSC/4. CCPTMRS0/1 pick the couple. Full reference: MANUAL.md.
 */
#ifndef PIC16F188XX_CCP_H
#define PIC16F188XX_CCP_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/** @brief Which CCP instance a handle or call refers to (the instance number). */
typedef enum {
    CCP_INSTANCE_1 = 1,   /**< CCP1, CCPR1L/H + CCP1CON. */
    CCP_INSTANCE_2 = 2,   /**< CCP2, CCPR2L/H + CCP2CON. */
    CCP_INSTANCE_3 = 3,   /**< CCP3, CCPR3L/H + CCP3CON. */
    CCP_INSTANCE_4 = 4,   /**< CCP4, CCPR4L/H + CCP4CON. */
    CCP_INSTANCE_5 = 5,   /**< CCP5, CCPR5L/H + CCP5CON. */
} CCP_InstanceTypeDef;

/**
 * @brief CCPxCON<MODE3:MODE0> mode select (DS40001802D Register 30-1).
 *        Codes 1100..1110 are reserved and have no enumerator.
 */
typedef enum {
    CCP_MODE_OFF                     = 0x0U,  /**< 0000, module off (resets it). */
    CCP_MODE_COMPARE_TOGGLE_CLR_TMR  = 0x1U,  /**< 0001, toggle output on match, clear the timer. */
    CCP_MODE_COMPARE_TOGGLE          = 0x2U,  /**< 0010, toggle output on match. */
    CCP_MODE_CAPTURE_ANY_EDGE        = 0x3U,  /**< 0011, every edge. */
    CCP_MODE_CAPTURE_FALLING         = 0x4U,  /**< 0100, every falling edge. */
    CCP_MODE_CAPTURE_RISING          = 0x5U,  /**< 0101, every rising edge. */
    CCP_MODE_CAPTURE_4TH             = 0x6U,  /**< 0110, every 4th rising edge. */
    CCP_MODE_CAPTURE_16TH            = 0x7U,  /**< 0111, every 16th rising edge. */
    CCP_MODE_COMPARE_SET             = 0x8U,  /**< 1000, set output on match. */
    CCP_MODE_COMPARE_CLEAR           = 0x9U,  /**< 1001, clear output on match. */
    CCP_MODE_COMPARE_PULSE           = 0xAU,  /**< 1010, pulse the output 0-1-0 on match. */
    CCP_MODE_COMPARE_PULSE_CLR_TMR   = 0xBU,  /**< 1011, pulse the output on match, clear the timer. */
    CCP_MODE_PWM                     = 0xFU,  /**< 1111, PWM. */
} CCP_ModeTypeDef;

/**
 * @brief Timer couple (CCPTMRS C1TSEL..C5TSEL, DS40001802D Registers 30-5
 *        and 30-6). Capture and compare use the first timer of the couple,
 *        PWM the second. Only Timer1 and Timer2 have drivers yet; the
 *        other couples are programmed but nothing here starts their timers.
 */
typedef enum {
    CCP_TIMER_1_2 = 1U,   /**< 01: Timer1 (capture/compare) or Timer2 (PWM). */
    CCP_TIMER_3_4 = 2U,   /**< 10: Timer3 or Timer4. */
    CCP_TIMER_5_6 = 3U,   /**< 11: Timer5 or Timer6. */
} CCP_TimerTypeDef;

/**
 * @brief Capture trigger input (CCPxCAP<CTS2:CTS0>, DS40001802D Register
 *        30-2). The CCPxPPS pin is the reset default; the host sim reads
 *        only that row.
 */
typedef enum {
    CCP_CAPTURE_CCPPPS  = 0U,   /**< 000, the pin CCPxPPS selects. */
    CCP_CAPTURE_C1OUT   = 1U,   /**< 001, comparator 1 output (synchronized). */
    CCP_CAPTURE_C2OUT   = 2U,   /**< 010, comparator 2 output (synchronized). */
    CCP_CAPTURE_IOC     = 3U,   /**< 011, the interrupt-on-change event. */
    CCP_CAPTURE_LC1OUT  = 4U,   /**< 100, CLC1 output. */
    CCP_CAPTURE_LC2OUT  = 5U,   /**< 101, CLC2 output. */
    CCP_CAPTURE_LC3OUT  = 6U,   /**< 110, CLC3 output. */
    CCP_CAPTURE_LC4OUT  = 7U,   /**< 111, CLC4 output. */
} CCP_CaptureSourceTypeDef;

/** Driver handle (Cube-style). One handle per instance. */
typedef struct {
    CCP_InstanceTypeDef      Instance;
    CCP_ModeTypeDef          Mode;
    CCP_TimerTypeDef         Timer;          /**< Timer couple, CCPTMRS. */
    CCP_CaptureSourceTypeDef CaptureSource;  /**< CCPxCAP.CTS, capture modes. */
    uint16_t                 CompareValue;   /**< 16-bit compare value (compare modes). */
    uint16_t                 PWMDuty;        /**< 10-bit PWM duty, 0..1023 (PWM mode). */
    uint8_t                  PWMLeftAligned; /**< CCPxCON.FMT: nonzero keeps the duty in CCPRxH:CCPRxL<7:6>. */
    /** @brief Fires on CCPxIF (capture, compare match, or PWM period).
     *         NULL leaves the interrupt masked. */
    void (*EventCallback)(void);
} CCP_HandleTypeDef;

#define CCP_HANDLE_DEFAULT {                                              \
    .Instance       = CCP_INSTANCE_1,                                     \
    .Mode           = CCP_MODE_OFF,                                       \
    .Timer          = CCP_TIMER_1_2,                                      \
    .CaptureSource  = CCP_CAPTURE_CCPPPS,                                 \
    .CompareValue   = 0U,                                                 \
    .PWMDuty        = 0U,                                                 \
    .PWMLeftAligned = 0U,                                                 \
    .EventCallback  = NULL,                                               \
}

/**
 * @brief Configure a CCP module from the handle.
 * @details Clears CCPxCON first (module reset, capture prescaler cleared),
 * then programs the timer couple (CCPTMRS) and capture source (CCPxCAP),
 * loads CCPRxH:L (the compare value, or the PWM duty in the FMT
 * alignment), clears CCPxIF, arms the interrupt (plus PEIE) only with a
 * callback, and writes CCPxCON last with EN set for every mode but OFF.
 * GIE is left alone. The timers are not touched: capture and compare need
 * Timer1 running from FOSC/4 or an external clock, PWM needs Timer2 (or
 * 4/6) running from FOSC/4 with PR2 holding the period.
 * @param h Handle with instance, mode, timer couple, source, values and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle,
 *         a reserved mode, or a PWM duty above 1023.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);

/**
 * @brief Reset one CCP module to its POR state.
 * @details Masks the interrupt, clears the flag, zeroes CCPxCON and CCPxCAP,
 * restores the instance's CCPTMRS field to Timer1/Timer2 and drops the
 * callback.
 * @param inst Instance, 1..5.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);

/**
 * @brief Load a new 16-bit compare value, high byte first.
 * @param inst Instance, 1..5.
 * @param value Value compared against the timer.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance.
 */
EPIC_StatusTypeDef EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);

/**
 * @brief Change only the mode field, leaving CCPRx, FMT, CCPxCAP and the
 *        interrupt state alone.
 * @details Writes CCPxCON with EN set for every mode but OFF. The datasheet
 * advises clearing CCPxCON before changing a capture prescaler: switch to
 * CCP_MODE_OFF first when the old mode is a capture mode, and clear CCPxIF
 * afterwards (a mode change can raise a false event).
 * @param inst Instance, 1..5.
 * @param mode New mode.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance or reserved mode.
 */
EPIC_StatusTypeDef EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode);

/**
 * @brief Read the captured value with a consistency retry: the high byte is
 *        read around the low byte until it is stable.
 * @param inst Instance, 1..5.
 * @return The 16-bit CCPRxH:L value, or 0 for an out-of-range instance.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);

/**
 * @brief Set the PWM duty in 10-bit counts, honouring the module's FMT bit.
 * @details Takes effect at the next period: the hardware latches the duty
 * when TMRx matches PRx, so the write is glitch-free at any time. Counts are
 * 1:4 of a timer step; a duty of at least EPIC_CCP_PWMPeriodCounts() keeps
 * the output high for the whole period.
 * @param inst Instance, 1..5.
 * @param duty Duty in counts, 0..1023.
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance or duty above 1023.
 */
EPIC_StatusTypeDef EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty);

/**
 * @brief Number of PWM duty counts in one period for a PRx value.
 * @details The period is 4 x (PRx + 1) counts (DS40001802D Equation 30-1
 * and 30-3), so 100% duty is this count or more and the resolution is
 * log2 of it, capped at 10 bits.
 * @param period PRx value, 0..255.
 * @return 4 to 1024.
 */
uint16_t EPIC_CCP_PWMPeriodCounts(uint8_t period);

/**
 * @brief Read CCPxCON.OUT, the output data bit.
 * @param inst Instance, 1..5.
 * @return 1 when the output is high, 0 when low or the instance is out of range.
 */
uint8_t EPIC_CCP_GetOutput(CCP_InstanceTypeDef inst);

/**
 * @brief CCP1 ISR: clears CCP1IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when CCP1IF and CCP1IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void CCP1_IRQHandler(void);
/** @brief CCP2 ISR; same contract as CCP1_IRQHandler. */
void CCP2_IRQHandler(void);
/** @brief CCP3 ISR; same contract as CCP1_IRQHandler. */
void CCP3_IRQHandler(void);
/** @brief CCP4 ISR; same contract as CCP1_IRQHandler. */
void CCP4_IRQHandler(void);
/** @brief CCP5 ISR; same contract as CCP1_IRQHandler. */
void CCP5_IRQHandler(void);

#endif
