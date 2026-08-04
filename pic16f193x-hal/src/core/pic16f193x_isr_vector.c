/**
 * @file    pic16f193x_isr_vector.c
 * @brief   Real-target interrupt-vector entry (XC8 target build only).
 *
 * @details
 *   Single PIC16F193X vector at 0x0004 (DS41364B §4.0): this file's
 *   `__interrupt()` handler calls the shared @ref pic8_dispatch_all_irqs
 *   fan-out. XC8-only build (the host has no interrupt vector); the host
 *   harness registers the same dispatcher as its sim IRQ callback instead.
 *
 *   The Enhanced Mid-range core saves W/STATUS/BSR/FSR0/FSR1/PCLATH to
 *   shadow registers in hardware on interrupt entry and restores them on
 *   RETFIE (DS41364B §4.1), so no manual push/pop is needed here (unlike
 *   classic PIC16F87XA).
 *
 *   Although the auto-context-save shape avoids the manual push/pop
 *   that forced the classic family's pushpop scratch byte, the
 *   platform's PIE1/2/3 read-modify-write macros still need an
 *   `__at()`-pinned common-RAM scratch byte to bridge the bank-select
 *   gap. The plain-C RMW that the foundation shipped silently
 *   produced `movwf fsr1l; clrf fsr1h` indirect addressing with
 *   FSR1H=0 (the wrong bank for PIE registers, which all live in
 *   bank 1); pin `pic8_irq_pie_scratch` to bank-independent common
 *   RAM at 0x70 (DS41364B Table 2-3, accessible from any bank) so
 *   the inline-asm PIE bit helpers can move the value through W
 *   without disturbing any C-level local. Mirrors the fix in
 *   pic16f87xa-hal/include/target/pic16f87xa_platform.h's
 *   scratch-byte extern + pic16_isr_vector.c's `__at()`-pinned
 *   definition (see pic16f87xa-hal/docs/ARCHITECTURE.md, Finding 1
 *   for the original classic-PIC16 codegen evidence; this family's
 *   ARCHITECTURE.md Finding 2 has the Enhanced-Mid-range evidence
 *   plus the assembly transition from the broken macro to the fix).
 */

#include "core/pic16f193x_irq.h"

/* Definition for target/pic16f193x_platform.h's
 * `extern volatile uint8_t pic8_irq_pie_scratch`; `__at()`-pinned into
 * PIC16F193X's bank-independent common RAM (Table 2-3, 0x70). The same
 * shape as pic16_isr_vector.c in pic16f87xa-hal. */
volatile uint8_t pic8_irq_pie_scratch __at(0x70);

/* Strong extern prototype instead of including pic8_harness.h, same
 * pattern pic16f193x_irq_dispatch.c uses for the peripheral handlers. */
extern void pic8_dispatch_all_irqs(void);

/**
 * @brief  Single PIC16F193X interrupt-vector handler. Delegates to the
 *         shared dispatcher so the fan-out logic lives in one place.
 */
void __interrupt() PIC16F193X_IRQ_Handler(void)
{
    pic8_dispatch_all_irqs();
}
