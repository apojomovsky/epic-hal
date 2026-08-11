/*
 * Definition of the shared file-scratch buffer (see pic_math_scratch.h).
 * __at-pinned into PIC16 mid-range's bank-independent common RAM
 * (0x70-0x7F): an unpinned buffer landed in bank 1 on PIC16F877A and
 * broke the link. Common RAM is the same physical addresses in every
 * bank, so the routine-level banksel is harmless and the whole window is
 * reachable from any bank.
 *
 * Known overlaps, deliberately not engineered around:
 * - 0x70/0x71 are also used by pic16f87xa-hal's epic_irq_pie_scratch/
 *   epic_bank1_scratch (src/target/pic16_isr_vector.c): images linking the
 *   HAL core emit XC8 warning 1482. Safe as long as user firmware does
 *   not run a PIE-enable/bank1-SFR macro while a math routine is
 *   mid-computation.
 * - 0x7E/0x7F overlap XC8's btemp/wtemp/btemp1 temporaries. The asm
 *   routines use only offsets 0-7; a routine needing a byte past offset
 *   7 must first relocate this buffer.
 */

#include "pic_math_scratch.h"

volatile uint8_t pic16_mscratch[12] __at(0x72);
