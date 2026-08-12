/*
 * One shared scratch buffer for every PIC16 asm leaf routine (XC8 inline
 * asm only addresses file-scope symbols, see ARCHITECTURE.md), sized to
 * fit PIC16F87XA's RAM budget. Hidden behind the by-value public API;
 * not safe against interrupt re-entrancy of the same routine.
 */

#ifndef PIC16_MATH_SCRATCH_H
#define PIC16_MATH_SCRATCH_H

#include <stdint.h>

/** Shared scratch for all PIC16 asm leaf routines (see epic_math_scratch.c). */
extern volatile uint8_t pic16_mscratch[12];

#endif /* PIC16_MATH_SCRATCH_H */
