/**
 * PIC16F188xx Timer1 driver (DS40001802H §28, Registers 28-1..28-4; the
 * 16F18857/77 sheet DS40001825F carries identical register text). 16-bit
 * timer/counter with a prescaler (1:1..1:8), a T1CLK clock select, and a
 * gate (T1GCON, T1GATE) that enables counting from a selectable signal,
 * with polarity, toggle and single-pulse modes. TMR1IF (PIR4) sets on the
 * FFFFh to 0000h rollover.
 */
#ifndef PIC16F188XX_TIMER1_H
#define PIC16F188XX_TIMER1_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/**
 * @brief Clock source (T1CLK<CS3:CS0>, DS40001802H Register 28-3).
 *        Code 1001 (the Timer1 overflow itself) is reserved on Timer1 and
 *        has no enumerator. T1CKIPPS selects pin RC0 at POR; remapping
 *        needs PPS setup the HAL does not provide yet. The TMR3/TMR5
 *        overflow rows need timers this HAL has no driver for.
 */
typedef enum {
    TIMER1_CS_T1CKIPPS  = 0x0U,
    TIMER1_CS_FOSC4     = 0x1U,
    TIMER1_CS_FOSC      = 0x2U,
    TIMER1_CS_HFINTOSC  = 0x3U,
    TIMER1_CS_LFINTOSC  = 0x4U,
    TIMER1_CS_MFINTOSC  = 0x5U,
    TIMER1_CS_SOSC      = 0x6U,
    TIMER1_CS_CLKR      = 0x7U,
    TIMER1_CS_TMR0_OVF  = 0x8U,
    TIMER1_CS_TMR3_OVF  = 0xAU,
    TIMER1_CS_TMR5_OVF  = 0xBU,
    TIMER1_CS_LC1OUT    = 0xCU,
    TIMER1_CS_LC2OUT    = 0xDU,
    TIMER1_CS_LC3OUT    = 0xEU,
    TIMER1_CS_LC4OUT    = 0xFU,
} TIMER1_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T1CON<CKPS1:CKPS0>, DS40001802H Register 28-1).
 */
typedef enum {
    TIMER1_PRESCALER_1_1 = 0x0U,
    TIMER1_PRESCALER_1_2 = 0x1U,
    TIMER1_PRESCALER_1_4 = 0x2U,
    TIMER1_PRESCALER_1_8 = 0x3U,
} TIMER1_PrescalerTypeDef;

/**
 * @brief Gate source (T1GATE<GSS4:GSS0>, DS40001802H Register 28-4).
 *        Code 00010 (the Timer1 overflow itself) is reserved on Timer1
 *        and has no enumerator. T1GPPS selects pin RB5 at POR; remapping
 *        needs PPS setup the HAL does not provide yet.
 */
typedef enum {
    TIMER1_GATE_T1GPPS    = 0x00U,
    TIMER1_GATE_TMR0_OVF  = 0x01U,
    TIMER1_GATE_TMR2_POST = 0x03U,
    TIMER1_GATE_TMR3_OVF  = 0x04U,
    TIMER1_GATE_TMR4_POST = 0x05U,
    TIMER1_GATE_TMR5_OVF  = 0x06U,
    TIMER1_GATE_TMR6_POST = 0x07U,
    TIMER1_GATE_SMT1_MATCH = 0x08U,
    TIMER1_GATE_SMT2_MATCH = 0x09U,
    TIMER1_GATE_CCP1_OUT  = 0x0AU,
    TIMER1_GATE_CCP2_OUT  = 0x0BU,
    TIMER1_GATE_CCP3_OUT  = 0x0CU,
    TIMER1_GATE_CCP4_OUT  = 0x0DU,
    TIMER1_GATE_CCP5_OUT  = 0x0EU,
    TIMER1_GATE_PWM6_OUT  = 0x0FU,
    TIMER1_GATE_PWM7_OUT  = 0x10U,
    TIMER1_GATE_DDS_OUT   = 0x11U,
    TIMER1_GATE_C1OUT     = 0x12U,
    TIMER1_GATE_C2OUT     = 0x13U,
    TIMER1_GATE_ZCD1      = 0x14U,
    TIMER1_GATE_LC1OUT    = 0x15U,
    TIMER1_GATE_LC2OUT    = 0x16U,
    TIMER1_GATE_LC3OUT    = 0x17U,
    TIMER1_GATE_LC4OUT    = 0x18U,
} TIMER1_GateSourceTypeDef;

/**
 * @brief Gate configuration (T1GCON and T1GATE, Registers 28-2 and 28-4).
 *        The gate only acts while Enable is set and the timer is on.
 */
typedef struct {
    uint8_t                  Enable;      /**< T1GCON.GE: nonzero lets the gate control counting. */
    uint8_t                  ActiveHigh;  /**< T1GCON.GPOL: nonzero counts while the gate is high, zero while low. */
    uint8_t                  Toggle;      /**< T1GCON.GTM: nonzero measures a full gate cycle (flip-flop toggles per rising edge). */
    uint8_t                  SinglePulse; /**< T1GCON.GSPM: nonzero gates one acquisition per EPIC_TIMER1_GateArm. */
    TIMER1_GateSourceTypeDef Source;
} TIMER1_GateTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER1_ClockSourceTypeDef ClockSource;
    TIMER1_PrescalerTypeDef   Prescaler;
    uint8_t                   Async;       /**< T1CON.SYNC: nonzero leaves an external input unsynchronized; ignored for FOSC and FOSC/4. */
    TIMER1_GateTypeDef        Gate;
    uint16_t                  ReloadValue; /**< 16-bit initial counter. */
    /** @brief Optional overflow callback (fires on TMR1IF). */
    void (*OverflowCallback)(void);
} TIMER1_HandleTypeDef;

#define TIMER1_HANDLE_DEFAULT {                                           \
    .ClockSource      = TIMER1_CS_FOSC4,                                  \
    .Prescaler        = TIMER1_PRESCALER_1_1,                             \
    .Async            = 0U,                                               \
    .Gate             = { .Enable = 0U, .ActiveHigh = 0U, .Toggle = 0U,   \
                          .SinglePulse = 0U,                              \
                          .Source = TIMER1_GATE_T1GPPS },                 \
    .ReloadValue      = 0x0000U,                                          \
    .OverflowCallback = NULL,                                             \
}

/**
 * @brief Configure Timer1 from the handle without starting it.
 * @details Halts the timer, programs T1CLK, T1GATE, T1GCON and T1CON
 * (RD16 set so counter reads are atomic), loads ReloadValue, clears
 * TMR1IF and enables the interrupt (plus PEIE) only with a callback.
 * GGO/DONE is never set here; arm a single pulse with
 * EPIC_TIMER1_GateArm. GIE is left alone.
 * @param h Handle with clock, prescaler, gate, reload and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Init(const TIMER1_HandleTypeDef *h);

/**
 * @brief Disable the TMR1 interrupt and restore the POR state (T1CON,
 * T1GCON, T1GATE and T1CLK 0x00, counter zeroed, callback dropped).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_DeInit(void);

/**
 * @brief Start Timer1: write ReloadValue to the counter, then program
 * T1CON (prescaler, SYNC, RD16, ON) in one write.
 * @param h Handle holding the reload value, prescaler and Async.
 *          Clock select, gate and the interrupt stay as programmed by Init.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Start(const TIMER1_HandleTypeDef *h);

/**
 * @brief Stop Timer1 by clearing T1CON<ON>, which also clears the gate
 * toggle flip-flop.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_Stop(void);

/**
 * @brief Read the 16-bit counter atomically (TMR1L first, which latches
 * TMR1H while RD16 is set).
 * @return TMR1H:TMR1L value, 0..65535.
 */
uint16_t EPIC_TIMER1_ReadCounter(void);

/**
 * @brief Write the 16-bit counter value, high byte first (the TMR1L
 * write commits both bytes while RD16 is set).
 * @param value Counter value, 0..65535.
 */
void EPIC_TIMER1_WriteCounter(uint16_t value);

/**
 * @brief Arm one single-pulse gate acquisition by setting T1GCON<GGO/DONE>.
 * @details Only meaningful with Gate.SinglePulse set: the timer then
 * counts through the next gate pulse and hardware clears GGO/DONE on its
 * trailing edge.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER1_GateArm(void);

/**
 * @brief Report whether a single-pulse acquisition is still pending.
 * @return 1 while T1GCON<GGO/DONE> is set, 0 once it completed or was never armed.
 */
uint8_t EPIC_TIMER1_GateBusy(void);

/**
 * @brief Read the live gate state (T1GCON<GVAL>), valid with the gate disabled.
 * @return 1 when the gate would let the timer count, 0 otherwise.
 */
uint8_t EPIC_TIMER1_GateValue(void);

/**
 * @brief Convert a prescaler enum to its integer ratio (1, 2, 4, 8).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER1_PrescalerToRatio(TIMER1_PrescalerTypeDef p);

/**
 * @brief Timer1 ISR: clears TMR1IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR1IF and TMR1IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void TIMER1_IRQHandler(void);

#endif
