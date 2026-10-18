/*
 * Low-Voltage Detect driver (DS39609B §22.0, Register 22-1). One control
 * register, LVDCON (0xFD2, Access Bank): LVDL<3:0> trip select,
 * LVDEN enable, IRVST (read-only) internal reference stable. The trip
 * event is PIR2<LVDIF>. Present on every part in the family, at the same
 * address (the ECAN quads too).
 *
 * Bring-up order the datasheet requires: enable (Start), poll IsStable,
 * then arm the interrupt (EnableInterrupt). The flag can be set
 * spuriously while the reference settles, which is why arming clears it.
 */

#ifndef PIC18F6520_LVD_H
#define PIC18F6520_LVD_H

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"

/** LVDL<3:0> = 1111: trip point taken from the external LVDIN pin. */
#define LVD_TRIP_EXTERNAL   0x0FU

/** Driver handle (Cube-style). */
typedef struct {
    /** @brief LVDL<3:0> trip-point select, 0..15 (the voltage each code
     *         selects is DS39609B Register 22-1); @ref LVD_TRIP_EXTERNAL
     *         selects the LVDIN pin. */
    uint8_t TripLevel;
    /** @brief Optional low-voltage callback (fires on LVDIF once the
     *         interrupt is armed with @ref EPIC_LVD_EnableInterrupt). */
    void (*TripCallback)(void);
} LVD_HandleTypeDef;

#define LVD_HANDLE_DEFAULT {                                            \
    .TripLevel    = 0x05U,                                              \
    .TripCallback = NULL,                                               \
}

/**
 * @brief  Configure the module from the handle: LVD off, trip level
 *         programmed, LVDIF cleared, LVDIE disabled, callback stored.
 * @param h the LVD handle describing the desired configuration.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or
 *         `h->TripLevel` exceeds 15.
 */
EPIC_StatusTypeDef EPIC_LVD_Init(const LVD_HandleTypeDef *h);

/**
 * @brief  Disable the LVD, clear its flag and interrupt enable, restore
 *         LVDCON to its power-on value and drop the stored callback.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_LVD_DeInit(void);

/**
 * @brief  Program the trip level and set LVDCON<LVDEN>. Does not wait:
 *         poll @ref EPIC_LVD_IsStable before trusting the detector.
 * @param h the handle whose trip level is applied.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL or
 *         `h->TripLevel` exceeds 15.
 */
EPIC_StatusTypeDef EPIC_LVD_Start(const LVD_HandleTypeDef *h);

/**
 * @brief  Clear LVDCON<LVDEN>, leaving the trip level programmed.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_LVD_Stop(void);

/**
 * @brief  Read LVDCON<IRVST>, the internal-reference-stable flag.
 * @return 1 once the reference is stable and the trip point is reliable,
 *         else 0 (always 0 while LVDEN is clear).
 */
uint8_t EPIC_LVD_IsStable(void);

/**
 * @brief  Clear LVDIF and enable the low-voltage interrupt (PIE2<LVDIE>).
 *         Call after @ref EPIC_LVD_IsStable reports 1.
 * @return EPIC_OK on success, EPIC_INVALID if no callback was stored by
 *         Init, EPIC_BUSY if the reference is not yet stable.
 */
EPIC_StatusTypeDef EPIC_LVD_EnableInterrupt(void);

/**
 * @brief Weak LVD ISR, override in user code to add application logic.
 */
void LVD_IRQHandler(void) EPIC_WEAK;

#endif /* PIC18F6520_LVD_H */
