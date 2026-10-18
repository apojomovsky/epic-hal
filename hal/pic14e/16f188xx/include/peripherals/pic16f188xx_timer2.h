/**
 * PIC16F188xx Timer2 driver (DS40001802H §29, Registers 29-1..29-4).
 * 8-bit timer: PR2 match, 1:1..1:128 prescaler, 1:1..1:16 postscaler,
 * T2CLKCON clock select, and the T2HLT hardware limit modes driven by
 * the T2RST signal. TMR2 counts 0..PR2 and clears on the next clock;
 * TMR2IF (PIR4) fires every prescaler x (PR2+1) x postscaler cycles.
 */
#ifndef PIC16F188XX_TIMER2_H
#define PIC16F188XX_TIMER2_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/**
 * @brief Clock source (T2CLKCON<CS3:CS0>, DS40001802H Register 29-1).
 *        T2CKIPPS selects pin RC3 at POR (the sim never ticks that row);
 *        remapping is not exposed by EPIC_PPS_MapInput.
 *        Codes 1110 and 1111 are reserved and have no enumerator.
 */
typedef enum {
    TIMER2_CS_T2CKIPPS   = 0x0U,
    TIMER2_CS_FOSC4      = 0x1U,
    TIMER2_CS_FOSC       = 0x2U,
    TIMER2_CS_HFINTOSC   = 0x3U,
    TIMER2_CS_LFINTOSC   = 0x4U,
    TIMER2_CS_MFINTOSC16 = 0x5U,
    TIMER2_CS_SOSC       = 0x6U,
    TIMER2_CS_CLKR       = 0x7U,
    TIMER2_CS_NCO        = 0x8U,
    TIMER2_CS_ZCD1       = 0x9U,
    TIMER2_CS_LC1OUT     = 0xAU,
    TIMER2_CS_LC2OUT     = 0xBU,
    TIMER2_CS_LC3OUT     = 0xCU,
    TIMER2_CS_LC4OUT     = 0xDU,
} TIMER2_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T2CON<CKPS2:CKPS0>, DS40001802H Register 29-2).
 *        Encoding N divides by 2^N, 000=1:1 through 111=1:128.
 */
typedef enum {
    TIMER2_PRESCALER_1_1   = 0x0U,
    TIMER2_PRESCALER_1_2   = 0x1U,
    TIMER2_PRESCALER_1_4   = 0x2U,
    TIMER2_PRESCALER_1_8   = 0x3U,
    TIMER2_PRESCALER_1_16  = 0x4U,
    TIMER2_PRESCALER_1_32  = 0x5U,
    TIMER2_PRESCALER_1_64  = 0x6U,
    TIMER2_PRESCALER_1_128 = 0x7U,
} TIMER2_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T2CON<OUTPS3:OUTPS0>, DS40001802H
 *        Register 29-2). Value N gives a 1:(N+1) divider.
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

/**
 * @brief Hardware limit mode (T2HLT<MODE4:MODE0>, DS40001802H
 *        Register 29-3 and Table 29-1). "Reset source" below is the
 *        T2RST signal (TMR2_ers). The reserved codes (0x10, 0x14, 0x15
 *        and 0x18 up) have no enumerator.
 */
typedef enum {
    /* Free running period: the timer runs until stopped by ON or ers. */
    TIMER2_MODE_FREE_SOFTWARE        = 0x00U, /**< Software gate: runs while ON. */
    TIMER2_MODE_FREE_GATE_HIGH       = 0x01U, /**< Hardware gate: runs while ers is high. */
    TIMER2_MODE_FREE_GATE_LOW        = 0x02U, /**< Hardware gate: runs while ers is low. */
    TIMER2_MODE_FREE_RESET_ANY_EDGE  = 0x03U, /**< Resets TMR2 on any ers edge. */
    TIMER2_MODE_FREE_RESET_RISING    = 0x04U, /**< Resets TMR2 on a rising ers edge. */
    TIMER2_MODE_FREE_RESET_FALLING   = 0x05U, /**< Resets TMR2 on a falling ers edge. */
    TIMER2_MODE_FREE_RESET_LOW       = 0x06U, /**< Held in reset while ers is low. */
    TIMER2_MODE_FREE_RESET_HIGH      = 0x07U, /**< Held in reset while ers is high. */
    /* One-shot: ON clears and TMR2 stops at 00h after the first period. */
    TIMER2_MODE_ONESHOT_SOFTWARE     = 0x08U, /**< Starts on ON. */
    TIMER2_MODE_ONESHOT_RISING       = 0x09U, /**< Starts on a rising ers edge. */
    TIMER2_MODE_ONESHOT_FALLING      = 0x0AU, /**< Starts on a falling ers edge. */
    TIMER2_MODE_ONESHOT_ANY_EDGE     = 0x0BU, /**< Starts on any ers edge. */
    TIMER2_MODE_ONESHOT_RISE_RST_RISE = 0x0CU, /**< Rising start and rising reset. */
    TIMER2_MODE_ONESHOT_FALL_RST_FALL = 0x0DU, /**< Falling start and falling reset. */
    TIMER2_MODE_ONESHOT_RISE_RST_LOW  = 0x0EU, /**< Rising start, low-level reset. */
    TIMER2_MODE_ONESHOT_FALL_RST_HIGH = 0x0FU, /**< Falling start, high-level reset. */
    /* Monostable: like one-shot, but ON stays set and ers restarts it. */
    TIMER2_MODE_MONO_RISING          = 0x11U,
    TIMER2_MODE_MONO_FALLING         = 0x12U,
    TIMER2_MODE_MONO_ANY_EDGE        = 0x13U,
    /* Level triggered one-shot with hardware reset. */
    TIMER2_MODE_ONESHOT_HIGH_START_LOW_RST = 0x16U,
    TIMER2_MODE_ONESHOT_LOW_START_HIGH_RST = 0x17U,
} TIMER2_ModeTypeDef;

/**
 * @brief External reset signal (T2RST<RSEL4:RSEL0>, DS40001802H
 *        Register 29-4). Code 00001 (TMR2_postscaled) is reserved on
 *        Timer2 and has no enumerator. T2AINPPS (the datasheet's T2INPPS) selects
 *        pin RC3 at POR; remap it with EPIC_PPS_MapInput(PPS_INPUT_T2IN, ...).
 */
typedef enum {
    TIMER2_RST_T2INPPS   = 0x00U,
    TIMER2_RST_TMR4_POST = 0x02U,
    TIMER2_RST_TMR6_POST = 0x03U,
    TIMER2_RST_CCP1_OUT  = 0x04U,
    TIMER2_RST_CCP2_OUT  = 0x05U,
    TIMER2_RST_CCP3_OUT  = 0x06U,
    TIMER2_RST_CCP4_OUT  = 0x07U,
    TIMER2_RST_CCP5_OUT  = 0x08U,
    TIMER2_RST_PWM6_OUT  = 0x09U,
    TIMER2_RST_PWM7_OUT  = 0x0AU,
    TIMER2_RST_C1OUT     = 0x0BU,
    TIMER2_RST_C2OUT     = 0x0CU,
    TIMER2_RST_ZCD1      = 0x0DU,
    TIMER2_RST_LC1OUT    = 0x0EU,
    TIMER2_RST_LC2OUT    = 0x0FU,
    TIMER2_RST_LC3OUT    = 0x10U,
    TIMER2_RST_LC4OUT    = 0x11U,
} TIMER2_ResetSourceTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER2_ClockSourceTypeDef ClockSource;
    TIMER2_PrescalerTypeDef   Prescaler;
    TIMER2_PostscalerTypeDef  Postscaler;
    TIMER2_ModeTypeDef        Mode;        /**< T2HLT.MODE hardware limit mode. */
    TIMER2_ResetSourceTypeDef ResetSource; /**< T2RST.RSEL external reset signal. */
    uint8_t                   FallingEdge; /**< T2HLT.CKPOL: nonzero clocks on the falling input edge. */
    uint8_t                   SyncOn;      /**< T2HLT.CKSYNC: nonzero synchronizes ON to the clock. */
    uint8_t                   SyncPrescaler; /**< T2HLT.PSYNC: nonzero synchronizes the prescaler output to FOSC/4. */
    uint8_t                   Period;      /**< PR2 value, 0..255. */
    /** @brief Optional match callback (fires on TMR2IF, i.e. every
     *         prescaler x (PR2+1) x postscaler input cycles). */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

#define TIMER2_HANDLE_DEFAULT {                                           \
    .ClockSource      = TIMER2_CS_FOSC4,                                  \
    .Prescaler        = TIMER2_PRESCALER_1_1,                             \
    .Postscaler       = TIMER2_POSTSCALER_1_1,                            \
    .Mode             = TIMER2_MODE_FREE_SOFTWARE,                        \
    .ResetSource      = TIMER2_RST_T2INPPS,                               \
    .FallingEdge      = 0U,                                               \
    .SyncOn           = 0U,                                               \
    .SyncPrescaler    = 0U,                                               \
    .Period           = 0xFFU,                                            \
    .OverflowCallback = NULL,                                             \
}

/**
 * @brief Configure Timer2 from the handle without starting it.
 * @details Halts the timer, programs T2CLKCON, T2RST, T2HLT and the
 * T2CON ratios (ON clear), writes PR2, clears TMR2IF and enables the
 * interrupt (plus PEIE) only with a callback. GIE is left alone.
 * @param h Handle with clock, ratios, mode, reset source, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Init(const TIMER2_HandleTypeDef *h);

/**
 * @brief Disable the TMR2 interrupt and restore the POR state (T2CON,
 * T2HLT, T2CLKCON and T2RST 0x00, TMR2 0x00, PR2 0xFF, callback dropped).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER2_DeInit(void);

/**
 * @brief Start Timer2: write PR2 before enabling ON to avoid a spurious
 * first match, zero TMR2 and program T2CON (ratios and ON) in one write.
 * @param h Handle holding the period, prescaler and postscaler (clock,
 *          mode, reset source and interrupt gate stay as programmed by Init).
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER2_Start(const TIMER2_HandleTypeDef *h);

/**
 * @brief Stop Timer2 by clearing T2CON<ON>, which resets the prescaler,
 * postscaler and mode state machine; TMR2 keeps its value.
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
 * (DS40001802H §29.1).
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
 * @brief Convert a prescaler enum to its integer ratio (1..128, 2^N).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);

/**
 * @brief Convert a postscaler enum to its integer ratio (1..16, N+1).
 * @param p Postscaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);

/**
 * @brief Timer2 ISR: clears TMR2IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR2IF and TMR2IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void TIMER2_IRQHandler(void);

#endif
