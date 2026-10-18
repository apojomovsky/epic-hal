/**
 * PIC16F188xx Timer4 driver, basic free-running mode (DS40001802H §29,
 * Registers 29-1..29-4). 8-bit timer: PR4 match, 1:1..1:128 prescaler,
 * 1:1..1:16 postscaler and T4CLKCON clock select. TMR4 counts 0..PR4 and
 * clears on the next clock; TMR4IF (PIR4) fires every prescaler x (PR4+1)
 * x postscaler cycles. Timer4 is the PWM time base of the CCP and PWM
 * Timer3/Timer4 couple (CCPTMRS value 10). The T4HLT hardware limit modes
 * are not driven: Init always leaves T4HLT and T4RST at 0 (software mode).
 */
#ifndef PIC16F188XX_TIMER4_H
#define PIC16F188XX_TIMER4_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/**
 * @brief Clock source (T4CLKCON<CS3:CS0>, DS40001802H Register 29-1). The
 *        encoding is Timer2's: both timers are one module (EDC _modsrc
 *        DOS-02458_tmr_mid2_v1). T4CKIPPS selects its POR pin; remapping
 *        is not exposed by EPIC_PPS_MapInput. Codes 1110 and 1111 are
 *        reserved and have no enumerator.
 */
typedef enum {
    TIMER4_CS_T4CKIPPS   = 0x0U,
    TIMER4_CS_FOSC4      = 0x1U,
    TIMER4_CS_FOSC       = 0x2U,
    TIMER4_CS_HFINTOSC   = 0x3U,
    TIMER4_CS_LFINTOSC   = 0x4U,
    TIMER4_CS_MFINTOSC16 = 0x5U,
    TIMER4_CS_SOSC       = 0x6U,
    TIMER4_CS_CLKR       = 0x7U,
    TIMER4_CS_NCO        = 0x8U,
    TIMER4_CS_ZCD1       = 0x9U,
    TIMER4_CS_LC1OUT     = 0xAU,
    TIMER4_CS_LC2OUT     = 0xBU,
    TIMER4_CS_LC3OUT     = 0xCU,
    TIMER4_CS_LC4OUT     = 0xDU,
} TIMER4_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T4CON<CKPS2:CKPS0>, DS40001802H Register 29-2).
 *        Encoding N divides by 2^N, 000=1:1 through 111=1:128.
 */
typedef enum {
    TIMER4_PRESCALER_1_1   = 0x0U,
    TIMER4_PRESCALER_1_2   = 0x1U,
    TIMER4_PRESCALER_1_4   = 0x2U,
    TIMER4_PRESCALER_1_8   = 0x3U,
    TIMER4_PRESCALER_1_16  = 0x4U,
    TIMER4_PRESCALER_1_32  = 0x5U,
    TIMER4_PRESCALER_1_64  = 0x6U,
    TIMER4_PRESCALER_1_128 = 0x7U,
} TIMER4_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T4CON<OUTPS3:OUTPS0>, DS40001802H
 *        Register 29-2). Value N gives a 1:(N+1) divider.
 */
typedef enum {
    TIMER4_POSTSCALER_1_1  = 0x0U,
    TIMER4_POSTSCALER_1_2  = 0x1U,
    TIMER4_POSTSCALER_1_3  = 0x2U,
    TIMER4_POSTSCALER_1_4  = 0x3U,
    TIMER4_POSTSCALER_1_5  = 0x4U,
    TIMER4_POSTSCALER_1_6  = 0x5U,
    TIMER4_POSTSCALER_1_7  = 0x6U,
    TIMER4_POSTSCALER_1_8  = 0x7U,
    TIMER4_POSTSCALER_1_9  = 0x8U,
    TIMER4_POSTSCALER_1_10 = 0x9U,
    TIMER4_POSTSCALER_1_11 = 0xAU,
    TIMER4_POSTSCALER_1_12 = 0xBU,
    TIMER4_POSTSCALER_1_13 = 0xCU,
    TIMER4_POSTSCALER_1_14 = 0xDU,
    TIMER4_POSTSCALER_1_15 = 0xEU,
    TIMER4_POSTSCALER_1_16 = 0xFU,
} TIMER4_PostscalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER4_ClockSourceTypeDef ClockSource;
    TIMER4_PrescalerTypeDef   Prescaler;
    TIMER4_PostscalerTypeDef  Postscaler;
    uint8_t                   Period;      /**< PR4 value, 0..255. */
    /** @brief Optional match callback (fires on TMR4IF, i.e. every
     *         prescaler x (PR4+1) x postscaler input cycles). */
    void (*OverflowCallback)(void);
} TIMER4_HandleTypeDef;

#define TIMER4_HANDLE_DEFAULT {                                           \
    .ClockSource      = TIMER4_CS_FOSC4,                                  \
    .Prescaler        = TIMER4_PRESCALER_1_1,                             \
    .Postscaler       = TIMER4_POSTSCALER_1_1,                            \
    .Period           = 0xFFU,                                            \
    .OverflowCallback = NULL,                                             \
}

/**
 * @brief Configure Timer4 from the handle without starting it.
 * @details Halts the timer, programs T4CLKCON, zeroes T4RST and T4HLT
 * (software mode), writes the T4CON ratios (ON clear) and PR4, clears
 * TMR4IF and enables the interrupt (plus PEIE) only with a callback. GIE
 * is left alone.
 * @param h Handle with clock, ratios, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Init(const TIMER4_HandleTypeDef *h);

/**
 * @brief Disable the TMR4 interrupt and restore the POR state (T4CON,
 * T4HLT, T4CLKCON and T4RST 0x00, TMR4 0x00, PR4 0xFF, callback dropped).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER4_DeInit(void);

/**
 * @brief Start Timer4: write PR4 before enabling ON to avoid a spurious
 * first match, zero TMR4 and program T4CON (ratios and ON) in one write.
 * @param h Handle holding the period, prescaler and postscaler (clock and
 *          interrupt gate stay as programmed by Init).
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Start(const TIMER4_HandleTypeDef *h);

/**
 * @brief Stop Timer4 by clearing T4CON<ON>, which resets the prescaler and
 * postscaler; TMR4 keeps its value.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER4_Stop(void);

/**
 * @brief Read the current TMR4 counter value.
 * @return TMR4 value, 0..255.
 */
uint8_t EPIC_TIMER4_ReadCounter(void);

/**
 * @brief Write the TMR4 counter value.
 * @details A write also clears the prescaler and postscaler counters
 * (DS40001802H §29.1).
 * @param value Counter value, 0..255.
 */
void EPIC_TIMER4_WriteCounter(uint8_t value);

/**
 * @brief Read the PR4 period register.
 * @return PR4 value, 0..255.
 */
uint8_t EPIC_TIMER4_ReadPeriod(void);

/**
 * @brief Write the PR4 period register.
 * @param period Period value, 0..255.
 */
void EPIC_TIMER4_WritePeriod(uint8_t period);

/**
 * @brief Convert a prescaler enum to its integer ratio (1..128, 2^N).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER4_PrescalerToRatio(TIMER4_PrescalerTypeDef p);

/**
 * @brief Convert a postscaler enum to its integer ratio (1..16, N+1).
 * @param p Postscaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER4_PostscalerToRatio(TIMER4_PostscalerTypeDef p);

/**
 * @brief Timer4 ISR: clears TMR4IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR4IF and TMR4IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void TIMER4_IRQHandler(void);

#endif
