/*
 * 10-bit SAR ADC (DS39609B §19.0), 12 channels on this part
 * (AN0-AN11, CHS3:CHS0). Same three-register shape as the 4550/2520:
 * ADCON0 (channel + GO/DONE + ADON), ADCON1 (VCFG1:VCFG0 + PCFG3:PCFG0),
 * ADCON2 (ADFM + ADCS only; no ACQT on this part). PinConfig is the raw
 * 4-bit PCFG code from Table 19-4, not a curated enum.
 */

#ifndef PIC18F6520_ADC_H
#define PIC18F6520_ADC_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"

/**
 * @brief A/D channel (ADCON0<CHS3:CHS0>, Register 19-1).
 *        AN0-AN11 are implemented on this part; values 0x0C..0x0F are
 *        unimplemented (12 channels total).
 */
typedef enum {
    ADC_CHANNEL_AN0  = 0x0U,
    ADC_CHANNEL_AN1  = 0x1U,
    ADC_CHANNEL_AN2  = 0x2U,
    ADC_CHANNEL_AN3  = 0x3U,
    ADC_CHANNEL_AN4  = 0x4U,
    ADC_CHANNEL_AN5  = 0x5U,
    ADC_CHANNEL_AN6  = 0x6U,
    ADC_CHANNEL_AN7  = 0x7U,
    ADC_CHANNEL_AN8  = 0x8U,
    ADC_CHANNEL_AN9  = 0x9U,
    ADC_CHANNEL_AN10 = 0xAU,
    ADC_CHANNEL_AN11 = 0xBU,
} ADC_ChannelTypeDef;

/**
 * @brief A/D conversion clock (ADCON2<ADCS2:ADCS0>, Register 19-3).
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
 * @brief A/D acquisition time. This part's ADCON2 has NO ACQT field
 *        (bits 3:6 unimplemented, confirmed in the DFP/EDC: only ADCS
 *        bits 2:0 and ADFM bit 7 exist, unlike the 4550/2520/1320) so
 *        the sample-and-hold acquisition is internally timed. The field
 *        is kept in the handle for cross-family contract compatibility
 *        and ignored by the driver.
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
 * @brief Result-format select (ADCON2<ADFM>, Register 19-3).
 */
typedef enum {
    ADC_FORMAT_LEFT  = 0x0U,   /**< ADFM=0, left justified. */
    ADC_FORMAT_RIGHT = 0x1U,   /**< ADFM=1, right justified. */
} ADC_ResultFormatTypeDef;

/**
 * @brief Voltage reference (ADCON1<VCFG1:VCFG0>, Register 19-2).
 *        Enum values are the ADCON1 bit patterns: VCFG0 (bit4) selects
 *        Vref+ (AN3 vs VDD), VCFG1 (bit5) selects Vref- (AN2 vs VSS).
 */
typedef enum {
    ADC_VREF_VDD_VSS  = 0x00U,                       /**< Vref+=VDD, Vref-=VSS.  */
    ADC_VREF_AN3_VSS  = PIC_ADCON1_VCFG0,            /**< Vref+=AN3, Vref-=VSS.  */
    ADC_VREF_VDD_AN2  = PIC_ADCON1_VCFG1,            /**< Vref+=VDD, Vref-=AN2.  */
    ADC_VREF_AN3_AN2  = (PIC_ADCON1_VCFG0 | PIC_ADCON1_VCFG1), /**< Vref+=AN3, Vref-=AN2. */
} ADC_VReferenceTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    ADC_ChannelTypeDef         Channel;
    ADC_ClockSourceTypeDef      ClockSource;
    /** @brief Acquisition time (ignored: no ACQT field on this part). */
    ADC_AcquisitionTypeDef      Acquisition;
    ADC_ResultFormatTypeDef     ResultFormat;
    ADC_VReferenceTypeDef       VReference;
    uint8_t                     PinConfig;      /**< PCFG3:PCFG0 (Table 19-4), 0..15. */
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
 * @brief  Configure the ADC: channel select, clock, acquisition time,
 *         result format and voltage reference from `h`, then enable the
 *         module (ADCON0<ADON>). Does not start a conversion.
 * @param h the ADC handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);

/**
 * @brief  Disable the ADC module (ADCON0<ADON> = 0) and clear ADIF.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);

/**
 * @brief  Start a conversion: sets ADCON0<GO/DONE>. The caller is expected
 *         to: select the channel, wait the acquisition time, call Start,
 *         then poll IsConversionDone() or wait for the IRQ.
 * @return 0 on success, 0xFFFF if a conversion was already in progress.
 */
uint16_t EPIC_ADC_Start(void);

/**
 * @brief Select the channel without starting conversion.
 * @param ch the channel to select (a @ref ADC_ChannelTypeDef value).
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);

/**
 * @brief Returns 1 if a conversion is in progress (GO/DONE = 1).
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

#endif /* PIC18F6520_ADC_H */
