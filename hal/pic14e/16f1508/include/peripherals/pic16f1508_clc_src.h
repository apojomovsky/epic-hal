/**
 * PIC16F1508/1509 CLC data-input table. Names the 16 LCx_in signals of
 * DS40001609E Table 24-1 (page 258) and maps one to the select code of a data
 * multiplexer (D1S..D4S, Registers 24-3 and 24-4). Header-only: static inline,
 * so no link dependency.
 */
#ifndef PIC16F1508_CLC_SRC_H
#define PIC16F1508_CLC_SRC_H

#include <stdint.h>

/**
 * Value returned by EPIC_CLC_SourceCode when a multiplexer cannot select the
 * input. Above the 0..7 field range, so EPIC_CLC_Init rejects it unwritten.
 */
#define EPIC_CLC_SRC_INVALID 0xFFU

/**
 * LCx_in[n] index of Table 24-1. Rows 0..11 are the same signal on every cell,
 * CLC_IN_CLCIN0 and CLC_IN_CLCIN1 being the CLCxIN0 and CLCxIN1 pins of the
 * cell being configured. Rows 12..15 name a different signal on each cell, so
 * they are listed per cell.
 */
typedef enum
{
    CLC_IN_CLCIN0       = 0,
    CLC_IN_CLCIN1       = 1,
    CLC_IN_C1OUT        = 2,  /* C1OUT_sync */
    CLC_IN_C2OUT        = 3,  /* C2OUT_sync */
    CLC_IN_FOSC         = 4,
    CLC_IN_T0_OVERFLOW  = 5,
    CLC_IN_T1_OVERFLOW  = 6,
    CLC_IN_T2_MATCH     = 7,
    CLC_IN_LC1_OUT      = 8,
    CLC_IN_LC2_OUT      = 9,
    CLC_IN_LC3_OUT      = 10,
    CLC_IN_LC4_OUT      = 11,

    CLC1_IN_NCO1_OUT    = 12,
    CLC1_IN_HFINTOSC    = 13,
    CLC1_IN_PWM3_OUT    = 14,
    CLC1_IN_PWM4_OUT    = 15,

    CLC2_IN_LFINTOSC    = 12,
    CLC2_IN_FRC         = 13,
    CLC2_IN_PWM1_OUT    = 14,
    CLC2_IN_PWM2_OUT    = 15,

    CLC3_IN_TX_OUT      = 12, /* EUSART */
    CLC3_IN_LFINTOSC    = 13,
    CLC3_IN_PWM2_OUT    = 14,
    CLC3_IN_PWM3_OUT    = 15,

    CLC4_IN_SCK_OUT     = 12, /* MSSP */
    CLC4_IN_SDO_OUT     = 13, /* MSSP */
    CLC4_IN_PWM1_OUT    = 14,
    CLC4_IN_PWM4_OUT    = 15
} CLC_InputTypeDef;

/**
 * @brief Select code of a data multiplexer for one LCx_in input.
 * @details Each multiplexer sees eight of the 16 inputs, and an input is on
 * two multiplexers, with a different code on each (Table 24-1). D1 sees
 * LCx_in[0..7], D2 sees [4..11], D3 sees [8..15], and D4 sees [12..15] at
 * codes 0..3 then [0..3] at codes 4..7. Store the result in
 * CLC_HandleTypeDef.Source[mux - 1].
 * @param mux Data multiplexer number, 1..4.
 * @param input LCx_in index 0..15, normally a CLC_InputTypeDef value.
 * @return Select code 0..7, or EPIC_CLC_SRC_INVALID when the multiplexer
 * cannot select that input or an argument is out of range.
 */
static inline uint8_t EPIC_CLC_SourceCode(uint8_t mux, uint8_t input)
{
    switch (mux)
    {
    case 1U:
        return (input <= 7U) ? input : EPIC_CLC_SRC_INVALID;
    case 2U:
        return ((input >= 4U) && (input <= 11U)) ? (uint8_t)(input - 4U) : EPIC_CLC_SRC_INVALID;
    case 3U:
        return ((input >= 8U) && (input <= 15U)) ? (uint8_t)(input - 8U) : EPIC_CLC_SRC_INVALID;
    case 4U:
        if (input <= 3U)
        {
            return (uint8_t)(input + 4U);
        }
        return ((input >= 12U) && (input <= 15U)) ? (uint8_t)(input - 12U) : EPIC_CLC_SRC_INVALID;
    default:
        return EPIC_CLC_SRC_INVALID;
    }
}

#endif
