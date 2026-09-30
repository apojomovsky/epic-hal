/*
 * ECCP1 (Enhanced CCP) driver (DS39605F §15.0): capture/compare/PWM.
 * The 1320 has a single Enhanced CCP1 (no CCP2, no SPP): full ECCP with
 * P1M multi-output PWM, ECCPAS auto-shutdown and PWM1CON dead-band/
 * auto-restart, matching the 4550's ECCP1 register shape.
 * Capture/Compare use Timer1 or Timer3 (T3CON<T3CCP1>, reset default
 * Timer1); PWM always uses Timer2.
 */

#ifndef PIC18F1320_ECCP_H
#define PIC18F1320_ECCP_H

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"

/**
 * @brief Which CCP module a handle refers to. Only CCP1 (ECCP1) exists on
 *        this part; CCP_INSTANCE_2 is absent (no CCP2), so the driver
 *        acts only on CCP_INSTANCE_1 but keeps the fixed contract name.
 */
typedef enum {
    CCP_INSTANCE_1 = 1,    /**< ECCP1, the only CCP module on this part. */
} CCP_InstanceTypeDef;

/**
 * @brief CCP/ECCP operating mode (CCP1CON<3:0>, DS39605F Register 15-1).
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
 * @brief Enhanced PWM output configuration (CCP1CON<P1M1:P1M0>, DS39605F
 *        Register 15-1).
 */
typedef enum {
    CCP_PWM_OUTPUT_SINGLE       = 0x0U,  /**< 00: P1A modulated; P1B/C/D port pins. */
    CCP_PWM_OUTPUT_FULL_FORWARD = 0x1U,  /**< 01: full-bridge forward.            */
    CCP_PWM_OUTPUT_HALF_BRIDGE  = 0x2U,  /**< 10: P1A,P1B modulated w/ dead-band.  */
    CCP_PWM_OUTPUT_FULL_REVERSE = 0x3U,  /**< 11: full-bridge reverse.            */
} CCP_PWMOutputTypeDef;

/**
 * @brief Configuration for PWM mode. Duty is 10-bit, encoded as
 *        `duty_full = (CCPR1L:CCP1CON<5:4>)`. `Period` is the Timer2 PR2
 *        value (informational; program PR2 via the Timer2 driver).
 */
typedef struct {
    uint16_t Period;   /**< Timer2 PR2 value, 0..255. */
    uint16_t Duty;     /**< 10-bit PWM duty, 0..1023. */
} CCP_PWMConfigTypeDef;

/**
 * @brief Dead-band delay + auto-restart (PWM1CON, DS39605F Register 15-2).
 *        Relevant in half-bridge and full-bridge PWM modes. `Delay` is the
 *        7-bit PDC6:PDC0 count (dead-band time in instruction-cycle units
 *        per DS39605F §15.5.4). `AutoRestart` maps to PRSEN: when true, the
 *        PWM auto-restarts after an auto-shutdown clears; when false,
 *        firmware must clear ECCPASE to restart.
 */
typedef struct {
    uint8_t Delay;        /**< PDC6:PDC0, 0..127. */
    bool    AutoRestart;  /**< PRSEN. */
} CCP_DeadBandConfigTypeDef;

/**
 * @brief Auto-shutdown pin state on a shutdown event (PSSAC/PSSBD,
 *        DS39605F Register 15-3). 1x = tri-state.
 */
typedef enum {
    CCP_SHUTDOWN_DRIVE_0  = 0x0U,  /**< 00: drive the pin pair to 0. */
    CCP_SHUTDOWN_DRIVE_1  = 0x1U,  /**< 01: drive the pin pair to 1. */
    CCP_SHUTDOWN_TRISTATE = 0x2U,  /**< 1x: tri-state the pin pair. */
} CCP_PinStateTypeDef;

/**
 * @brief Auto-shutdown source (ECCPAS2:ECCPAS0, DS39605F Register 15-3).
 *        On this part the shutdown sources are the external Interrupt
 *        pins INT0/INT1/INT2 (an 18-pin part has no FLT0 fault pin and no
 *        comparators): per Register 15-3, ECCPAS0 (bit 4) selects INT1,
 *        ECCPAS1 (bit 5) selects INT2, and ECCPAS2 (bit 6) selects INT0.
 *        This differs from the 4550/2520, whose ECCPAS sources are FLT0
 *        and the two comparators; the 1320's sources are confirmed from
 *        DS39605F Register 15-3.
 */
typedef enum {
    CCP_AUTOSHUTDOWN_DISABLED  = 0x0U,  /**< 000: no auto-shutdown source. */
    CCP_AUTOSHUTDOWN_INT1      = 0x1U,  /**< 001: ECCPAS0, INT1 pin low causes shutdown. */
    CCP_AUTOSHUTDOWN_INT2      = 0x2U,  /**< 010: ECCPAS1, INT2 pin low causes shutdown. */
    CCP_AUTOSHUTDOWN_INT0      = 0x4U,  /**< 100: ECCPAS2, INT0 pin low causes shutdown. */
    CCP_AUTOSHUTDOWN_INT0_OR_INT1  = 0x5U, /**< 101. */
    CCP_AUTOSHUTDOWN_INT0_OR_INT2  = 0x6U, /**< 110. */
    CCP_AUTOSHUTDOWN_INT1_OR_INT2  = 0x3U, /**< 011. */
    CCP_AUTOSHUTDOWN_ALL_INT       = 0x7U, /**< 111. */
} CCP_AutoShutdownSourceTypeDef;

/**
 * @brief Auto-shutdown configuration (ECCPAS, DS39605F Register 15-3).
 */
typedef struct {
    CCP_AutoShutdownSourceTypeDef Source;  /**< ECCPAS2:ECCPAS0. */
    CCP_PinStateTypeDef           PinsAC;  /**< P1A/P1C shutdown state. */
    CCP_PinStateTypeDef           PinsBD;  /**< P1B/P1D shutdown state. */
} CCP_AutoShutdownConfigTypeDef;

/**
 * @brief Driver handle (Cube-style).
 */
typedef struct {
    CCP_InstanceTypeDef            Instance;    /**< Always CCP_INSTANCE_1 here. */
    CCP_ModeTypeDef                Mode;
    uint16_t                       CompareValue;  /**< 16-bit compare/capture value. */
    CCP_PWMConfigTypeDef           PWM;           /**< PWM period + duty (PWM mode). */
    CCP_PWMOutputTypeDef           PWMOutputMode; /**< P1M. */
    CCP_DeadBandConfigTypeDef      DeadBand;      /**< PWM1CON. */
    CCP_AutoShutdownConfigTypeDef  AutoShutdown;  /**< ECCPAS. */
    void (*EventCallback)(void);                  /**< Fires on CCP1IF. */
} CCP_HandleTypeDef;

/**
 * @brief  Configure the ECCP1 module. Programs CCP1CON (mode + P1M + duty
 *         LSBs), CCPR1L/H, PWM1CON (dead-band) and ECCPAS (auto-shutdown),
 *         and enables the CCP1 interrupt if `EventCallback != NULL`.
 *
 * @note   For PWM, also call `EPIC_TIMER2_Init` + `EPIC_TIMER2_Start` with a
 *         period matching `h->PWM.Period` (Timer2 is the PWM time base). For
 *         capture/compare, start Timer1 (or Timer3).
 * @param h the ECCP handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or its Instance
 *         is not CCP_INSTANCE_1.
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);

/**
 * @brief Reset CCP1CON (and PWM1CON/ECCPAS) to 0x00; clear the CCP1 flag,
 *        disable the interrupt and drop the stored handle.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return EPIC_OK on success, EPIC_INVALID for another instance.
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);

/**
 * @brief Set the 16-bit CCPR1 compare value, high byte first to avoid a
 *        spurious compare match (DS39605F §15.4).
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param value the 16-bit compare/capture value to load.
 */
void EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);

/**
 * @brief Change only CCP1CON's mode field, leaving CCPR1 and IRQ enable
 *        state untouched. Cheap: no flag-clear or IRQ-enable bookkeeping,
 *        unlike EPIC_CCP_Init. For repeated in-frame mode switches.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param mode the new operating mode (a @ref CCP_ModeTypeDef value).
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode);

/**
 * @brief Atomically read the 16-bit CCPR1 value (high-low-high idiom).
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return the current 16-bit CCPR1 value.
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);

/**
 * @brief  Set PWM duty in 10-bit units (0..1023). Writes the LSBs into
 *         CCP1CON<5:4> then CCPR1L (bits 9:2), preserving the mode bits.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param duty the 10-bit duty value, 0..1023.
 */
void EPIC_CCP_SetPWMDuty(CCP_InstanceTypeDef inst, uint16_t duty);

/**
 * @brief  Configure the ECCP1 dead-band delay + auto-restart (PWM1CON).
 *         Relevant in half-bridge / full-bridge PWM modes.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param delay the 7-bit dead-band count, 0..127.
 * @param auto_restart true to auto-restart PWM after auto-shutdown (PRSEN).
 */
void EPIC_CCP_ConfigDeadBand(CCP_InstanceTypeDef inst,
                            uint8_t delay, bool auto_restart);

/**
 * @brief  Configure the ECCP1 auto-shutdown source + pin states (ECCPAS).
 *         Pass `CCP_AUTOSHUTDOWN_DISABLED` to turn it off.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @param source the auto-shutdown source (a @ref CCP_AutoShutdownSourceTypeDef value).
 * @param pins_ac the P1A/P1C shutdown pin state.
 * @param pins_bd the P1B/P1D shutdown pin state.
 */
void EPIC_CCP_ConfigAutoShutdown(CCP_InstanceTypeDef inst,
                                CCP_AutoShutdownSourceTypeDef source,
                                CCP_PinStateTypeDef pins_ac,
                                CCP_PinStateTypeDef pins_bd);

/**
 * @brief Returns 1 if an ECCP1 auto-shutdown event is active
 *        (ECCPAS<ECCPASE>).
 * @param inst CCP module (CCP_INSTANCE_1 only).
 * @return 1 while an auto-shutdown event is active, else 0.
 */
uint8_t EPIC_CCP_IsShutdown(CCP_InstanceTypeDef inst);

/**
 * @brief  Clear the ECCP1 auto-shutdown status (ECCPASE) to restart the PWM.
 *         Only effective when PRSEN = 0 (manual restart); with PRSEN = 1 the
 *         hardware auto-clears when the shutdown source deasserts.
 * @param inst CCP module (CCP_INSTANCE_1 only).
 */
void EPIC_CCP_Restart(CCP_InstanceTypeDef inst);

/**
 * @brief Weak CCP1 (ECCP1) ISR, override in user code.
 */
void CCP1_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F1320_ECCP_H */
