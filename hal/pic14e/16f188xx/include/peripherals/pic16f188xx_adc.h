/**
 * PIC16F188xx 10-bit ADC with computation and auto-conversion trigger
 * (DS40001802D §23, Registers 23-1..23-8, Table 23-2). Basic polling
 * mode only: ADCON1/ADCON2 stay at POR (no precharge, no accumulation),
 * ADCONT is clear (one conversion per trigger). Mirrors the PIC16F193X
 * EPIC_ADC_* shape and adds the ADACT trigger source.
 */
#ifndef PIC16F188XX_ADC_H
#define PIC16F188XX_ADC_H

#include "pic16f188xx.h"
#include "pic16f188xx_sfr.h"

/** Conversion clock (ADCON0<ADCS>, Register 23-1). */
typedef enum {
    ADC_CLOCK_FOSC = 0x0U,  /**< FOSC divided by ADCLK<ADCCS> (Register 23-6). */
    ADC_CLOCK_FRC  = 0x1U,  /**< Dedicated FRC oscillator, Fosc-independent. */
} ADC_ClockSourceTypeDef;

/** Positive reference (ADREF<ADPREF>, Register 23-7). */
typedef enum {
    ADC_POSREF_VDD     = 0x0U,
    ADC_POSREF_VREFPIN = 0x2U,
    ADC_POSREF_FVR     = 0x3U,  /**< FVR buffer 1, needs EPIC_FVR_Init first. */
} ADC_ReferencePosTypeDef;

/** Negative reference (ADREF<ADNREF>, Register 23-7). */
typedef enum {
    ADC_NEGREF_VSS     = 0x0U,
    ADC_NEGREF_VREFPIN = 0x1U,
} ADC_ReferenceNegTypeDef;

/** Result format (ADCON0<ADFRM0>, Register 23-1). */
typedef enum {
    ADC_FORMAT_LEFT_JUSTIFIED  = 0x0U,
    ADC_FORMAT_RIGHT_JUSTIFIED = 0x1U,
} ADC_ResultFormatTypeDef;

/** Auto-conversion trigger (ADACT<ADACT>, Table 23-2). */
typedef enum {
    ADC_TRIGGER_DISABLED = 0x00U,  /**< Manual: EPIC_ADC_Start only. */
    ADC_TRIGGER_ADACTPPS = 0x01U,  /**< Pin the ADACTPPS select names. */
    ADC_TRIGGER_TMR0     = 0x02U,  /**< Timer0 overflow. */
    ADC_TRIGGER_TMR1     = 0x03U,  /**< Timer1 overflow. */
    ADC_TRIGGER_TMR2     = 0x04U,  /**< Timer2 postscaled match. */
    ADC_TRIGGER_TMR3     = 0x05U,  /**< Timer3 overflow. */
    ADC_TRIGGER_TMR4     = 0x06U,  /**< Timer4 postscaled match. */
    ADC_TRIGGER_TMR5     = 0x07U,  /**< Timer5 overflow. */
    ADC_TRIGGER_TMR6     = 0x08U,  /**< Timer6 postscaled match. */
    ADC_TRIGGER_SMT1     = 0x09U,  /**< SMT1 period match. */
    ADC_TRIGGER_SMT2     = 0x0AU,  /**< SMT2 period match. */
    ADC_TRIGGER_CCP1     = 0x0BU,  /**< CCP1 output. */
    ADC_TRIGGER_CCP2     = 0x0CU,  /**< CCP2 output. */
    ADC_TRIGGER_CCP3     = 0x0DU,  /**< CCP3 output. */
    ADC_TRIGGER_CCP4     = 0x0EU,  /**< CCP4 output. */
    ADC_TRIGGER_CCP5     = 0x0FU,  /**< CCP5 output. */
    ADC_TRIGGER_PWM6     = 0x10U,  /**< PWM6 output. */
    ADC_TRIGGER_PWM7     = 0x11U,  /**< PWM7 output. */
    ADC_TRIGGER_C1       = 0x12U,  /**< Comparator C1 output. */
    ADC_TRIGGER_C2       = 0x13U,  /**< Comparator C2 output. */
    ADC_TRIGGER_IOC      = 0x14U,  /**< Interrupt-on-change flag. */
    ADC_TRIGGER_CLC1     = 0x15U,  /**< CLC1 output. */
    ADC_TRIGGER_CLC2     = 0x16U,  /**< CLC2 output. */
    ADC_TRIGGER_CLC3     = 0x17U,  /**< CLC3 output. */
    ADC_TRIGGER_CLC4     = 0x18U,  /**< CLC4 output. */
    ADC_TRIGGER_ADERR    = 0x1CU,  /**< Read of ADERR. */
    ADC_TRIGGER_ADRESH   = 0x1DU,  /**< Read of ADRESH. */
    ADC_TRIGGER_ADPCH    = 0x1FU,  /**< Read of ADPCH. */
} ADC_TriggerSourceTypeDef;

/** Internal channels (ADPCH, Register 23-8). */
#define ADC_CHANNEL_AVSS 0x3CU  /**< Analog ground. */
#define ADC_CHANNEL_TEMP 0x3DU  /**< Temperature indicator. */
#define ADC_CHANNEL_DAC  0x3EU  /**< DAC1 output. */
#define ADC_CHANNEL_FVR  0x3FU  /**< Fixed voltage reference. */

typedef struct {
    ADC_ClockSourceTypeDef   ClockSource;
    uint8_t                  ClockDiv;  /**< ADCCS (ADCLK): FOSC/(2*(div+1)). */
    ADC_ReferencePosTypeDef  PosRef;
    ADC_ReferenceNegTypeDef  NegRef;
    ADC_ResultFormatTypeDef  Format;
    ADC_TriggerSourceTypeDef Trigger;
} ADC_HandleTypeDef;

#define ADC_HANDLE_DEFAULT { \
    .ClockSource = ADC_CLOCK_FRC, .ClockDiv = 0U, \
    .PosRef = ADC_POSREF_VDD, .NegRef = ADC_NEGREF_VSS, \
    .Format = ADC_FORMAT_RIGHT_JUSTIFIED, .Trigger = ADC_TRIGGER_DISABLED, \
}

/**
 * @brief Configure the ADC from `h` and enable the module.
 * @param h handle with clock, references, format and trigger
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);
/**
 * @brief Disable the ADC and return its registers to POR.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);
/**
 * @brief Select the input channel for the next conversion.
 * @param channel ADPCH code: pin channels to PIC16F188XX_FAMILY_ADC_MAX_PIN,
 *        or one of ADC_CHANNEL_AVSS/TEMP/DAC/FVR
 * @return EPIC_OK on success, EPIC_INVALID for a channel the part lacks
 */
EPIC_StatusTypeDef EPIC_ADC_SelectChannel(uint8_t channel);
/**
 * @brief Start a conversion on the selected channel (no wait).
 */
void EPIC_ADC_Start(void);
/**
 * @brief Poll whether the current conversion has completed.
 * @return 1 if conversion is done, 0 while it is still running
 */
uint8_t EPIC_ADC_IsConversionDone(void);
/**
 * @brief Read the last conversion result as a 10-bit value in both
 *        justification modes.
 * @return the 10-bit conversion result
 */
uint16_t EPIC_ADC_Read(void);

/**
 * @brief ADC interrupt handler: clears ADIF (weak, override in user code).
 */
void ADC_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F188XX_ADC_H */
