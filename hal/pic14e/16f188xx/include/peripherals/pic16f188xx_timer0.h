/**
 * PIC16F188xx Timer0 driver (DS40001825F §27, Registers 27-1/27-2: the
 * sibling 18857/77 datasheet documents this exact Timer0 module, EDC
 * _modsrc DOS-01631_tmr_mid0_v1_8bit on both). 8/16-bit timer with
 * independent prescaler (1:1..1:32768), output postscaler (1:1..1:16)
 * and overflow flag PIR0.TMR0IF. In 8-bit mode TMR0L counts 0..TMR0H
 * and resets on match; in 16-bit mode TMR0H:TMR0L rolls over.
 */
#ifndef PIC16F188XX_TIMER0_H
#define PIC16F188XX_TIMER0_H

#include <stddef.h>
#include <stdint.h>
#include "pic16f188xx.h"

/**
 * @brief Clock source (T0CON1<T0CS2:T0CS0>, DS40001825F Register 27-2).
 *        T0CKIPPS needs a PPS pin mapping, which this HAL does not set
 *        up yet, so those two rows count but never tick in the sim.
 */
typedef enum {
    TIMER0_CS_T0CKIPPS      = 0x0U,
    TIMER0_CS_T0CKIPPS_INV  = 0x1U,
    TIMER0_CS_FOSC4         = 0x2U,
    TIMER0_CS_HFINTOSC      = 0x3U,
    TIMER0_CS_LFINTOSC      = 0x4U,
    TIMER0_CS_SOSC          = 0x5U,
    TIMER0_CS_LC1OUT        = 0x6U,
} TIMER0_ClockSourceTypeDef;

/**
 * @brief Prescaler ratio (T0CON1<T0CKPS3:T0CKPS0>, DS40001825F
 *        Register 27-2). Encoding N divides by 2^N, 0000=1:1
 *        through 1111=1:32768.
 */
typedef enum {
    TIMER0_PRESCALER_1_1     = 0x0U,
    TIMER0_PRESCALER_1_2     = 0x1U,
    TIMER0_PRESCALER_1_4     = 0x2U,
    TIMER0_PRESCALER_1_8     = 0x3U,
    TIMER0_PRESCALER_1_16    = 0x4U,
    TIMER0_PRESCALER_1_32    = 0x5U,
    TIMER0_PRESCALER_1_64    = 0x6U,
    TIMER0_PRESCALER_1_128   = 0x7U,
    TIMER0_PRESCALER_1_256   = 0x8U,
    TIMER0_PRESCALER_1_512   = 0x9U,
    TIMER0_PRESCALER_1_1024  = 0xAU,
    TIMER0_PRESCALER_1_2048  = 0xBU,
    TIMER0_PRESCALER_1_4096  = 0xCU,
    TIMER0_PRESCALER_1_8192  = 0xDU,
    TIMER0_PRESCALER_1_16384 = 0xEU,
    TIMER0_PRESCALER_1_32768 = 0xFU,
} TIMER0_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T0CON0<T0OUTPS3:T0OUTPS0>, DS40001825F
 *        Register 27-1). Value N gives a 1:(N+1) divider.
 */
typedef enum {
    TIMER0_POSTSCALER_1_1  = 0x0U,
    TIMER0_POSTSCALER_1_2  = 0x1U,
    TIMER0_POSTSCALER_1_3  = 0x2U,
    TIMER0_POSTSCALER_1_4  = 0x3U,
    TIMER0_POSTSCALER_1_5  = 0x4U,
    TIMER0_POSTSCALER_1_6  = 0x5U,
    TIMER0_POSTSCALER_1_7  = 0x6U,
    TIMER0_POSTSCALER_1_8  = 0x7U,
    TIMER0_POSTSCALER_1_9  = 0x8U,
    TIMER0_POSTSCALER_1_10 = 0x9U,
    TIMER0_POSTSCALER_1_11 = 0xAU,
    TIMER0_POSTSCALER_1_12 = 0xBU,
    TIMER0_POSTSCALER_1_13 = 0xCU,
    TIMER0_POSTSCALER_1_14 = 0xDU,
    TIMER0_POSTSCALER_1_15 = 0xEU,
    TIMER0_POSTSCALER_1_16 = 0xFU,
} TIMER0_PostscalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER0_ClockSourceTypeDef ClockSource;
    TIMER0_PrescalerTypeDef   Prescaler;
    TIMER0_PostscalerTypeDef  Postscaler;
    uint8_t                   Is16Bit;   /**< Nonzero for 16-bit mode. */
    uint16_t                  Period;    /**< 8-bit: TMR0H match value.
                                          *   16-bit: Start preload. */
    /** @brief Optional overflow callback (fires on TMR0IF, i.e. every
     *         prescaler x range x postscaler input cycles). */
    void (*OverflowCallback)(void);
} TIMER0_HandleTypeDef;

#define TIMER0_HANDLE_DEFAULT {                                           \
    .ClockSource      = TIMER0_CS_FOSC4,                                  \
    .Prescaler        = TIMER0_PRESCALER_1_1,                              \
    .Postscaler       = TIMER0_POSTSCALER_1_1,                             \
    .Is16Bit          = 0U,                                               \
    .Period           = 0xFFU,                                             \
    .OverflowCallback = NULL,                                              \
}

/**
 * @brief Configure Timer0 from the handle without starting it.
 * @details Programs the control registers and period, clears TMR0IF,
 * enables the interrupt (plus PEIE) only with a callback; T0ASYNC
 * follows the source (async rows set, FOSC/4 and HFINTOSC clear).
 * @param h Handle with source, ratios, mode, period and callback.
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Init(const TIMER0_HandleTypeDef *h);

/**
 * @brief Disable the TMR0 interrupt and restore the POR state (T0CON0 and
 * T0CON1 0x00, TMR0L 0x00, TMR0H 0xFF, callback dropped).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void);

/**
 * @brief Start Timer0 from the handle: reload the period, zero the 8-bit
 * counter (or preload it with Period in 16-bit mode) and set T0EN.
 * @param h Handle holding the period, width and postscaler (clock source,
 *          prescaler and interrupt gate stay as programmed by Init).
 * @return EPIC_OK on success, EPIC_INVALID for a NULL or out-of-range handle.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h);

/**
 * @brief Stop Timer0 by clearing T0CON0<T0EN>.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void);

/**
 * @brief Read the 16-bit counter (TMR0L first, which latches TMR0H on silicon).
 * @return TMR0H:TMR0L; in 8-bit mode the high byte is the match value, not time.
 */
uint16_t EPIC_TIMER0_ReadCounter(void);

/**
 * @brief Write the counter: TMR0H then TMR0L in 16-bit mode, TMR0L only in
 * 8-bit mode (TMR0H holds the match value there and must survive).
 * @param value Counter value.
 */
void EPIC_TIMER0_WriteCounter(uint16_t value);

/**
 * @brief Convert a prescaler enum to its integer ratio (1..32768, 2^N).
 * @param p Prescaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p);

/**
 * @brief Convert a postscaler enum to its integer ratio (1..16, N+1).
 * @param p Postscaler selection.
 * @return Divider ratio, or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PostscalerToRatio(TIMER0_PostscalerTypeDef p);

/**
 * @brief Timer0 ISR: clears TMR0IF and runs the owned callback, if any.
 * @details Called from epic_dispatch_all_irqs only when TMR0IF and TMR0IE
 * are both set. Keeps no cstack autos (ISR bank rule, family MANUAL.md).
 */
void TIMER0_IRQHandler(void);

#endif
