/* Comparator driver, two independent comparators C1 and C2. Source:
 * DS40001291H §8.0, Registers 8-1..8-3, Figures 8-1..8-3; full
 * reference: MANUAL.md §Comparators. Unlike the 87XA's single CMCON
 * mode word, each comparator is configured independently through
 * CM1CON0/CM2CON0, has its own interrupt flag (C1IF/C2IF in PIR2), and
 * the shared CM2CON1 carries the reference selects, the Timer1 gate
 * source select and C2's Timer1 synchronization. The SR latch lives in
 * SRCON (separate driver, see pic16f88x_srlatch.h). */

#ifndef PIC16F88X_COMP_H
#define PIC16F88X_COMP_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief Comparator inverting-input channel (CMxCON0<CxCH1:CxCH0>,
 *        DS40001291H Register 8-1/8-2).
 */
typedef enum {
    COMP_CHANNEL_IN0 = 0x0U,   /**< C12IN0- pin. */
    COMP_CHANNEL_IN1 = 0x1U,   /**< C12IN1- pin. */
    COMP_CHANNEL_IN2 = 0x2U,   /**< C12IN2- pin (C2 only). */
    COMP_CHANNEL_IN3 = 0x3U,   /**< C12IN3- pin (C2 only). */
} COMP_ChannelTypeDef;

/**
 * @brief Comparator non-inverting-input source (CMxCON0<CxR>).
 */
typedef enum {
    COMP_INPUT_PIN     = 0x0U,   /**< CxIN+ pin. */
    COMP_INPUT_REF     = 0x1U,   /**< CxVREF (CVREF or 0.6 V, per CM2CON1<CxRSEL>). */
} COMP_InputSourceTypeDef;

/**
 * @brief Comparator reference source (CM2CON1<CxRSEL>, Register 8-3).
 */
typedef enum {
    COMP_REF_ABSOLUTE   = 0x0U,   /**< 0.6 V fixed reference. */
    COMP_REF_CVREF      = 0x1U,   /**< CVREF (VRCON, configurable). */
} COMP_RefSourceTypeDef;

/** Driver handle, one per comparator (Cube-style). */
typedef struct {
    COMP_ChannelTypeDef      Channel;       /**< CxCH<1:0>, inverting input. */
    COMP_InputSourceTypeDef  InputSource;   /**< CxR, non-inverting input. */
    COMP_RefSourceTypeDef    RefSource;     /**< CM2CON1<CxRSEL>. */
    bool                     Inverted;      /**< CxPOL. */
    bool                     OutputEnable;  /**< CxOE (output to pin). */
    /** @brief Optional change callback (fires on CxIF). */
    void (*ChangeCallback)(void);
} COMP_HandleTypeDef;

#define COMP_HANDLE_DEFAULT {                                              \
    .Channel         = COMP_CHANNEL_IN0,                                   \
    .InputSource     = COMP_INPUT_PIN,                                     \
    .RefSource       = COMP_REF_ABSOLUTE,                                  \
    .Inverted        = false,                                              \
    .OutputEnable    = false,                                              \
    .ChangeCallback  = NULL,                                               \
}

/**
 * @brief  Initialize comparator C1 with the given handle. Programs
 *         CM1CON0, the CM2CON1<C1RSEL> bit, and installs the change
 *         callback (PIE2<C1IE>).
 * @param h handle with Channel, InputSource, RefSource, Inverted,
 *        OutputEnable, ChangeCallback.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP1_Init(const COMP_HandleTypeDef *h);

/**
 * @brief  Initialize comparator C2 with the given handle. Programs
 *         CM2CON0, the CM2CON1<C2RSEL> bit, and installs the change
 *         callback (PIE2<C2IE>).
 * @param h handle with Channel, InputSource, RefSource, Inverted,
 *        OutputEnable, ChangeCallback.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP2_Init(const COMP_HandleTypeDef *h);

/**
 * @brief  De-initialize comparator C1. Returns CM1CON0 to reset and
 *         clears the change callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP1_DeInit(void);

/**
 * @brief  De-initialize comparator C2. Returns CM2CON0 to reset and
 *         clears the change callback.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP2_DeInit(void);

/**
 * @brief Returns 1 if C1 output is high (CM1CON0<C1OUT>).
 * @return 1 if C1 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C1Out(void);

/**
 * @brief Returns 1 if C2 output is high (CM2CON0<C2OUT>).
 * @return 1 if C2 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C2Out(void);

/**
 * @brief Returns 1 if C1IF is set.
 * @return 1 if the C1 change flag is set, 0 otherwise.
 */
uint8_t EPIC_COMP_C1ChangeFlag(void);

/**
 * @brief Returns 1 if C2IF is set.
 * @return 1 if the C2 change flag is set, 0 otherwise.
 */
uint8_t EPIC_COMP_C2ChangeFlag(void);

/**
 * @brief Clear the C1IF flag (must be done in the C1 change IRQ).
 */
void EPIC_COMP_ClearC1Flag(void);

/**
 * @brief Clear the C2IF flag (must be done in the C2 change IRQ).
 */
void EPIC_COMP_ClearC2Flag(void);

/**
 * @brief Timer1 gate source constants for @ref EPIC_COMP_SetT1GateSource
 *        (CM2CON1<T1GSS>, DS40001291H Register 8-3).
 */
#define COMP_GATE_SRC_C2OUT  0U   /**< Timer1 gate = synchronized C2OUT. */
#define COMP_GATE_SRC_T1G    1U   /**< Timer1 gate = T1G pin. */

/**
 * @brief Select the Timer1 gate source (CM2CON1<T1GSS>): T1G pin or the
 *        synchronized C2 output. Used by the Timer1 driver's gate mode;
 *        exposed here so application code can switch the source without
 *        re-initializing Timer1.
 * @param src COMP_GATE_SRC_T1G or COMP_GATE_SRC_C2OUT.
 */
void EPIC_COMP_SetT1GateSource(uint8_t src);

/**
 * @brief Enable or disable C2's output synchronization to Timer1
 *        (CM2CON1<C2SYNC>, DS40001291H §8.8.2). Recommended when the
 *        comparator gates Timer1, to avoid missed increments.
 * @param enable 1 to synchronize, 0 for asynchronous output.
 */
void EPIC_COMP_SetC2Sync(uint8_t enable);

/**
 * @brief Weak C1 change ISR, override in user code.
 */
void COMP1_IRQHandler(void) EPIC_WEAK;

/**
 * @brief Weak C2 change ISR, override in user code.
 */
void COMP2_IRQHandler(void) EPIC_WEAK;

#endif /* PIC16F88X_COMP_H */
