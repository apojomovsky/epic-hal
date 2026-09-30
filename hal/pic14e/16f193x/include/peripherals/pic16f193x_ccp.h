/**
 * PIC16F193X CCP1-5 driver (DS41364B §15.0, Registers 15-1/15-2),
 * capture and compare modes only this phase (PWM deferred). Both
 * CCP1/CCP2 are Enhanced CCP on this device. Every SFR access inside
 * the driver branches on the instance before touching any register, so
 * each branch's own access stays a literal PIC_REG_* token (mirrors
 * pic18fxx5x_ccp.c's CCP_WRITE_* / CCP_READ_* macros,
 * docs/adding-a-device.md §4.8). Full reference: MANUAL.md.
 */

#ifndef PIC16F193X_CCP_H
#define PIC16F193X_CCP_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief Which CCP instance a handle or call refers to. Values equal
 *        the instance number for readability (mirrors
 *        CCP_InstanceTypeDef's CCP_INSTANCE_1/2 = 1/2 convention).
 */
typedef enum {
    CCP_INSTANCE_1 = 1,   /**< CCP1, CCPR1L/H + CCP1CON (bank 5). */
    CCP_INSTANCE_2 = 2,   /**< CCP2, CCPR2L/H + CCP2CON (bank 5). */
    CCP_INSTANCE_3 = 3,   /**< CCP3, CCPR3L/H + CCP3CON (bank 6, ECCP). */
    CCP_INSTANCE_4 = 4,   /**< CCP4, CCPR4L/H + CCP4CON (bank 6, plain). */
    CCP_INSTANCE_5 = 5,   /**< CCP5, CCPR5L/H + CCP5CON (bank 6, plain). */
} CCP_InstanceTypeDef;

/** CCPxCON<3:0> mode select, capture/compare encodings only this phase
 *  (DS41364B Register 15-1); PWM (11xx) is rejected by EPIC_CCP_Init,
 *  not silently accepted, mirroring Timer1's TIMER1_CLOCK_EXTERNAL
 *  rejection precedent. */
typedef enum {
    CCP_MODE_OFF              = 0x0U,  /**< 0000, module disabled. */
    CCP_MODE_CAPTURE_FALLING  = 0x4U,  /**< 0100, every falling edge. */
    CCP_MODE_CAPTURE_RISING   = 0x5U,  /**< 0101, every rising edge. */
    CCP_MODE_CAPTURE_4TH      = 0x6U,  /**< 0110, every 4th rising. */
    CCP_MODE_CAPTURE_16TH     = 0x7U,  /**< 0111, every 16th rising. */
    CCP_MODE_COMPARE_SET      = 0x8U,  /**< 1000, set output on match. */
    CCP_MODE_COMPARE_CLEAR    = 0x9U,  /**< 1001, clear output on match. */
    CCP_MODE_COMPARE_SOFT_IF  = 0xAU,  /**< 1010, software interrupt only. */
    CCP_MODE_COMPARE_TRIGGER  = 0xBU,  /**< 1011, special event trigger. */
    CCP_MODE_PWM              = 0xCU,  /**< 11xx: rejected this phase. */
} CCP_ModeTypeDef;

/** Driver handle (Cube-style). One handle per instance. */
typedef struct {
    CCP_InstanceTypeDef Instance;
    CCP_ModeTypeDef      Mode;
    uint16_t             CompareValue;   /**< 16-bit compare/capture value. */
    /** @brief Fires on CCPxIF; NULL = no callback, IRQ still clears flag. */
    void (*EventCallback)(void);
} CCP_HandleTypeDef;

#define CCP_HANDLE_DEFAULT { \
    .Instance = CCP_INSTANCE_1, .Mode = CCP_MODE_OFF, \
    .CompareValue = 0U, .EventCallback = 0, \
}

/**
 * @brief Configure a CCP module from the handle and enable it: loads the
 *        compare/capture value into CCPRxH:L, programs the mode into
 *        CCPxCON, and enables or disables the instance IRQ based on
 *        `EventCallback`. CCP_MODE_PWM is rejected this phase.
 * @param h handle with instance, mode, compare value and callback
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle, an
 *         out-of-range instance, or CCP_MODE_PWM
 */
EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);
/**
 * @brief Disable a CCP module: disables the instance IRQ, clears its
 *        flag, resets CCPxCON, and releases the stored handle.
 * @param inst which CCP instance (1-5)
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance
 */
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);
/**
 * @brief Load a new 16-bit compare/capture value into CCPRxH:L.
 * @param inst which CCP instance (1-5)
 * @param value 16-bit value to write to the CCPR registers
 * @return EPIC_OK on success, EPIC_INVALID for an out-of-range instance
 */
EPIC_StatusTypeDef EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);

/**
 * @brief Change only CCPxCON's mode field, leaving CCPRx and IRQ enable
 *        state untouched.
 *
 * Cheap: no flag-clear or IRQ-enable bookkeeping, unlike EPIC_CCP_Init.
 * For repeated in-frame mode switches (bit-banged protocols reprogramming
 * the module every bit), not one-time setup.
 *
 * @param inst which CCP instance (1-5)
 * @param mode new capture/compare mode (CCP_MODE_PWM rejected at Init)
 */
void EPIC_CCP_SetMode(CCP_InstanceTypeDef inst, CCP_ModeTypeDef mode);

/**
 * @brief Read the latched capture value with a consistency retry: the
 *        high byte is read twice around the low byte and re-read until
 *        stable, so the returned 16-bit value is coherent.
 * @param inst which CCP instance (1-5)
 * @return the captured 16-bit value, or 0 for an out-of-range instance
 */
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);

/**
 * @brief Weak CCP1/CCP2 ISRs, one per instance, override in user code.
 */
void CCP1_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CCP2 ISR, override in user code.
 */
void CCP2_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CCP3 ISR, override in user code.
 */
void CCP3_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CCP4 ISR, override in user code.
 */
void CCP4_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CCP5 ISR, override in user code.
 */
void CCP5_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_CCP_H */
