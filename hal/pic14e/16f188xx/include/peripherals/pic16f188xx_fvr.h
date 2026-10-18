/**
 * PIC16F188xx Fixed Voltage Reference (DS40001802D §16, Register 16-1).
 * Same EPIC_FVR_* shape as the PIC16F193X port: the two independent
 * gain buffers (ADC and comparator/DAC). TSEN/TSRNG stay off; the ADC
 * temperature channel needs them and is documented in MANUAL.md.
 */
#ifndef PIC16F188XX_FVR_H
#define PIC16F188XX_FVR_H

#include "pic16f188xx.h"
#include "pic16f188xx_sfr.h"

/** Buffer gain (ADFVR/CDAFVR, Register 16-1). */
typedef enum {
    FVR_GAIN_OFF = 0x0U,
    FVR_GAIN_1X  = 0x1U,  /**< 1.024V. */
    FVR_GAIN_2X  = 0x2U,  /**< 2.048V. */
    FVR_GAIN_4X  = 0x3U,  /**< 4.096V, needs VDD above it. */
} FVR_GainTypeDef;

typedef struct {
    FVR_GainTypeDef AdcGain;      /**< FVR buffer 1, to the ADC. */
    FVR_GainTypeDef CompDacGain;  /**< FVR buffer 2, to DAC and comparators. */
} FVR_HandleTypeDef;

#define FVR_HANDLE_DEFAULT { .AdcGain = FVR_GAIN_2X, .CompDacGain = FVR_GAIN_2X }

/**
 * @brief Enable the Fixed Voltage Reference and program the ADC and
 *        comparator/DAC gain selections from the handle.
 * @param h handle with ADC and comparator/DAC gain selections
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_FVR_Init(const FVR_HandleTypeDef *h);
/**
 * @brief Disable the Fixed Voltage Reference (clears FVRCON).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_FVR_DeInit(void);
/**
 * @brief Poll whether the Fixed Voltage Reference output is stable.
 * @return 1 when FVRRDY is set, 0 while the reference is stabilizing
 */
uint8_t EPIC_FVR_IsReady(void);
#endif /* PIC16F188XX_FVR_H */
