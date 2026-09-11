/* Shared A/D converter driver (SPI of the analog world: 10-bit SAR with
 * a mux), classic mid-range: 87XA (DS39582B section 11.0) and 88X
 * (DS40001291H section 15.0). The two families wire the ADC differently
 * and the divergences are gated, not hidden:
 *   - PIC14MIDRANGE_HAS_ADC_PCFG (87XA): 16 PCFG reference/voltage-pin
 *     configurations in ADCON1, 3-bit channel mux, 7 clock rates via
 *     ADCS2+ADCS1:0. Without it (88X): 4 VCFG reference combos, 4-bit
 *     channel mux (AN0..13 + CVREF/VP6), 4 clock rates.
 *   - PIC14MIDRANGE_HAS_ANSEL (88X): per-pin analog gating through
 *     ANSEL/ANSELH and the ConfigChannel API.
 * Each family's shim header (pic16f87xa_adc.h / pic16f88x_adc.h) sets
 * its umbrella + SFR map, then includes this body; do not include it
 * directly. */

#ifndef PIC14_ADC_BODY_H
#define PIC14_ADC_BODY_H

#include "pic14_midrange.h"
#include "pic14_midrange_sfr.h"

/**
 * @brief Analog channel select (ADCON0<CHS>).
 *        The 88X muxes 14 pins plus two internal sources; the 87XA
 *        muxes 8 pins. The tail is gated accordingly.
 */
typedef enum {
    ADC_CHANNEL_AN0   = 0x0U,
    ADC_CHANNEL_AN1   = 0x1U,
    ADC_CHANNEL_AN2   = 0x2U,
    ADC_CHANNEL_AN3   = 0x3U,
    ADC_CHANNEL_AN4   = 0x4U,
    ADC_CHANNEL_AN5   = 0x5U,   /**< 40/44-pin only. */
    ADC_CHANNEL_AN6   = 0x6U,   /**< 40/44-pin only. */
    ADC_CHANNEL_AN7   = 0x7U,
#if PIC14MIDRANGE_HAS_ANSEL
    ADC_CHANNEL_AN8   = 0x8U,
    ADC_CHANNEL_AN9   = 0x9U,
    ADC_CHANNEL_AN10  = 0xAU,
    ADC_CHANNEL_AN11  = 0xBU,
    ADC_CHANNEL_AN12  = 0xCU,
    ADC_CHANNEL_AN13  = 0xDU,
    ADC_CHANNEL_CVREF = 0xEU,   /**< Internal CVREF (comparator reference). */
    ADC_CHANNEL_VP6   = 0xFU,   /**< Fixed 0.6 V reference (FVR). */
#endif
} ADC_ChannelTypeDef;

/**
 * @brief ADC clock select (ADCS2:ADCS0 on the 87XA, ADCS1:ADCS0 on the
 *        88X). The 87XA reaches the four high rates through ADCS2 in
 *        ADCON1; the 88X has no ADCS2, so its enum stops at RC.
 */
typedef enum {
    ADC_CLOCK_FOSC_2     = 0x0U,   /**< 000/00, Fosc/2.   */
    ADC_CLOCK_FOSC_8     = 0x1U,   /**< 001/01, Fosc/8.   */
    ADC_CLOCK_FOSC_32    = 0x2U,   /**< 010/10, Fosc/32.  */
    ADC_CLOCK_RC         = 0x3U,   /**< 011, internal A/D RC. */
#if PIC14MIDRANGE_HAS_ADC_PCFG
    ADC_CLOCK_FOSC_4     = 0x4U,   /**< 100, Fosc/4.   */
    ADC_CLOCK_FOSC_16    = 0x5U,   /**< 101, Fosc/16.  */
    ADC_CLOCK_FOSC_64    = 0x6U,   /**< 110, Fosc/64.  */
#endif
} ADC_ClockSourceTypeDef;

#if PIC14MIDRANGE_HAS_ADC_PCFG
/**
 * @brief Reference/voltage-pin configuration (ADCON1<PCFG3:0>,
 *        DS39582B Register 11-2). Each value fixes which pins are
 *        analog and where Vref+ / Vref- come from.
 */
typedef enum {
    ADC_REFERENCE_VDD_VSS_8CH    = 0x0U,   /* AN0..AN7 analog, Vref+=Vdd, Vref-=Vss. */
    ADC_REFERENCE_VDD_VSS_7CH    = 0x1U,   /* AN0..AN6 + Vref-=AN3. */
    ADC_REFERENCE_VDD_VSS_5CH    = 0x2U,   /* AN0..AN4, Vref+ on AN3. */
    ADC_REFERENCE_VDD_VSS_4CH    = 0x3U,   /* AN0..AN3, Vref+/Vref- on AN3/AN2. */
    ADC_REFERENCE_VDD_VSS_3CH    = 0x4U,   /* AN0..AN2. */
    ADC_REFERENCE_VREF_2CH       = 0x5U,   /* AN0..AN1 + Vref+/Vref- on AN3/AN2. */
    ADC_REFERENCE_VDD_VSS_6CH    = 0x6U,   /* AN0..AN5. */
    ADC_REFERENCE_VDD_VSS_5CH_B  = 0x7U,
    ADC_REFERENCE_VREF_6CH       = 0x8U,   /* AN0..AN5, Vref+/Vref- on AN3/AN2. */
    ADC_REFERENCE_VDD_VSS_6CH_C  = 0x9U,
    ADC_REFERENCE_VDD_VSS_5CH_D  = 0xAU,
    ADC_REFERENCE_VREF_4CH       = 0xBU,
    ADC_REFERENCE_VREF_3CH       = 0xCU,
    ADC_REFERENCE_VREF_2CH_B     = 0xDU,
    ADC_REFERENCE_VDD_VSS_2CH    = 0xEU,
    ADC_REFERENCE_VREF_2CH_C     = 0xFU,
} ADC_ReferenceTypeDef;
#else
/**
 * @brief Reference voltage select (ADCON1<VCFG1:VCFG0>).
 */
typedef enum {
    ADC_REFERENCE_VDD_VSS      = 0x0U,   /**< Vref+ = Vdd, Vref- = Vss. */
    ADC_REFERENCE_VDD_VREFN    = 0x1U,   /**< Vref+ = Vdd, Vref- = VREF- pin. */
    ADC_REFERENCE_VREFP_VSS    = 0x2U,   /**< Vref+ = VREF+ pin, Vref- = Vss. */
    ADC_REFERENCE_VREFP_VREFN  = 0x3U,   /**< Vref+ = VREF+ pin, Vref- = VREF- pin. */
} ADC_ReferenceTypeDef;
#endif

/**
 * @brief Result justification (ADCON1<ADFM>). Read always returns a
 *        right-justified 0..1023 value regardless of this setting.
 */
typedef enum {
    ADC_FORMAT_LEFT  = 0x0U,   /**< ADFM=0, left justified. */
    ADC_FORMAT_RIGHT = 0x1U,   /**< ADFM=1, right justified. */
} ADC_ResultFormatTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    ADC_ChannelTypeDef        Channel;
    ADC_ClockSourceTypeDef    ClockSource;
    ADC_ReferenceTypeDef      Reference;
    ADC_ResultFormatTypeDef   ResultFormat;
    /** @brief Optional conversion-complete callback (fires on ADIF). */
    void (*ConvCpltCallback)(uint16_t result);
} ADC_HandleTypeDef;

/* Reference default 0 = Vdd/Vss in both flavors (87XA PCFG 0). */
#define ADC_HANDLE_DEFAULT {                                              \
    .Channel         = ADC_CHANNEL_AN0,                                     \
    .ClockSource     = ADC_CLOCK_FOSC_2,                                    \
    .Reference       = 0,                                                   \
    .ResultFormat    = ADC_FORMAT_RIGHT,                                    \
    .ConvCpltCallback = NULL,                                               \
}

/* init / deinit. */

/**
 * @brief  Initialize the A/D converter: program ADCON0/ADCON1 from the
 *         handle and arm the conversion-complete interrupt if a callback
 *         is set.
 * @param h handle with Channel, ClockSource, Reference, ResultFormat,
 *        ConvCpltCallback.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);

/**
 * @brief  De-initialize the A/D converter: disable its interrupt, clear
 *         the pending flag, and reset ADCON0/ADCON1.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);

/* channel select. */

/**
 * @brief Select the analog channel without starting a conversion.
 * @param ch the analog channel to select.
 */
void EPIC_ADC_SelectChannel(ADC_ChannelTypeDef ch);

#if PIC14MIDRANGE_HAS_ANSEL
/**
 * @brief Gate one pin as analog (set its ANSEL/ANSELH bit). The 88X
 *        gates every pin through ANSEL/ANSELH independently of the
 *        reference configuration; a conversion reads the pin as
 *        digital (and wrong) while its gate is clear.
 * @param ch the analog channel to gate.
 */
void EPIC_ADC_ConfigChannel(ADC_ChannelTypeDef ch);
#endif /* PIC14MIDRANGE_HAS_ANSEL */

/* conversion control. */

/**
 * @brief Start a conversion by setting GO/DONE.
 * @return 0 on success, 0xFFFF if a conversion was already running.
 */
uint16_t EPIC_ADC_Start(void);

/**
 * @brief Report whether a conversion is in progress.
 * @return 1 if GO/DONE is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionInProgress(void);

/**
 * @brief Report whether the latest conversion completed.
 * @return 1 if ADIF (PIR1<6>) is set, 0 otherwise.
 */
uint8_t EPIC_ADC_IsConversionDone(void);

/**
 * @brief Clear the ADIF flag via the IRQ table.
 */
void EPIC_ADC_ClearITFlag(void);

/**
 * @brief Read the latest 10-bit result (0..1023), right-justified.
 * @return the conversion result.
 */
uint16_t EPIC_ADC_Read(void);

/* interrupts. */

/**
 * @brief Weak ADC conversion-complete ISR; clears ADIF and invokes the
 *        registered callback.
 */
void ADC_IRQHandler(void) EPIC_WEAK;

#endif /* PIC14_ADC_BODY_H */
