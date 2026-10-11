/**
 * PIC16F1508/1509 CLC gate helper. Builds the CLCxGLSg bits for one data
 * input so callers never repeat the N/T bit layout. Header-only: static inline,
 * so no link dependency. Bit layout from the pinned DFP 1.9.258 header, not
 * yet checked against the datasheet.
 */
#ifndef PIC16F1508_CLC_GLS_H
#define PIC16F1508_CLC_GLS_H

#include <stdint.h>
#include "core/hal_status.h"

/**
 * @brief GLS bits for one data input of a gate.
 * @details Data input i (1..4) owns bit 2*(i-1) for the negate term (DxN) and
 * bit 2*(i-1)+1 for the true term (DxT). Combine inputs by OR into one
 * CLCxGLSg register.
 * @param input Data input number, 1..4. Any other value returns 0.
 * @param negate Nonzero selects the inverted input (DxN).
 * @param true_en Nonzero selects the true input (DxT).
 * @return Bits to OR into a CLCxGLSg register.
 */
static inline uint8_t EPIC_CLC_GLSInput(uint8_t input, uint8_t negate, uint8_t true_en)
{
    uint8_t shift;

    if ((input < 1U) || (input > 4U))
    {
        return 0U;
    }
    shift = (uint8_t)(2U * (input - 1U));
    return (uint8_t)(((negate != 0U) ? EPIC_BIT(shift) : 0U) |
                     ((true_en != 0U) ? EPIC_BIT(shift + 1U) : 0U));
}

#endif
