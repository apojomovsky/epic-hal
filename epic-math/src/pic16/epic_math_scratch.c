/*
 * Shared file-scratch buffer (see epic_math_scratch.h), __at-pinned
 * into common RAM (0x70-0x7F, bank-independent): an unpinned buffer
 * lands in banked RAM and breaks the link. Known overlaps: 0x70/0x71
 * are also the HAL ISR scratch (XC8 warning 1482 in HAL-linked images;
 * safe unless firmware runs a PIE/bank1 macro mid-computation) and
 * 0x7E/0x7F are XC8's btemp/wtemp (routines use offsets 0-7 only).
 */

#include "epic_math_scratch.h"

volatile uint8_t pic16_mscratch[12] __at(0x72);
