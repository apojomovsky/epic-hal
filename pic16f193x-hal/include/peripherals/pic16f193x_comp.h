/**
 * @file    peripherals/pic16f193x_comp.h
 * @brief   PIC16F193X dual comparator driver (DS41364B §9.0).
 */
#ifndef PIC16F193X_COMP_H
#define PIC16F193X_COMP_H

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"

typedef enum {
    COMP_INSTANCE_1 = 1,
    COMP_INSTANCE_2 = 2,
} COMP_InstanceTypeDef;

typedef enum {
    COMP_INT_EDGE_NONE    = 0x0U,
    COMP_INT_EDGE_RISING   = 0x1U,
    COMP_INT_EDGE_FALLING  = 0x2U,
    COMP_INT_EDGE_BOTH     = 0x3U,
} COMP_InterruptEdgeTypeDef;

typedef struct {
    COMP_InstanceTypeDef      Instance;
    uint8_t                   PosChannel;
    uint8_t                   NegChannel;
    uint8_t                   HysteresisOn;
    uint8_t                   InvertOutput;
    uint8_t                   OutputToPin;
    COMP_InterruptEdgeTypeDef InterruptEdge;
    void (*EventCallback)(void);
} COMP_HandleTypeDef;

#define COMP_HANDLE_DEFAULT { \
    .Instance = COMP_INSTANCE_1, .PosChannel = 0U, .NegChannel = 0U, \
    .HysteresisOn = 0U, .InvertOutput = 0U, .OutputToPin = 0U, \
    .InterruptEdge = COMP_INT_EDGE_NONE, .EventCallback = 0, \
}

HAL_StatusTypeDef HAL_COMP_Init(const COMP_HandleTypeDef *h);
HAL_StatusTypeDef HAL_COMP_DeInit(COMP_InstanceTypeDef inst);
uint8_t HAL_COMP_ReadOutput(COMP_InstanceTypeDef inst);

void CMP1_IRQHandler(void) PIC8_WEAK;
void CMP2_IRQHandler(void) PIC8_WEAK;

#endif /* PIC16F193X_COMP_H */
