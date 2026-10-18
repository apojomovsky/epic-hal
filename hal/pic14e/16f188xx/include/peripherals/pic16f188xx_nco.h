/**
 * PIC16F188xx NCO1 driver (DS40001802D §24, Registers 24-1 to 24-8). A
 * 20-bit accumulator adds a 20-bit increment on every input clock; each
 * overflow is the raw output. Overflow frequency is
 * clock * increment / 2^20. Fixed Duty Cycle (FDC) mode toggles the output
 * on each overflow (50% duty, half the overflow rate); Pulse Frequency
 * (PFM) mode makes the output active for 1..128 input clocks per overflow.
 * The output reaches a pin through EPIC_PPS_MapOutput(PPS_OUTPUT_NCO1).
 * Reset clears every NCO register except NCO1INCL, which powers up 1.
 */
#ifndef PIC16F188XX_NCO_H
#define PIC16F188XX_NCO_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/** Largest value the 20-bit increment and accumulator hold. */
#define NCO_VALUE_MAX 0xFFFFFUL

/**
 * @brief Input clock (NCO1CLK<N1CKS2:N1CKS0>, DS40001802D Register 24-2).
 *        The LCx rows take a Configurable Logic Cell output, which this HAL
 *        does not drive yet. Values 6 and 7 are reserved.
 */
typedef enum {
    NCO_CLK_FOSC     = 0x0U,
    NCO_CLK_HFINTOSC = 0x1U,
    NCO_CLK_LC1OUT   = 0x2U,
    NCO_CLK_LC2OUT   = 0x3U,
    NCO_CLK_LC3OUT   = 0x4U,
    NCO_CLK_LC4OUT   = 0x5U,
} NCO_ClockSourceTypeDef;

/** @brief Output mode (NCO1CON<N1PFM>, Register 24-1). */
typedef enum {
    NCO_MODE_FDC = 0x0U,   /**< Fixed duty cycle: toggle per overflow. */
    NCO_MODE_PFM = 0x1U,   /**< Pulse frequency: pulse per overflow. */
} NCO_ModeTypeDef;

/**
 * @brief PFM pulse width (NCO1CLK<N1PWS2:N1PWS0>, Register 24-2). Value N
 *        gives 2^N input clocks of active output. A width longer than the
 *        overflow period makes the output undefined.
 */
typedef enum {
    NCO_PULSE_1   = 0x0U,
    NCO_PULSE_2   = 0x1U,
    NCO_PULSE_4   = 0x2U,
    NCO_PULSE_8   = 0x3U,
    NCO_PULSE_16  = 0x4U,
    NCO_PULSE_32  = 0x5U,
    NCO_PULSE_64  = 0x6U,
    NCO_PULSE_128 = 0x7U,
} NCO_PulseWidthTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    NCO_ClockSourceTypeDef ClockSource;
    NCO_ModeTypeDef        Mode;
    NCO_PulseWidthTypeDef  PulseWidth;   /**< PFM only; ignored in FDC. */
    uint8_t                Invert;       /**< Nonzero inverts the output (N1POL). */
    uint32_t               Increment;    /**< 0..NCO_VALUE_MAX. */
    /** @brief Optional overflow callback (fires on NCO1IF). */
    void (*OverflowCallback)(void);
} NCO_HandleTypeDef;

#define NCO_HANDLE_DEFAULT {                                              \
    .ClockSource      = NCO_CLK_FOSC,                                     \
    .Mode             = NCO_MODE_FDC,                                     \
    .PulseWidth       = NCO_PULSE_1,                                      \
    .Invert           = 0U,                                               \
    .Increment        = 1UL,                                              \
    .OverflowCallback = NULL,                                             \
}

/**
 * @brief Configure NCO1 from the handle, leaving it disabled.
 * @details Clears N1EN, programs clock, mode, pulse width, polarity and the
 * increment, and clears NCO1IF. The interrupt (plus PEIE) is enabled only
 * with a callback. The accumulator is left as it is.
 * @param h Handle with clock, mode, pulse width, polarity, increment and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle, a reserved or
 *         out-of-range field, or an increment above NCO_VALUE_MAX.
 */
EPIC_StatusTypeDef EPIC_NCO_Init(const NCO_HandleTypeDef *h);

/**
 * @brief Disable NCO1 and its interrupt and restore the POR state (control,
 * clock and accumulator 0, increment 1, callback dropped).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_DeInit(void);

/**
 * @brief Enable NCO1 (set N1EN), keeping the mode and polarity from Init.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_Start(void);

/**
 * @brief Disable NCO1 (clear N1EN), keeping the mode and polarity from Init.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_NCO_Stop(void);

/**
 * @brief Change the increment, also while running.
 * @details Writes NCO1INCU, NCO1INCH, then NCO1INCL: the write to the low
 * byte loads the double-buffered increment (§24.1.4), so the three bytes
 * take effect together.
 * @param increment New increment, 0..NCO_VALUE_MAX.
 * @return EPIC_OK on success, EPIC_INVALID when above NCO_VALUE_MAX
 *         (nothing is written).
 */
EPIC_StatusTypeDef EPIC_NCO_SetIncrement(uint32_t increment);

/**
 * @brief Read the increment registers.
 * @return NCO1INCU:NCO1INCH:NCO1INCL.
 */
uint32_t EPIC_NCO_GetIncrement(void);

/**
 * @brief Read the accumulator.
 * @details The three bytes are read low to high with no atomic snapshot, so
 * while the NCO runs the value may straddle an addition (§24.8, Register
 * 24-5 note). Stop the NCO for an exact read.
 * @return NCO1ACCU:NCO1ACCH:NCO1ACCL.
 */
uint32_t EPIC_NCO_ReadAccumulator(void);

/**
 * @brief Load the accumulator while the NCO is stopped.
 * @details A write while the NCO runs gives undefined results (Register
 * 24-5 note), so it is refused.
 * @param value New accumulator value, 0..NCO_VALUE_MAX.
 * @return EPIC_OK on success, EPIC_BUSY while N1EN is set, EPIC_INVALID when
 *         above NCO_VALUE_MAX (nothing is written in either case).
 */
EPIC_StatusTypeDef EPIC_NCO_WriteAccumulator(uint32_t value);

/**
 * @brief Read the live output level (NCO1CON<N1OUT>, polarity applied).
 * @return 1 when the output is high, 0 otherwise.
 */
uint8_t EPIC_NCO_ReadOutput(void);

/**
 * @brief NCO1 ISR: clears NCO1IF and runs the callback, if any.
 * @details Called from epic_dispatch_all_irqs only when NCO1IF and NCO1IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void NCO_IRQHandler(void);

#endif /* PIC16F188XX_NCO_H */
