/* SR latch driver (DS40001291H §8.9, Register 8-4). The SR latch can
 * be set by C1OUT or a software pulse, reset by C2OUT or a software
 * pulse (reset-dominant), and its Q/Q outputs can be routed to the
 * C1OUT/C2OUT pins via SR1:SR0. SRCON lives in Bank 3. */

#ifndef PIC16F88X_SRLATCH_H
#define PIC16F88X_SRLATCH_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief Latch output configuration (SRCON<SR1:SR0>, DS40001291H §8.9.2).
 */
typedef enum {
    SRLATCH_OUT_C1C2      = 0x0U,   /**< 00: C1OUT and C2OUT pins (unlatched). */
    SRLATCH_OUT_C1_Q      = 0x1U,   /**< 01: C1OUT pin + latch Q. */
    SRLATCH_OUT_C2_Q      = 0x2U,   /**< 10: C2OUT pin + latch Q. */
    SRLATCH_OUT_Q_QBAR    = 0x3U,   /**< 11: latch Q and Q-bar. */
} SRLATCH_OutputTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    SRLATCH_OutputTypeDef  Output;    /**< SR1:SR0. */
    bool                   C1SetEnable;   /**< C1SEN: C1OUT sets the latch. */
    bool                   C2ResetEnable; /**< C2REN: C2OUT resets the latch. */
    bool                   FVREN;         /**< Enable the 0.6 V fixed reference. */
} SRLATCH_HandleTypeDef;

#define SRLATCH_HANDLE_DEFAULT {                                         \
    .Output         = SRLATCH_OUT_C1C2,                                   \
    .C1SetEnable    = false,                                              \
    .C2ResetEnable  = false,                                              \
    .FVREN          = false,                                              \
}

/**
 * @brief  Initialize the SR latch with the given handle. Programs SRCON
 *         (output config, set/reset enables, FVR enable).
 * @param h handle with Output, C1SetEnable, C2ResetEnable, FVREN.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_SRLATCH_Init(const SRLATCH_HandleTypeDef *h);

/**
 * @brief  De-initialize the SR latch. Returns SRCON to reset (the
 *         unlatched C1OUT/C2OUT default).
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_SRLATCH_DeInit(void);

/**
 * @brief  Set the SR latch with a software pulse (PULSS, self-clearing).
 */
void EPIC_SRLATCH_Set(void);

/**
 * @brief  Reset the SR latch with a software pulse (PULSR, self-clearing).
 *         The latch is reset-dominant: if both Set and Reset are high,
 *         the latch goes to the Reset state.
 */
void EPIC_SRLATCH_Reset(void);

#endif /* PIC16F88X_SRLATCH_H */
