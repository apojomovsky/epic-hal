/**
 * @file    pic_math_scratch.c (PIC16 inline-asm backend)
 * @brief   Definition of the shared 16-byte file-scratch buffer.
 *
 * @details
 *   See pic_math_scratch.h. `__at`-pinned into PIC16 mid-range's
 *   bank-independent common RAM (0x70-0x7F, 16 bytes), so the inline-asm
 *   operand fixups never overflow regardless of how the linker scatters
 *   other statics (an unpinned buffer landed at 0xA0, bank 1, on
 *   PIC16F877A and broke the link; 16F876A was one static away from the
 *   same failure). Common RAM is the same physical addresses in every
 *   bank, so the routine-level `banksel` is harmless and the whole
 *   16-byte window is reachable from any bank. Linked by the PIC16
 *   target build (manifest driver) alongside the other src/pic16/ bodies.
 *
 *   Known limitations, deliberately not engineered around:
 *   - 0x70/0x71 are also used by pic16f87xa-hal's `epic_irq_pie_scratch`/
 *     `epic_bank1_scratch` (defined in src/core/pic16_isr_vector.c), so
 *     any image linking the HAL core (including this module's own
 *     selftest, which has `hal = true`) emits XC8 warning 1482 for the
 *     overlap. Safe in the selftest because nothing there touches those
 *     bytes concurrently with a math call; user firmware must not run a
 *     PIE-enable/bank1-SFR macro while a math routine is mid-computation
 *     (same class as the documented interrupt re-entrancy limitation).
 *   - 0x7E/0x7F overlap XC8's own `btemp`/`wtemp`/`btemp1` temporaries.
 *     The asm routines use only offsets 0-7, so this is safe only as
 *     long as offsets 8-15 stay unused. A future routine that needs a
 *     byte beyond offset 7 must first relocate this buffer.
 */

#include "pic_math_scratch.h"

volatile uint8_t pic16_mscratch[16] __at(0x70);
