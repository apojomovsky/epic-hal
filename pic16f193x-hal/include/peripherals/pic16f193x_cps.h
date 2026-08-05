#ifndef PIC16F193X_CPS_H
#define PIC16F193X_CPS_H
#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
typedef struct {
    uint8_t Channel;
    uint8_t Range;
} CPS_HandleTypeDef;
#define CPS_HANDLE_DEFAULT { .Channel = 0U, .Range = 0x00U }
HAL_StatusTypeDef HAL_CPS_Init(const CPS_HandleTypeDef *h);
HAL_StatusTypeDef HAL_CPS_DeInit(void);
#endif
