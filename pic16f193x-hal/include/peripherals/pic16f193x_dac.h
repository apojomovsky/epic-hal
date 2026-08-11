#ifndef PIC16F193X_DAC_H
#define PIC16F193X_DAC_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t OutputValue;
} DAC_HandleTypeDef;
#define DAC_HANDLE_DEFAULT { .OutputValue = 0x0FU }
/**
 * @brief Enable the DAC and program the output value from the handle.
 * @param h handle with the 4-bit output value
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h);
/**
 * @brief Disable the DAC and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_DAC_DeInit(void);
#endif
