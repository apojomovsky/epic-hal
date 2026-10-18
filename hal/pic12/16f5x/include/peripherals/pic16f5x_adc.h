/* 16F506 ADC driver (DS41268D §9.0, Register 9-1/9-2). 8-bit
 * result in ADRES, VDD reference (§9.1.1), no interrupt vector on
 * this core (DS41213D §4.0). The handle mirrors the pic14 ADC shape
 * minus Reference (fixed VDD), ResultFormat (fixed 8-bit) and the
 * callback (no IRQ): Init owns ADCON0 outright, including the ANS
 * analog-pin cover for the selected channel, so coordinate with the
 * comparator driver on the shared RB0/RB1 pins. Only the 16F506
 * carries this block. */

#ifndef PIC16F5X_ADC_H
#define PIC16F5X_ADC_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"

/**
 * @brief Analog channel select (ADCON0<CHS>, Register 9-1).
 *        CHS=11 samples the internal 0.6V reference, no pin needed.
 */
typedef enum {
    ADC_CHANNEL_AN0      = 0x0U,   /**< CHS=00, RB0. */
    ADC_CHANNEL_AN1      = 0x1U,   /**< CHS=01, RB1. */
    ADC_CHANNEL_AN2      = 0x2U,   /**< CHS=10, RB2. */
    ADC_CHANNEL_VREF_0V6 = 0x3U,   /**< CHS=11, 0.6V reference. */
} ADC_ChannelTypeDef;

/**
 * @brief Conversion clock select (ADCON0<ADCS>, §9.1/Tables 9-1/9-2).
 *        13 TAD per conversion; TAD must stay within 500 ns..50 us,
 *        so scale the divisor with Fosc (FOSC_8 fits the 4 MHz default).
 */
typedef enum {
    ADC_CLOCK_FOSC_16  = 0x0U,   /**< ADCS=00, Fosc/16. */
    ADC_CLOCK_FOSC_8   = 0x1U,   /**< ADCS=01, Fosc/8. */
    ADC_CLOCK_FOSC_4   = 0x2U,   /**< ADCS=10, Fosc/4. */
    ADC_CLOCK_INTOSC_4 = 0x3U,   /**< ADCS=11, INTOSC/4. */
} ADC_ClockSourceTypeDef;

/** Driver handle (Cube-style). No callback field: no IRQ exists. */
typedef struct {
    ADC_ChannelTypeDef     Channel;      /* ADCON0<CHS>. */
    ADC_ClockSourceTypeDef ClockSource;  /* ADCON0<ADCS>. */
} ADC_HandleTypeDef;

/** Default initialiser: AN0 at Fosc/8 (TAD=2 us at 4 MHz, in range). */
#define ADC_HANDLE_DEFAULT {                                              \
    .Channel            = ADC_CHANNEL_AN0,                                 \
    .ClockSource        = ADC_CLOCK_FOSC_8,                                \
}

/**
 * @brief  Configure the ADC from the handle: ADON set, GO clear, CHS
 *         and ADCS from the handle, ANS covering the channel pin
 *         (Register 9-1: AN0 needs ANS=10, AN1 ANS=11, AN2 ANS=01;
 *         VREF_0V6 leaves ANS unchanged, no pin is sampled).
 * @param h handle with channel and clock selection.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);

/**
 * @brief  Shut the ADC off and return its pins to digital (ADCON0
 *         reads 0x00, the same analog-off state GPIO init leaves).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);

/**
 * @brief  Switch channel: programs CHS plus the ANS cover for the
 *         new channel (VREF_0V6 programs CHS only). Call with no
 *         conversion in progress: ANS must not change mid-conversion
 *         (§9.1.2), a CHS change mid-conversion applies after it.
 * @param ch the channel to sample next.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);

/**
 * @brief  Start a conversion by setting GO/DONE. Hardware refuses GO
 *         while ADON is clear (Register 9-1), so starting shut off
 *         reports busy too.
 * @return 0 on success, 0xFFFF if a conversion is running or ADON
 *         is clear.
 */
uint16_t EPIC_ADC_Start(void);

/**
 * @brief Report whether a conversion is in progress.
 * @return 1 if GO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void);

/**
 * @brief Read the last conversion result.
 * @return the ADRES byte (0..255, VDD reference).
 */
uint8_t EPIC_ADC_Read(void);

#endif /* PIC16F5X_ADC_H */
