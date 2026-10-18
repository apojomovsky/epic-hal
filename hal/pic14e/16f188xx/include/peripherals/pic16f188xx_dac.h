/**
 * PIC16F188xx 5-bit DAC1 (DS40001802D §25, Registers 25-1/25-2). Mirrors
 * the PIC16F193X EPIC_DAC_* shape with the silicon's wider handle: the
 * 5-bit code, the source pair and the two pin output enables. With both
 * enables clear the DAC still feeds the ADC (channel ADC_CHANNEL_DAC)
 * and the comparators (PCH 5) internally.
 */
#ifndef PIC16F188XX_DAC_H
#define PIC16F188XX_DAC_H

#include "pic16f188xx.h"
#include "pic16f188xx_sfr.h"

/** Positive source (DAC1CON0<DAC1PSS>, Register 25-1). */
typedef enum {
    DAC_POSSRC_VDD     = 0x0U,
    DAC_POSSRC_VREFPIN = 0x1U,
    DAC_POSSRC_FVR     = 0x2U,  /**< FVR buffer 2, needs EPIC_FVR_Init first. */
} DAC_PosSourceTypeDef;

/** Negative source (DAC1CON0<DAC1NSS>, Register 25-1). */
typedef enum {
    DAC_NEGSRC_VSS     = 0x0U,
    DAC_NEGSRC_VREFPIN = 0x1U,
} DAC_NegSourceTypeDef;

/** Pin output enables (DAC1CON0<DAC1OE1:DAC1OE2>, Register 25-1). */
#define DAC_OUT1_ENABLE 0x01U  /**< DAC1OUT1 pin (RA2). */
#define DAC_OUT2_ENABLE 0x02U  /**< DAC1OUT2 pin (RB7). */

typedef struct {
    uint8_t           OutputValue;  /**< 5-bit code, DAC1R<4:0>. */
    DAC_PosSourceTypeDef PosSource;
    DAC_NegSourceTypeDef NegSource;
    uint8_t           OutputEnable; /**< DAC_OUT1_ENABLE and DAC_OUT2_ENABLE. */
} DAC_HandleTypeDef;

#define DAC_HANDLE_DEFAULT { \
    .OutputValue = 0x0FU, .PosSource = DAC_POSSRC_VDD, \
    .NegSource = DAC_NEGSRC_VSS, .OutputEnable = 0U, \
}

/**
 * @brief Enable DAC1 and program the code, sources and pin enables.
 * @param h handle with the 5-bit code, sources and output enables
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle or a code
 *         above 31
 */
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h);
/**
 * @brief Disable DAC1 and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_DAC_DeInit(void);
/**
 * @brief Change the output code while the DAC stays enabled.
 * @param value 5-bit code, DAC1R<4:0>
 * @return EPIC_OK on success, EPIC_INVALID for a code above 31
 */
EPIC_StatusTypeDef EPIC_DAC_SetValue(uint8_t value);

#endif /* PIC16F188XX_DAC_H */
