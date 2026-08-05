/**
 * @file    peripherals/pic16f193x_timer246.h
 * @brief   Timer2/Timer4/Timer6 driver: three instances of the same
 *          8-bit timer with PR-match reset and postscaler.
 *
 * @details
 *   Source: DS41364B §17.0. Full reference: MANUAL.md (see that file's
 *   table of contents for the current section number; peripherals land
 *   in whatever order the roadmap executes them in, and the section
 *   number is reassigned on merge if something else lands first).
 *
 *   Unlike Timer0/Timer1 (raw free-running overflow at 0xFF/0xFFFF),
 *   TMRx counts 0..PRx and resets to 0 on the cycle it would exceed
 *   PRx (never reaches PRx+1). TMRxIF fires once every
 *   prescaler x (PRx+1) x postscaler input cycles: PR match happens
 *   every prescaler x (PRx+1) cycles, and the postscaler divides that
 *   further before setting the flag.
 *
 *   One driver covers all three instances via TIMER246_InstanceTypeDef
 *   (mirrors pic18fxx5x_ccp.h's CCP_InstanceTypeDef convention). Every
 *   SFR access inside the driver branches on the instance before
 *   touching any register, so each branch's own access stays a literal
 *   PIC_REG_* token (see pic18fxx5x_ccp.c's CCP_WRITE_* and CCP_READ_*
 *   macros for the proven shape this mirrors).
 */

#ifndef PIC16F193X_TIMER246_H
#define PIC16F193X_TIMER246_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

/**
 * @brief Which Timer2/4/6 instance a handle or call refers to. Values
 *        equal the timer number for readability (mirrors
 *        CCP_InstanceTypeDef's CCP_INSTANCE_1/2 = 1/2 convention).
 */
typedef enum {
    TIMER246_INSTANCE_2 = 2,   /**< Timer2, TMR2/PR2/T2CON. */
    TIMER246_INSTANCE_4 = 4,   /**< Timer4, TMR4/PR4/T4CON. */
    TIMER246_INSTANCE_6 = 6,   /**< Timer6, TMR6/PR6/T6CON. */
} TIMER246_InstanceTypeDef;

/**
 * @brief Prescaler ratio (T*CON<T*CKPS1:T*CKPS0>, DS41364B §17.0).
 *        Only 3 of the 4 encodings are distinct: 00=1:1, 01=1:4,
 *        1x=1:16 (both 10 and 11 give 1:16). No separate enum value
 *        for 0x3; EPIC_TIMER246_PrescalerToRatio's lookup table has all
 *        4 entries, the enum only exposes the 3 distinct ratios.
 */
typedef enum {
    TIMER246_PRESCALER_1_1  = 0x0U,
    TIMER246_PRESCALER_1_4  = 0x1U,
    TIMER246_PRESCALER_1_16 = 0x2U,
} TIMER246_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T*CON<TOUTPS3:TOUTPS0>, DS41364B §17.0).
 *        Postscaler value N -> 1:(N+1) divider, linear, all 16
 *        encodings distinct.
 */
typedef enum {
    TIMER246_POSTSCALER_1_1  = 0x0U,
    TIMER246_POSTSCALER_1_2  = 0x1U,
    TIMER246_POSTSCALER_1_3  = 0x2U,
    TIMER246_POSTSCALER_1_4  = 0x3U,
    TIMER246_POSTSCALER_1_5  = 0x4U,
    TIMER246_POSTSCALER_1_6  = 0x5U,
    TIMER246_POSTSCALER_1_7  = 0x6U,
    TIMER246_POSTSCALER_1_8  = 0x7U,
    TIMER246_POSTSCALER_1_9  = 0x8U,
    TIMER246_POSTSCALER_1_10 = 0x9U,
    TIMER246_POSTSCALER_1_11 = 0xAU,
    TIMER246_POSTSCALER_1_12 = 0xBU,
    TIMER246_POSTSCALER_1_13 = 0xCU,
    TIMER246_POSTSCALER_1_14 = 0xDU,
    TIMER246_POSTSCALER_1_15 = 0xEU,
    TIMER246_POSTSCALER_1_16 = 0xFU,
} TIMER246_PostscalerTypeDef;

/** Driver handle (Cube-style). One handle per instance; the caller
 *  owns three of these if it uses all three timers. */
typedef struct {
    TIMER246_InstanceTypeDef    Instance;
    TIMER246_PrescalerTypeDef   Prescaler;
    TIMER246_PostscalerTypeDef  Postscaler;
    uint8_t                     Period;   /**< PRx value, 0..255. */
    /** @brief Optional overflow callback (fires on TMRxIF, i.e. every
     *         prescaler x (PRx+1) x postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER246_HandleTypeDef;

#define TIMER246_HANDLE_DEFAULT {                                      \
    .Instance         = TIMER246_INSTANCE_2,                           \
    .Prescaler        = TIMER246_PRESCALER_1_1,                        \
    .Postscaler       = TIMER246_POSTSCALER_1_1,                       \
    .Period           = 0xFFU,                                         \
    .OverflowCallback = NULL,                                          \
}

EPIC_StatusTypeDef EPIC_TIMER246_Init(const TIMER246_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER246_DeInit(TIMER246_InstanceTypeDef inst);
EPIC_StatusTypeDef EPIC_TIMER246_Start(const TIMER246_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_TIMER246_Stop(TIMER246_InstanceTypeDef inst);

uint8_t  EPIC_TIMER246_ReadCounter(TIMER246_InstanceTypeDef inst);
void     EPIC_TIMER246_WriteCounter(TIMER246_InstanceTypeDef inst, uint8_t value);

uint8_t  EPIC_TIMER246_ReadPeriod(TIMER246_InstanceTypeDef inst);
void     EPIC_TIMER246_WritePeriod(TIMER246_InstanceTypeDef inst, uint8_t period);

uint16_t EPIC_TIMER246_PrescalerToRatio(TIMER246_PrescalerTypeDef p);
uint16_t EPIC_TIMER246_PostscalerToRatio(TIMER246_PostscalerTypeDef p);

/** Weak Timer2/4/6 ISRs, one per instance, override in user code. */
void TIMER2_IRQHandler(void) PIC8_WEAK;
void TIMER4_IRQHandler(void) PIC8_WEAK;
void TIMER6_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F193X_TIMER246_H */
