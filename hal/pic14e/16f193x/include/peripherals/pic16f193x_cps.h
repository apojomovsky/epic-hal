#ifndef PIC16F193X_CPS_H
#define PIC16F193X_CPS_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t Channel;
    uint8_t Range;
} CPS_HandleTypeDef;
#define CPS_HANDLE_DEFAULT { .Channel = 0U, .Range = 0x00U }
/**
 * @brief Enable the capacitive sensing module and program the channel
 *        and range from the handle.
 * @param h handle with channel and range selection
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle
 */
EPIC_StatusTypeDef EPIC_CPS_Init(const CPS_HandleTypeDef *h);
/**
 * @brief Disable the capacitive sensing module and reset its registers.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_CPS_DeInit(void);
#endif
