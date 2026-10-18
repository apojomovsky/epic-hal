/**
 * PIC16F188xx comparator pair C1/C2 (DS40001802D §18). Same EPIC_COMP_*
 * shape as the PIC16F193X port, with NCH/PCH channel selects. Pin
 * output needs EPIC_PPS_MapOutput (PPS_OUTPUT_C1OUT/C2OUT): there is
 * no output-to-pin bit, so OutputToPin is gone.
 */
#ifndef PIC16F188XX_COMP_H
#define PIC16F188XX_COMP_H

#include "pic16f188xx.h"
#include "pic16f188xx_sfr.h"

typedef enum {
    COMP_INSTANCE_1 = 1,
    COMP_INSTANCE_2 = 2,
} COMP_InstanceTypeDef;

typedef enum {
    COMP_INT_EDGE_NONE    = 0x0U,
    COMP_INT_EDGE_RISING  = 0x1U,  /**< CMxCON1<INTP>. */
    COMP_INT_EDGE_FALLING = 0x2U,  /**< CMxCON1<INTN>. */
    COMP_INT_EDGE_BOTH    = 0x3U,
} COMP_InterruptEdgeTypeDef;

/** Negative input (CMxNSEL<NCH>, Register 18-3). */
typedef enum {
    COMP_NEG_PIN0 = 0x0U,  /**< CxIN0- (RA0). */
    COMP_NEG_PIN1 = 0x1U,  /**< CxIN1- (RA1). */
    COMP_NEG_PIN2 = 0x2U,  /**< CxIN2- (RB3). */
    COMP_NEG_PIN3 = 0x3U,  /**< CxIN3- (RB1). */
    COMP_NEG_FVR  = 0x6U,  /**< FVR buffer 2. */
    COMP_NEG_AVSS = 0x7U,  /**< Analog ground. */
} COMP_NegChannelTypeDef;

/** Positive input (CMxPSEL<PCH>, Register 18-4). */
typedef enum {
    COMP_POS_PIN0 = 0x0U,  /**< CxIN0+ (RA2). */
    COMP_POS_PIN1 = 0x1U,  /**< C1IN1+ (RA3), C2IN1+ (RB0). */
    COMP_POS_DAC  = 0x5U,  /**< DAC1 output. */
    COMP_POS_FVR  = 0x6U,  /**< FVR buffer 2. */
    COMP_POS_AVSS = 0x7U,  /**< Analog ground. */
} COMP_PosChannelTypeDef;

typedef struct {
    COMP_InstanceTypeDef      Instance;
    COMP_PosChannelTypeDef    PosChannel;
    COMP_NegChannelTypeDef    NegChannel;
    uint8_t                   HysteresisOn;
    uint8_t                   InvertOutput;
    COMP_InterruptEdgeTypeDef InterruptEdge;
    void (*EventCallback)(void);
} COMP_HandleTypeDef;

#define COMP_HANDLE_DEFAULT { \
    .Instance = COMP_INSTANCE_1, .PosChannel = COMP_POS_PIN0, \
    .NegChannel = COMP_NEG_PIN0, .HysteresisOn = 0U, .InvertOutput = 0U, \
    .InterruptEdge = COMP_INT_EDGE_NONE, .EventCallback = 0, \
}

/**
 * @brief Configure a comparator from the handle and enable it: programs
 *        CMxNSEL/CMxPSEL (input channels), CMxCON1 (interrupt edge) and
 *        CMxCON0 (CxON, hysteresis, output inversion).
 * @param h handle with instance, channel and edge selection
 * @return EPIC_OK on success, EPIC_INVALID for a NULL handle, an invalid
 *         instance or a channel outside the enums above
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h);
/**
 * @brief Disable a comparator: clears CMxCON0, CMxCON1, CMxNSEL and
 *        CMxPSEL for the instance.
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
 * @brief Weak C1 ISR, override in user code.
 */
void CMP1_IRQHandler(void) EPIC_WEAK;
/**
 * @brief Weak C2 ISR, override in user code.
 */
void CMP2_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F188XX_COMP_H */
