#ifndef PIC16F193X_FVR_H
#define PIC16F193X_FVR_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t AdcGain;
    uint8_t CompDacGain;
} FVR_HandleTypeDef;
#define FVR_HANDLE_DEFAULT { .AdcGain = 0x02U, .CompDacGain = 0x02U }
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
#endif
