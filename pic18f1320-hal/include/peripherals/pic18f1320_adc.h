/*
 * 10-bit SAR ADC (DS39605F §17.0), 7 channels (AN0-AN6). Mirrors
 * `pic18fxx5x_adc.h`'s API but with the 1320's genuine register-layout
 * differences: VCFG1:VCFG0 live in ADCON0 bits 7:6 (not ADCON1, which on
 * this part is a 7-bit per-pin PCFG with no VCFG), and the channel field
 * is CHS2:CHS0 (3 bits, ADCON0 bits 4:2). ADCON2 (ADFM/ACQT/ADCS) matches
 * the 4550.
 */

#ifndef PIC18F1320_ADC_H
#define PIC18F1320_ADC_H

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"

/**
 * @brief A/D channel (ADCON0<CHS2:CHS0>, Register 17-1). This part has
 *        7 channels AN0..AN6 (CHS is 3 bits); value 111 (7) is
 *        unimplemented.
 */
typedef enum {
    ADC_CHANNEL_AN0  = 0x0U,
    ADC_CHANNEL_AN1  = 0x1U,
    ADC_CHANNEL_AN2  = 0x2U,
    ADC_CHANNEL_AN3  = 0x3U,
    ADC_CHANNEL_AN4  = 0x4U,
    ADC_CHANNEL_AN5  = 0x5U,
    ADC_CHANNEL_AN6  = 0x6U,
} ADC_ChannelTypeDef;

/**
 * @brief A/D conversion clock (ADCON2<ADCS2:ADCS0>, Register 17-3).
 *        011 and 111 both select the internal A/D RC (FRC) clock.
 */
typedef enum {
    ADC_CLOCK_FOSC_2  = 0x0U,   /**< 000, Fosc/2.   */
    ADC_CLOCK_FOSC_8  = 0x1U,   /**< 001, Fosc/8.   */
    ADC_CLOCK_FOSC_32 = 0x2U,   /**< 010, Fosc/32.  */
    ADC_CLOCK_FRC     = 0x3U,   /**< 011, internal A/D RC. */
    ADC_CLOCK_FOSC_4  = 0x4U,   /**< 100, Fosc/4.   */
    ADC_CLOCK_FOSC_16 = 0x5U,   /**< 101, Fosc/16.  */
    ADC_CLOCK_FOSC_64 = 0x6U,   /**< 110, Fosc/64.  */
    ADC_CLOCK_FRC2    = 0x7U,   /**< 111, internal A/D RC (alias of 011). */
} ADC_ClockSourceTypeDef;

/**
 * @brief A/D acquisition time (ADCON2<ACQT2:ACQT0>, Register 17-3).
 *        000 = 0 Tad (manual; caller must guarantee acquisition).
 */
typedef enum {
    ADC_ACQ_0TAD  = 0x0U,
    ADC_ACQ_2TAD  = 0x1U,
    ADC_ACQ_4TAD  = 0x2U,
    ADC_ACQ_6TAD  = 0x3U,
    ADC_ACQ_8TAD  = 0x4U,
    ADC_ACQ_12TAD = 0x5U,
    ADC_ACQ_16TAD = 0x6U,
    ADC_ACQ_20TAD = 0x7U,
} ADC_AcquisitionTypeDef;

/**
 * @brief Result-format select (ADCON2<ADFM>, Register 17-3).
 */
typedef enum {
    ADC_FORMAT_LEFT  = 0x0U,   /**< ADFM=0, left justified. */
    ADC_FORMAT_RIGHT = 0x1U,   /**< ADFM=1, right justified. */
} ADC_ResultFormatTypeDef;

/**
 * @brief Voltage reference (ADCON0<VCFG1:VCFG0>, Register 17-1). On this
 *        part the VCFG bits live in ADCON0 (bit 6 = VCFG0 selects Vref+
 *        AN3 vs VDD; bit 7 = VCFG1 selects Vref- AN2 vs VSS), not in
 *        ADCON1 as on the 4550.
 */
typedef enum {
    ADC_VREF_VDD_VSS  = 0x00U,                       /**< Vref+=VDD, Vref-=VSS.  */
    ADC_VREF_AN3_VSS  = PIC_ADCON0_VCFG0,            /**< Vref+=AN3, Vref-=VSS.  */
    ADC_VREF_VDD_AN2  = PIC_ADCON0_VCFG1,            /**< Vref+=VDD, Vref-=AN2.  */
    ADC_VREF_AN3_AN2  = (PIC_ADCON0_VCFG0 | PIC_ADCON0_VCFG1), /**< Vref+=AN3, Vref-=AN2. */
} ADC_VReferenceTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    ADC_ChannelTypeDef         Channel;
    ADC_ClockSourceTypeDef      ClockSource;
    ADC_AcquisitionTypeDef      Acquisition;
    ADC_ResultFormatTypeDef     ResultFormat;
    ADC_VReferenceTypeDef       VReference;
    uint8_t                     PinConfig;      /**< PCFG6:PCFG0 (bit per pin), 0..127. */
    /** @brief Optional conversion-complete callback (fires on ADIF). */
    void (*ConvCpltCallback)(uint16_t result);
} ADC_HandleTypeDef;

#define ADC_HANDLE_DEFAULT {                                              \
    .Channel          = ADC_CHANNEL_AN0,                                  \
    .ClockSource      = ADC_CLOCK_FOSC_8,                                 \
    .Acquisition      = ADC_ACQ_2TAD,                                     \
    .ResultFormat     = ADC_FORMAT_RIGHT,                                 \
    .VReference       = ADC_VREF_VDD_VSS,                                 \
    .PinConfig        = 0x0U,                                              \
    .ConvCpltCallback = NULL,                                             \
}

/**
 * @brief  Configure the A/D converter from a handle: channel select,
 *         clock, acquisition time, result format and voltage reference,
 *         then the 7-bit pin configuration (ADCON1) and module enable
 *         (ADCON0<ADON>). Does not start a conversion.
 * @param h the ADC handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);

/**
 * @brief  Disable the A/D converter: disable its interrupt, clear the
 *         flag, restore ADCON0/1/2 to power-on values and drop the stored
 *         handle.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);

/**
 * @brief  Select the analog input channel without starting a conversion.
 * @param ch the channel to select (an @ref ADC_ChannelTypeDef value).
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);

/**
 * @brief  Start an A/D conversion by setting ADCON0<GO/DONE>. Call after
 *         selecting the channel and waiting the acquisition time.
 * @return 0 on success, 0xFFFF if a conversion is already in progress.
 */
uint16_t EPIC_ADC_Start(void);

/**
 * @brief  Return 1 if a conversion is in progress (GO/DONE = 1).
 * @return 1 while a conversion runs, else 0.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void);

/**
 * @brief Returns 1 if the latest conversion has completed (ADIF = 1).
 * @return 1 when the conversion-complete flag is set, else 0.
 */
uint8_t EPIC_ADC_IsConversionDone(void);

/**
 * @brief Clear the ADIF flag (must be called in the conversion-complete IRQ).
 */
void EPIC_ADC_ClearITFlag(void);

/**
 * @brief  Read the latest 10-bit result. Returns 0..1023; left-justified
 *         results (ADFM=0) are shifted down so the caller always gets a
 *         0..1023 value.
 * @return the latest conversion result, 0..1023.
 */
uint16_t EPIC_ADC_Read(void);

/**
 * @brief ADC conversion-complete interrupt handler (weak default).
 */
void ADC_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F1320_ADC_H */
