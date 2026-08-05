/**
 * @file    peripherals/pic16f87xa_vref.h
 * @brief   Comparator Voltage Reference driver.
 *
 * @details
 *   Source: DS39582B §13.0, Register 13-1. 16-tap resistor ladder:
 *   CVRR=0 gives 0..0.75 VDD in VDD/24 steps; CVRR=1 gives
 *   0.25..0.75 VDD in VDD/32 steps. CVROE routes the output to
 *   RA2/AN2/VREF-, shared with the comparator/ADC reference.
 */

#ifndef PIC16F87XA_VREF_H
#define PIC16F87XA_VREF_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief Voltage-reference range select (CVRCON<CVRR>, Register 13-1).
 */
typedef enum {
    VREF_RANGE_LOW  = 0x0U,   /* 0..0.75 VDD  (steps of VDD/24) */
    VREF_RANGE_HIGH = 0x1U,   /* 0.25..0.75 VDD (steps of VDD/32) */
} VREF_RangeTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    VREF_RangeTypeDef      Range;     /* Low or high range. */
    uint8_t                Value;     /* 0..15, ladder tap. */
    bool                   OutputEnable;  /* Route to RA2. */
    bool                   Enabled;        /* CVREN. */
} VREF_HandleTypeDef;

#define VREF_HANDLE_DEFAULT {                                              \
    .Range         = VREF_RANGE_LOW,                                       \
    .Value         = 0,                                                    \
    .OutputEnable  = false,                                                \
    .Enabled       = false,                                                \
}

EPIC_StatusTypeDef EPIC_VREF_Init(const VREF_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_VREF_DeInit(void);

/**
 * @brief  Compute the nominal output voltage (mV) for a given range +
 *         tap value. Assumes CVRSRC = Vdd_mv.
 */
uint32_t EPIC_VREF_MilliVolts(uint32_t vdd_mv,
                             VREF_RangeTypeDef range,
                             uint8_t value);

#endif /* PIC16F87XA_VREF_H */
