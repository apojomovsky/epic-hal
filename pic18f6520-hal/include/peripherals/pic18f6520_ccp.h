/*
 * CCP1-5 driver (DS39609B §16.0): capture/compare/PWM. All five
 * instances are plain CCP modules: no ECCP auto-shutdown/PWM bridge
 * registers exist on this part (confirmed absent from the DFP header,
 * no PSTRCON/ECCPAS/PWM1CON), unlike the 4550's ECCP1 or the 2520's
 * reduced ECCP1. Capture/Compare use Timer1 or Timer3 (T3CON<
 * T3CCP2:T3CCP1>, reset default Timer1+Timer2, DS39609B Register
 * 14-1); PWM uses Timer2. One driver with an instance selector, per
 * docs/adding-a-device.md §5 step 6's multiple-identical-instances
 * rule (the 193x CCP1-5 precedent).
 */

#ifndef PIC18F6520_CCP_H
#define PIC18F6520_CCP_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"

/**
 * @brief Which CCP module a handle refers to. Values equal the
 *        instance number for readability (mirrors the 193x/2520
 *        CCP_InstanceTypeDef = 1..5 convention).
 */
typedef enum {
    CCP_INSTANCE_1 = 1,    /**< CCP1. */
    CCP_INSTANCE_2 = 2,    /**< CCP2. */
    CCP_INSTANCE_3 = 3,    /**< CCP3. */
    CCP_INSTANCE_4 = 4,    /**< CCP4. */
    CCP_INSTANCE_5 = 5,    /**< CCP5. */
} CCP_InstanceTypeDef;

/**
 * @brief CCP operating mode (CCPxCON<3:0>, DS39609B Register 16-1).
 *        PIC18 adds CCP_MODE_COMPARE_TOGGLE (0010) vs. PIC16.
 */
typedef enum {
    CCP_MODE_OFF              = 0x0U,   /**< 0000, module disabled. */
    CCP_MODE_CAPTURE_FALLING  = 0x4U,   /**< 0100, every falling edge.  */
    CCP_MODE_CAPTURE_RISING   = 0x5U,   /**< 0101, every rising edge.   */
    CCP_MODE_CAPTURE_4TH      = 0x6U,   /**< 0110, every 4th rising.    */
    CCP_MODE_CAPTURE_16TH     = 0x7U,   /**< 0111, every 16th rising.   */
    CCP_MODE_COMPARE_TOGGLE   = 0x2U,   /**< 0010, toggle output on match (PIC18). */
    CCP_MODE_COMPARE_SET      = 0x8U,   /**< 1000, set output on match. */
    CCP_MODE_COMPARE_CLEAR    = 0x9U,   /**< 1001, clear output on match. */
    CCP_MODE_COMPARE_SOFT_IF  = 0xAU,   /**< 1010, software interrupt only. */
    CCP_MODE_COMPARE_TRIGGER  = 0xBU,   /**< 1011, special event trigger. */
    CCP_MODE_PWM              = 0xCU,   /**< 11xx, PWM (any 11xx). */
} CCP_ModeTypeDef;

/**
 * @brief Configuration for PWM mode. Duty is 10-bit, encoded as
 *        `duty_full = (CCPRxL:CCPxCON<5:4>)`. `Period` is the Timer2 PR2
 *        value (informational; program PR2 via the Timer2 driver).
 */
typedef struct {
    uint16_t Period;   /**< Timer2 PR2 value, 0..255. */
    uint16_t Duty;     /**< 10-bit PWM duty, 0..1023. */
} CCP_PWMConfigTypeDef;

/**
 * @brief Driver handle (Cube-style). All five instances are plain CCP:
 *        no auto-shutdown fields (no ECCP1AS/PWM1CON on this part).
 */
typedef struct {
    CCP_InstanceTypeDef            Instance;
    CCP_ModeTypeDef                Mode;
    uint16_t                       CompareValue;  /**< 16-bit compare/capture value. */
    CCP_PWMConfigTypeDef           PWM;           /**< PWM period + duty (PWM mode). */
    void (*EventCallback)(void);                  /**< Fires on CCPxIF. */
} CCP_HandleTypeDef;

/**
 * @brief  Default initialiser: CCP1, module off, no callback.
 */
#define CCP_HANDLE_DEFAULT {                                            \
    .Instance      = CCP_INSTANCE_1,                                    \
    .Mode          = CCP_MODE_OFF,                                      \
    .CompareValue  = 0U,                                                \
    .PWM           = { 0U, 0U },                                        \
    .EventCallback = NULL,                                              \
}

/**
 * @brief  Configure a CCP module. Programs CCPxCON (mode + duty LSBs),
 *         CCPRxL/H, and enables the matching interrupt if
 *         `EventCallback != NULL`.
 *
 * @note   For PWM, also call `EPIC_TIMER2_Init` + `EPIC_TIMER2_Start` with a
 *         period matching `h->PWM.Period` before/after this call (Timer2 is
 *         the PWM time base). For capture/compare, start Timer1 (or Timer3).
 * @param h the CCP handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID on bad handle/instance.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);

/**
 * @brief Reset CCPxCON to POR; clear the PIR flag; disable the IRQ.
 * @param inst which CCP module to reset (CCP_INSTANCE_1..5).
 * @return EPIC_OK on success, EPIC_INVALID on bad instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);

/**
 * @brief Set the 16-bit CCPRx value (high byte first, DS39609B §16.x idiom).
 * @param inst which CCP module to configure.
 * @param value the 16-bit compare/capture value to load.
 */
void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);

/**
 * @brief Change only CCPxCON's mode field, leaving CCPRx and IRQ enable
 *        state untouched.
 * @param inst which CCP module to configure.
 * @param mode the new operating mode (a @ref CCP_ModeTypeDef value).
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode);

/**
 * @brief Atomically read the 16-bit CCPRx value.
 * @param inst which CCP module to read.
 * @return the current 16-bit CCPRx value.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);

/**
 * @brief  Set PWM duty in 10-bit units (0..1023). Writes the LSBs into
 *         CCPxCON<5:4> then CCPRxL (bits 9:2), preserving the mode bits.
 * @param inst which CCP module to configure.
 * @param duty the 10-bit duty value, 0..1023.
 */
void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty);

/** @brief Weak CCP1 ISR (PIR1<CCP1IF>), override in user code. */
void CCP1_IRQHandler(void) EPIC_WEAK;
/** @brief Weak CCP2 ISR (PIR2<CCP2IF>), override in user code. */
void CCP2_IRQHandler(void) EPIC_WEAK;
/** @brief Weak CCP3 ISR (PIR3<CCP3IF>), override in user code. */
void CCP3_IRQHandler(void) EPIC_WEAK;
/** @brief Weak CCP4 ISR (PIR3<CCP4IF>), override in user code. */
void CCP4_IRQHandler(void) EPIC_WEAK;
/** @brief Weak CCP5 ISR (PIR3<CCP5IF>), override in user code. */
void CCP5_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F6520_CCP_H */
