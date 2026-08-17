/* A/D converter driver, 10-bit, 11/14 channels + CVREF + VP6. Source:
 * DS40001291H §9.0; full register/wiring reference:
 * pic16f88x-hal/MANUAL.md §ADC. The 88X ADC uses ADCON0<CHS3:CHS0>
 * (4-bit channel select), ADCON0<GO/DONE> at bit 1, and ADCON1<VCFG1:
 * VCFG0> for the external VREF pins; the analog input pins are selected
 * per-pin through ANSEL/ANSELH (Bank 3), not ADCON1<PCFG> like the
 * 87XA. */

#ifndef PIC16F88X_ADC_H
#define PIC16F88X_ADC_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief A/D channel (ADCON0<CHS3:CHS0>, Register 9-1).
 *        AN0..AN4 on all parts; AN5..AN7 only on 40/44-pin (RE0..RE2);
 *        AN8..AN13 on RB2/RB3/RB1/RB4/RB0/RB5 all parts; CVREF and VP6
 *        (fixed 0.6 V reference) are internal.
 */
typedef enum {
    ADC_CHANNEL_AN0   = 0x0U,
    ADC_CHANNEL_AN1   = 0x1U,
    ADC_CHANNEL_AN2   = 0x2U,
    ADC_CHANNEL_AN3   = 0x3U,
    ADC_CHANNEL_AN4   = 0x4U,
    ADC_CHANNEL_AN5   = 0x5U,   /* 40/44-pin only. */
    ADC_CHANNEL_AN6   = 0x6U,   /* 40/44-pin only. */
    ADC_CHANNEL_AN7   = 0x7U,   /* 40/44-pin only. */
    ADC_CHANNEL_AN8   = 0x8U,
    ADC_CHANNEL_AN9   = 0x9U,
    ADC_CHANNEL_AN10  = 0xAU,
    ADC_CHANNEL_AN11  = 0xBU,
    ADC_CHANNEL_AN12  = 0xCU,
    ADC_CHANNEL_AN13  = 0xDU,
    ADC_CHANNEL_CVREF = 0xEU,   /**< Internal CVREF (comparator reference). */
    ADC_CHANNEL_VP6   = 0xFU,   /**< Fixed 0.6 V reference (FVR). */
} ADC_ChannelTypeDef;

/**
 * @brief A/D clock source (ADCON0<ADCS1:ADCS0>, Register 9-1).
 */
typedef enum {
    ADC_CLOCK_FOSC_2     = 0x0U,   /**< 00, Fosc/2.  */
    ADC_CLOCK_FOSC_8     = 0x1U,   /**< 01, Fosc/8.  */
    ADC_CLOCK_FOSC_32    = 0x2U,   /**< 10, Fosc/32. */
    ADC_CLOCK_RC         = 0x3U,   /**< 11, Internal A/D RC (500 kHz max). */
} ADC_ClockSourceTypeDef;

/**
 * @brief Result-format select (ADCON1<ADFM>, Register 9-2).
 */
typedef enum {
    ADC_FORMAT_LEFT  = 0x0U,   /**< ADFM=0, left justified. */
    ADC_FORMAT_RIGHT = 0x1U,   /**< ADFM=1, right justified. */
} ADC_ResultFormatTypeDef;

/**
 * @brief Voltage reference configuration (ADCON1<VCFG1:VCFG0>, §9.0
 *        Register 9-2). Unlike the 87XA's PCFG table, the 88X selects
 *        the VREF+ / VREF- sources independently; the analog channel
 *        pins are always enabled per-pin through ANSEL/ANSELH.
 */
typedef enum {
    ADC_REFERENCE_VDD_VSS      = 0x0U,   /**< VCFG1:VCFG0 = 00, Vref+ = Vdd, Vref- = Vss. */
    ADC_REFERENCE_VDD_VREFN    = 0x1U,   /**< 01, Vref+ = Vdd, Vref- = VREF- pin. */
    ADC_REFERENCE_VREFP_VSS    = 0x2U,   /**< 10, Vref+ = VREF+ pin, Vref- = Vss. */
    ADC_REFERENCE_VREFP_VREFN  = 0x3U,   /**< 11, Vref+ = VREF+ pin, Vref- = VREF- pin. */
} ADC_ReferenceTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    ADC_ChannelTypeDef          Channel;
    ADC_ClockSourceTypeDef      ClockSource;
    ADC_ResultFormatTypeDef     ResultFormat;
    ADC_ReferenceTypeDef        Reference;
    /** @brief Optional conversion-complete callback (fires on ADIF). */
    void (*ConvCpltCallback)(uint16_t result);
} ADC_HandleTypeDef;

#define ADC_HANDLE_DEFAULT {                                              \
    .Channel         = ADC_CHANNEL_AN0,                                    \
    .ClockSource     = ADC_CLOCK_FOSC_8,                                   \
    .ResultFormat    = ADC_FORMAT_RIGHT,                                  \
    .Reference       = ADC_REFERENCE_VDD_VSS,                             \
    .ConvCpltCallback = NULL,                                             \
}

/* init / deinit. */

/**
 * @brief  Initialize the A/D converter with the given handle.
 *         Programs ADCON0/ADCON1 (clock, format, reference) and
 *         installs the conversion-complete callback.
 * @param h handle with Channel, ClockSource, ResultFormat, Reference.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);

/**
 * @brief  De-initialize the A/D converter. Clears the callback and
 *         returns ADCON0/ADCON1 to their reset state.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);

/* conversion control. */

/**
 * @brief  Select the analog channel and start a conversion.
 *         Sets ADCON0<GO/DONE> = 1. The user is expected to:
 *           1. Select the channel (with EPIC_ADC_SelectChannel)
 *           2. Wait for the acquisition time (§9.1)
 *           3. Call EPIC_ADC_Start() to begin conversion
 *           4. Poll EPIC_ADC_IsConversionDone() or wait for the IRQ
 *
 *         Returns 0xFFFF if a conversion was already in progress
 *         (GO/DONE was 1).
 * @return 0xFFFF if a conversion was already running, else the raw
 *         ADRESH/ADRESL pair (low byte undefined until conversion ends).
 */
uint16_t EPIC_ADC_Start(void);

/**
 * @brief Select the channel without starting conversion.
 * @param ch the analog channel to select.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);

/**
 * @brief  Configure the ANSEL/ANSELH bits for one analog channel,
 *         releasing its pin to the ADC (DS40001291H §9.0, Registers
 *         3-3/3-4). Internal channels (CVREF, VP6) have no pin.
 * @param ch the channel whose pin to enable.
 */
void EPIC_ADC_ConfigChannel(ADC_ChannelTypeDef ch);

/**
 * @brief Returns 1 if a conversion is in progress (GO/DONE = 1).
 * @return 1 if a conversion is in progress, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void);

/**
 * @brief Returns 1 if the latest conversion has completed (ADIF = 1).
 * @return 1 if the conversion has completed, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionDone(void);

/**
 * @brief Clear the ADIF flag. Must be called in the conversion-complete IRQ.
 */
void EPIC_ADC_ClearITFlag(void);

/* result. */

/**
 * @brief  Read the latest 10-bit result. Returns 0..1023 in right-
 *         justified format; left-justified results are shifted down
 *         to 0..1023.
 * @return the latest 10-bit conversion result, 0..1023.
 */
uint16_t EPIC_ADC_Read(void);

/* interrupts. */

/**
 * @brief Weak ADC ISR, override in user code.
 */
void ADC_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F88X_ADC_H */
