/* Comparator Voltage Reference (CVREF) driver. Source: DS40001291H
 * §8.10, Register 8-5. 16-tap resistor ladder: VRR=0 (high range) gives
 * VDD/4..VDD in VDD/32 steps; VRR=1 (low range) gives 0..0.625 VDD in
 * VDD/24 steps. VRSS selects the reference source (VDD-VSS or VREF+-
 * VREF- pins); VROE routes the output to RA2/AN2/CVREF. */

#ifndef PIC16F88X_VREF_H
#define PIC16F88X_VREF_H

#include "pic16f88x.h"
#include "pic16f88x_sfr.h"

/**
 * @brief Voltage-reference range select (VRCON<VRR>, Register 8-5).
 */
typedef enum {
    VREF_RANGE_HIGH = 0x0U,   /* VDD/4 + (n/32)*VDD, 0.25..1.0 VDD. */
    VREF_RANGE_LOW  = 0x1U,   /* (n/24)*VDD, 0..0.625 VDD. */
} VREF_RangeTypeDef;

/**
 * @brief Reference source select (VRCON<VRSS>, Register 8-5).
 */
typedef enum {
    VREF_SRC_VDD_VSS   = 0x0U,   /**< CVRSRC = VDD - VSS. */
    VREF_SRC_VREFP_VREFN = 0x1U, /**< CVRSRC = VREF+ - VREF- pins. */
} VREF_SourceTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    VREF_RangeTypeDef      Range;     /* High or low range (VRR). */
    VREF_SourceTypeDef     Source;    /* VDD-VSS or VREF pins (VRSS). */
    uint8_t                Value;     /* 0..15, ladder tap (VR<3:0>). */
    bool                   OutputEnable;  /* VROE: route to RA2. */
    bool                   Enabled;        /* VREN. */
} VREF_HandleTypeDef;

#define VREF_HANDLE_DEFAULT {                                              \
    .Range         = VREF_RANGE_HIGH,                                      \
    .Source        = VREF_SRC_VDD_VSS,                                     \
    .Value         = 0,                                                    \
    .OutputEnable  = false,                                                \
    .Enabled       = false,                                                \
}

/**
 * @brief  Initialize the voltage reference with the given handle.
 *         Programs VRCON (range, source, tap, output enable, enable).
 * @param h handle with Range, Source, Value, OutputEnable, Enabled.
 * @return EPIC_OK on success, EPIC_ERROR if `h` is NULL.
 */
EPIC_StatusTypeDef EPIC_VREF_Init(const VREF_HandleTypeDef *h);

/**
 * @brief  De-initialize the voltage reference. Disables it and clears
 *         the output enable.
 * @return EPIC_OK on success.
 */
EPIC_StatusTypeDef EPIC_VREF_DeInit(void);

/**
 * @brief  Compute the nominal output voltage (mV) for a given range +
 *         tap value (DS40001291H Register 8-5).
 * @param vdd_mv the supply voltage in millivolts.
 * @param range VREF_RANGE_HIGH or VREF_RANGE_LOW.
 * @param value the ladder tap, 0..15.
 * @return the nominal output voltage in millivolts.
 */
uint32_t EPIC_VREF_MilliVolts(uint32_t vdd_mv,
                             VREF_RangeTypeDef range,
                             uint8_t value);

#endif /* PIC16F88X_VREF_H */
