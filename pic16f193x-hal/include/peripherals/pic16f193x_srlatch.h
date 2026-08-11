#ifndef PIC16F193X_SRLATCH_H
#define PIC16F193X_SRLATCH_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
/**
 * @brief  Enable the SR latch module by setting SRCON0<SRLEN>.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Enable(void);

/**
 * @brief  Disable the SR latch module by clearing SRCON0<SRLEN>.
 *
 * @return EPIC_OK
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Disable(void);
#endif
