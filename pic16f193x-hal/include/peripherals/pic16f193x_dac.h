#ifndef PIC16F193X_DAC_H
#define PIC16F193X_DAC_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t OutputValue;
} DAC_HandleTypeDef;
#define DAC_HANDLE_DEFAULT { .OutputValue = 0x0FU }
EPIC_StatusTypeDef EPIC_DAC_Init(const DAC_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_DAC_DeInit(void);
#endif
