/**
 * @file    pic16_isr_vector.c
 * @brief   Real-target interrupt-vector entry (XC8 target build only).
 *
 * @details
 *   The PIC16F87XA family has a single interrupt vector at 0x0004
 *   (DS39582B §14.11). On a real XC8 target this file installs the
 *   `__interrupt()` handler the CPU jumps to on any enabled IRQ. It just
 *   calls the shared fan-out @ref pic8_dispatch_all_irqs, which
 *   routes to every peripheral IRQHandler.
 *
 *   This file is built only by the XC8 Makefile (real target). The host
 *   CMake build does NOT compile it, `__interrupt()` is an XC8-specific
 *   attribute and the host has no interrupt vector, so the selection is
 *   done at build time, with no `#ifdef` here. On the host the harness
 *   registers the same dispatcher as the sim IRQ callback instead.
 */

#include "core/pic16_irq.h"

/* Definition for target/pic16f87xa_platform.h's `extern volatile
 * uint8_t pic8_irq_pie_scratch;` (see PIC8_PIE_ENABLE_BIT's header
 * comment there for the full account of what this scratch byte is for
 * and why it has to live in a target-only translation unit). This file
 * is as good a home as any target-only one: already exists, already
 * interrupt-machinery-themed, and this repo avoids adding new files
 * without a real reason to. `__at(0x70)` pins it into PIC16 mid-range's
 * bank-independent common RAM. */
volatile uint8_t pic8_irq_pie_scratch __at(0x70);

/* Declared in pic8_harness.h (shared). Declared here as a strong extern
 * prototype instead of including that header, to keep the harness's
 * unused inline (pic8_harness_report) out of this translation unit's
 * warning surface — same pattern pic16_irq_dispatch.c uses for the
 * peripheral handlers. */
extern void pic8_dispatch_all_irqs(void);

/**
 * @brief  Single PIC16 interrupt-vector handler. Delegates to the shared
 *         dispatcher so the fan-out logic lives in one place.
 */
void __interrupt() PIC16_IRQ_Handler(void)
{
    pic8_dispatch_all_irqs();
}
