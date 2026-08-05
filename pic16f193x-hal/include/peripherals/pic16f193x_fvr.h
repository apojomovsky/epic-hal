#ifndef PIC16F193X_FVR_H
#define PIC16F193X_FVR_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t AdcGain;
    uint8_t CompDacGain;
} FVR_HandleTypeDef;
#define FVR_HANDLE_DEFAULT { .AdcGain = 0x02U, .CompDacGain = 0x02U }
EPIC_StatusTypeDef EPIC_FVR_Init(const FVR_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_FVR_DeInit(void);
uint8_t EPIC_FVR_IsReady(void);
#endif
