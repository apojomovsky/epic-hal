/*
 * ECCP1 + CCP2 driver (DS39631E §15.0): capture/compare/PWM. CCP1 is the
 * Enhanced CCP, adding auto-shutdown/restart (ECCP1AS) and PRSEN; CCP2 is
 * plain. NOTE: the 2520's ECCP1 is REDUCED vs the 4550's: no P1M bridge
 * modes (single P1A output only), no PDC dead-band delay, no PSSBD
 * (P1B/P1D do not exist on this 28-pin part). Capture/Compare use Timer1
 * or Timer3 (T3CON<T3CCP2:T3CCP1>, reset default Timer1); PWM uses Timer2.
 */

#ifndef PIC18F2520_CCP_H
#define PIC18F2520_CCP_H

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"

/**
 * @brief Which CCP module a handle refers to.
 */
typedef enum {
    CCP_INSTANCE_1 = 1,    /**< CCP1 / ECCP1, pin P1A (RC2). */
    CCP_INSTANCE_2 = 2,    /**< CCP2, pin RC1 (or RB3 if CCP2MX=PORTB). */
} CCP_InstanceTypeDef;

/**
 * @brief CCP/ECCP operating mode (CCPxCON<3:0>, DS39631E Register 15-1).
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
 * @brief Auto-shutdown pin state on a shutdown event (PSSAC1:PSSAC0,
 *        DS39631E Register 15-2). 1x = tri-state. Applies to P1A/P1C;
 *        P1B/P1D do not exist on this part (no PSSBD).
 */
typedef enum {
    CCP_SHUTDOWN_DRIVE_0  = 0x0U,   /**< 00: drive the pin(s) to 0. */
    CCP_SHUTDOWN_DRIVE_1  = 0x1U,   /**< 01: drive the pin(s) to 1. */
    CCP_SHUTDOWN_TRISTATE = 0x2U,   /**< 1x: tri-state the pin(s). */
} CCP_PinStateTypeDef;

/**
 * @brief Auto-shutdown source (ECCPAS2:ECCPAS0, DS39631E Register 15-2).
 *        FLT0 is the external fault pin; the comparators are the analog
 *        comparators. `CCP_AUTOSHUTDOWN_DISABLED` turns auto-shutdown off.
 */
typedef enum {
    CCP_AUTOSHUTDOWN_DISABLED               = 0x0U,  /**< 000. */
    CCP_AUTOSHUTDOWN_COMP1                  = 0x1U,  /**< 001: Comparator 1 output. */
    CCP_AUTOSHUTDOWN_COMP2                  = 0x2U,  /**< 010: Comparator 2 output. */
    CCP_AUTOSHUTDOWN_COMP1_OR_COMP2         = 0x3U,  /**< 011. */
    CCP_AUTOSHUTDOWN_FLT0                   = 0x4U,  /**< 100: FLT0 pin. */
    CCP_AUTOSHUTDOWN_FLT0_OR_COMP1          = 0x5U,  /**< 101. */
    CCP_AUTOSHUTDOWN_FLT0_OR_COMP2          = 0x6U,  /**< 110. */
    CCP_AUTOSHUTDOWN_FLT0_OR_COMP1_OR_COMP2 = 0x7U,  /**< 111. */
} CCP_AutoShutdownSourceTypeDef;

/**
 * @brief Auto-shutdown configuration (ECCP1AS, ECCP1 only). No PinsBD:
 *        P1B/P1D do not exist on this 28-pin part.
 */
typedef struct {
    CCP_AutoShutdownSourceTypeDef Source;  /**< ECCPAS2:ECCPAS0. */
    CCP_PinStateTypeDef           PinsAC;  /**< P1A/P1C shutdown state. */
    bool                          AutoRestart; /**< PRSEN (PWM1CON<7>). */
} CCP_AutoShutdownConfigTypeDef;

/**
 * @brief Driver handle (Cube-style). AutoShutdown applies to CCP1;
 *        CCP2 ignores it. No bridge/dead-band fields: the 2520 has no
 *        P1M/PDC hardware.
 */
typedef struct {
    CCP_InstanceTypeDef            Instance;
    CCP_ModeTypeDef                Mode;
    uint16_t                       CompareValue;  /**< 16-bit compare/capture value. */
    CCP_PWMConfigTypeDef           PWM;           /**< PWM period + duty (PWM mode). */
    CCP_AutoShutdownConfigTypeDef  AutoShutdown;  /**< ECCP1AS+PRSEN (ECCP1 only). */
    void (*EventCallback)(void);                  /**< Fires on CCP1IF / CCP2IF. */
} CCP_HandleTypeDef;

/**
 * @brief  Configure the CCP/ECCP module. Programs CCPxCON (mode + duty
 *         LSBs), CCPRxL/H, ECCP1AS (auto-shutdown) + PRSEN for ECCP1,
 *         and enables the matching interrupt if `EventCallback != NULL`.
 *
 * @note   For PWM, also call `EPIC_TIMER2_Init` + `EPIC_TIMER2_Start` with a
 *         period matching `h->PWM.Period` before/after this call (Timer2 is
 *         the PWM time base). For capture/compare, start Timer1 (or Timer3).
 * @param h the CCP handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID on bad handle/instance.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);

/**
 * @brief Reset CCPxCON (and ECCP1AS/PRSEN for ECCP1) to POR; clear PIR flag.
 * @param inst which CCP module to reset (CCP_INSTANCE_1 or CCP_INSTANCE_2).
 * @return EPIC_OK on success, EPIC_INVALID on bad instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);

/**
 * @brief Set the 16-bit CCPRx value (high byte first, DS39631E §15.x idiom).
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

/**
 * @brief  Configure the ECCP1 auto-shutdown source + pin state + restart
 *         (ECCP1AS + PRSEN). No-op for CCP2. Pass
 *         `CCP_AUTOSHUTDOWN_DISABLED` to turn it off.
 * @param inst which CCP module to configure (only CCP1 applies).
 * @param source the auto-shutdown source.
 * @param pins_ac the P1A/P1C shutdown pin state.
 * @param auto_restart true for hardware auto-restart (PRSEN).
 */
void EPIC_CCP_ConfigAutoShutdown(CCP_InstanceTypeDef inst,
                                CCP_AutoShutdownSourceTypeDef source,
                                CCP_PinStateTypeDef pins_ac,
                                bool auto_restart);

/**
 * @brief Returns 1 if an ECCP1 auto-shutdown event is active
 *        (ECCP1AS<ECCPASE>).
 * @param inst which CCP module to query (only CCP1 applies).
 * @return 1 while an auto-shutdown event is active, else 0.
 */
uint8_t EPIC_CCP_IsShutdown(CCP_InstanceTypeDef inst);

/**
 * @brief  Clear the ECCP1 auto-shutdown status (ECCPASE) to restart the PWM.
 *         Only effective when PRSEN = 0 (manual restart). No-op for CCP2.
 * @param inst which CCP module to restart (only CCP1 applies).
 */
void EPIC_CCP_Restart(CCP_InstanceTypeDef inst);

/**
 * @brief Weak CCP1 (ECCP1) ISR, override in user code.
 */
void CCP1_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CCP2 ISR, override in user code.
 */
void CCP2_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F2520_CCP_H */
