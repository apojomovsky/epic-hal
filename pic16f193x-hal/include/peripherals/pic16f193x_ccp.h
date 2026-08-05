/**
 * @file    peripherals/pic16f193x_ccp.h
 * @brief   PIC16F193X CCP1/CCP2 driver, capture and compare modes
 *          only this phase (PWM deferred, see the plan's Non-goals).
 *
 * @details
 *   Source: DS41364B §15.0, Registers 15-1/15-2. Both instances are
 *   Enhanced CCP on this device (unlike PIC18 where only CCP1 is).
 *   Handle shape mirrors pic18fxx5x_ccp.h's Cube-style API, adapted
 *   for this family. Full reference: MANUAL.md (see that file's table
 *   of contents for the current section number).
 *
 *   Every SFR access inside the driver branches on the instance before
 *   touching any register, so each branch's own access stays a literal
 *   PIC_REG_* token (mirrors pic18fxx5x_ccp.c's CCP_WRITE_* and
 *   CCP_READ_* macros, docs/adding-a-device.md section 4.8's proven
 *   pattern).
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

EPIC_StatusTypeDef EPIC_CCP_Init(const CCP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_CCP_DeInit(CCP_InstanceTypeDef inst);
EPIC_StatusTypeDef EPIC_CCP_SetCompare(CCP_InstanceTypeDef inst, uint16_t value);
uint16_t EPIC_CCP_GetCapture(CCP_InstanceTypeDef inst);

/** Weak CCP1/CCP2 ISRs, one per instance, override in user code. */
void CCP1_IRQHandler(void) PIC8_WEAK;
void CCP2_IRQHandler(void) PIC8_WEAK;
void CCP3_IRQHandler(void) PIC8_WEAK;
void CCP4_IRQHandler(void) PIC8_WEAK;
void CCP5_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F193X_CCP_H */
