/* 16F506 dual-comparator driver (DS41268D §7.0, Register 7-2/7-3;
 * reference §8.0, Register 8-1). Two independent comparators with no
 * shared mode mux and no interrupt vector on this core (DS41213D
 * §4.0), so the handle mirrors the pic14 shape minus its
 * ChangeCallback/CMIF. Init programs CxON/CxPOL by
 * read-modify-write, preserving the input-mux and output-enable
 * bits it does not own; VRCON is fully owned (tap, range, output,
 * enable). Only the 16F506 carries this block. */

#ifndef PIC16F5X_COMP_H
#define PIC16F5X_COMP_H

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"

/** Driver handle (Cube-style). No callback field: no IRQ exists. */
typedef struct {
    bool                  C1Enabled;    /* CM1CON0<C1ON>. */
    bool                  C1Inverted;   /* Inverted C1 output. */
    bool                  C2Enabled;    /* CM2CON0<C2ON>. */
    bool                  C2Inverted;   /* Inverted C2 output. */
    bool                  VrefEnabled;  /* VRCON<VREN>. */
    bool                  VrefOutput;   /* VRCON<VROE>, CVREF to RC2. */
    bool                  VrefRange;    /* VRCON<VRR>, 1 = low range. */
    uint8_t               VrefTap;      /* VRCON<VR3:VR0>, low 4 bits. */
} COMP_HandleTypeDef;

/** Default initialiser: C1 on, non-inverted; C2 off; reference off. */
#define COMP_HANDLE_DEFAULT {                                           \
    .C1Enabled         = true,                                          \
    .C1Inverted        = false,                                         \
    .C2Enabled         = false,                                         \
    .C2Inverted        = false,                                         \
    .VrefEnabled       = false,                                         \
    .VrefOutput        = false,                                         \
    .VrefRange         = false,                                         \
    .VrefTap           = 0x00U,                                         \
}

/**
 * @brief  Configure the comparators and reference from the handle.
 *         Programs CM1CON0/CM2CON0 (enable, polarity) and VRCON.
 * @param h handle with enable, polarity and reference selection.
 * @return EPIC_OK on success, EPIC_INVALID if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h);

/**
 * @brief  Disable both comparators and the reference (clear CxON and
 *         VREN), preserving the mux and output-enable bits. Returns
 *         the shared pins to digital I/O.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_COMP_DeInit(void);

/**
 * @brief Read the C1 output bit (CM1CON0<C1OUT>, 1 = VIN+ > VIN-).
 * @return 1 if the C1 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C1Out(void);

/**
 * @brief Read the C2 output bit (CM2CON0<C2OUT>, 1 = VIN+ > VIN-).
 * @return 1 if the C2 output is high, 0 otherwise.
 */
uint8_t EPIC_COMP_C2Out(void);

#endif /* PIC16F5X_COMP_H */
