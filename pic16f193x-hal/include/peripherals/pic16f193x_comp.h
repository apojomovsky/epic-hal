/** PIC16F193X dual comparator driver (DS41364B §9.0). */
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

/**
 * @brief Configure a comparator from the handle and enable it: programs
 *        CMxCON0 (CxON, hysteresis, output inversion, output to pin) and
 *        CMxCON1 (input channels, interrupt edge).
 * @param h handle with instance, channel and edge selection
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle, an invalid
 *         instance, or a channel above 3
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h);
/**
 * @brief Disable a comparator: clears CMxCON0 and CMxCON1 for the
 *        instance.
 * @param inst which comparator instance (1 or 2)
 * @return EPIC_OK on success, EPIC_INVALID for an invalid instance
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst);
/**
 * @brief Read the comparator's digital output level from CMOUT.
 * @param inst which comparator instance (1 or 2)
 * @return 1 if the output is high, 0 if low (or for an invalid instance)
 */
uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst);

/**
 * @brief Weak CMP1 ISR, override in user code.
 */
void CMP1_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak CMP2 ISR, override in user code.
 */
void CMP2_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F193X_COMP_H */
