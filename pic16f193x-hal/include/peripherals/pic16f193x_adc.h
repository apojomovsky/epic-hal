/**
 * @file    peripherals/pic16f193x_adc.h
 * @brief   PIC16F193X 10-bit ADC driver.
 * @details Source: DS41364B ADC chapter. DS80000479 errata: ADC may
 *          not complete at FOSC > 8 MHz; ADC_HANDLE_DEFAULT uses
 *          ADC_CLOCK_FRC (Fosc-independent) to sidestep it by default.
 *          Full reference: MANUAL.md.
 */
#ifndef PIC16F193X_ADC_H
#define PIC16F193X_ADC_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

typedef enum {
    ADC_CLOCK_FOSC_2  = 0x0U,
    ADC_CLOCK_FOSC_8  = 0x1U,
    ADC_CLOCK_FOSC_32 = 0x2U,
    ADC_CLOCK_FRC     = 0x3U,  /**< Fosc-independent, errata-safe. */
    ADC_CLOCK_FOSC_4  = 0x4U,
    ADC_CLOCK_FOSC_16 = 0x5U,
    ADC_CLOCK_FOSC_64 = 0x6U,
} ADC_ClockSourceTypeDef;

typedef enum {
    ADC_POSREF_VDD = 0x0U,
    ADC_POSREF_FVR = 0x3U,
} ADC_ReferencePosTypeDef;

typedef enum {
    ADC_NEGREF_VSS = 0x0U,
} ADC_ReferenceNegTypeDef;

typedef enum {
    ADC_FORMAT_LEFT_JUSTIFIED  = 0x0U,
    ADC_FORMAT_RIGHT_JUSTIFIED = 0x1U,
} ADC_ResultFormatTypeDef;

typedef struct {
    ADC_ClockSourceTypeDef  ClockSource;
    ADC_ReferencePosTypeDef PosRef;
    ADC_ReferenceNegTypeDef NegRef;
    ADC_ResultFormatTypeDef Format;
} ADC_HandleTypeDef;

#define ADC_HANDLE_DEFAULT { \
    .ClockSource = ADC_CLOCK_FRC, .PosRef = ADC_POSREF_VDD, \
    .NegRef = ADC_NEGREF_VSS, .Format = ADC_FORMAT_RIGHT_JUSTIFIED, \
}

EPIC_StatusTypeDef EPIC_ADC_Init(const ADC_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_ADC_DeInit(void);
EPIC_StatusTypeDef EPIC_ADC_SelectChannel(uint8_t channel);
void EPIC_ADC_Start(void);
uint8_t EPIC_ADC_IsConversionDone(void);
uint16_t EPIC_ADC_Read(void);

void ADC_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F193X_ADC_H */
