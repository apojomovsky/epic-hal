/**
 * @file    target/pic16f193x_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   Target half of the SFR mapping layer (paired with
 *   host/pic16f193x_platform.h); the build's include path picks which
 *   resolves, so pic16f193x.h includes this name unconditionally with no
 *   `#ifdef`. SFR access is a direct volatile dereference of the literal
 *   address; XC8 has no weak symbols, so EPIC_WEAK is empty.
 *
 *   Plain C reads against compile-time-constant SFR tokens compile to
 *   `movlb N; movf <sfr>,w` once XC8 sees which bank each token belongs
 *   to (it pulls the bank map from the DFP at -O2). That is why
 *   `EPIC_REG8(addr)` dereferences the literal address without bank
 *   switching: XC8 sets BSR itself. Verified on the Enhanced Mid-range
 *   core by the Foundation's codegen probe (`docs/adding-a-device.md` §4,
 *   Finding 1 of the foundation ARCHITECTURE.md).
 *
 *   PIE1/PIE2/PIE3 enable/disable is NOT a plain-C read-modify-write:
 *   on this core, the original plain-C RMW shape silently produced
 *   `movwf fsr1l; clrf fsr1h` indirect addressing with FSR1H=0, which
 *   is not what PIE1 lives behind (PIE1 is at 0x91 in bank 1, not in
 *   the linear address space FSR1 reads with FSR1H=0). The fix mirrors
 *   pic16f87xa-hal's proven `__at()`-pinned scratch byte +
 *   inline-asm bank-switch pattern, on the Enhanced Mid-range idiom
 *   (`movlb 1` sets BSR=1 in one instruction). All three PIEs are in
 *   bank 1 (DS41364B Tables 2-4), so a single `movlb 1` covers all
 *   three pir_index values (0/1/2). See
 *   pic16f193x-hal/docs/ARCHITECTURE.md Finding 2 for the failing
 *   codegen evidence (FSR1H=0 read of address 0x91) and the fixed
 *   assembly (`movlb 1; iorwf PIE1,f; movlb 0`).
 */

#ifndef PIC16F193X_PLATFORM_H
#define PIC16F193X_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define EPIC_WEAK

/* Placement pins map to XC8's __at(addr); the host header no-ops it. */
#define EPIC_PLACE(addr)         __at(addr)

/* SFR access resolves to a direct volatile dereference of the address. */
#define EPIC_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define epic_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define EPIC_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

/* File-scope symbol the asm below needs (inline asm can only address
 * file-scope symbols, see epic-math/docs/ARCHITECTURE.md's "Inline-asm
 * binding"). `__at()`-pinned into bank-independent common RAM (0x70,
 * PIC16F193X Table 2-3, accessable across all banks) in
 * pic16f193x_isr_vector.c rather than left to the linker's default
 * placement, which scatters unpinned statics by best-fit, not
 * declaration order (AGENTS.md). Same shape as
 * pic16f87xa-hal/include/target/pic16f87xa_platform.h. */
extern volatile uint8_t epic_irq_pie_scratch __at(0x70);

/* PIE1/PIE2/PIE3 enable/disable via inline asm on the Enhanced
 * Mid-range core. `pir_index` is 0 for PIE1, 1 for PIE2, 2 for PIE3
 * (DS41364B §4.5). All three PIEs are in bank 1, so a single `movlb 1`
 * selects the right bank for any pir_index; the literal `PIE1` /
 * `PIE2` / `PIE3` symbols in pic16f1937.inc resolve to the same offset
 * (0x11/0x12/0x13 within bank 1) and the asm picks the right symbol
 * via the C switch. Bits go in via the `__at()`-pinned scratch byte so
 * the load of W can happen before the bank switch without disturbing
 * any C-level local (matches pic16f87xa-hal's proven pattern). */
#define EPIC_PIE_ENABLE_BIT(pir_index, mask)                              \
    do {                                                                  \
        epic_irq_pie_scratch = (uint8_t)(mask);                          \
        if ((pir_index) == 2U) {                                         \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("iorwf PIE3,f");                                         \
            asm("movlb 0");                                              \
        } else if ((pir_index) == 1U) {                                  \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("iorwf PIE2,f");                                         \
            asm("movlb 0");                                              \
        } else {                                                         \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("iorwf PIE1,f");                                         \
            asm("movlb 0");                                              \
        }                                                                \
    } while (0)

#define EPIC_PIE_DISABLE_BIT(pir_index, mask)                             \
    do {                                                                  \
        epic_irq_pie_scratch = (uint8_t)~(uint8_t)(mask);                 \
        if ((pir_index) == 2U) {                                         \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("andwf PIE3,f");                                         \
            asm("movlb 0");                                              \
        } else if ((pir_index) == 1U) {                                  \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("andwf PIE2,f");                                         \
            asm("movlb 0");                                              \
        } else {                                                         \
            asm("movf _epic_irq_pie_scratch,w");                          \
            asm("movlb 1");                                              \
            asm("andwf PIE1,f");                                         \
            asm("movlb 0");                                              \
        }                                                                \
    } while (0)

/* Read the TMR1IE bit (PIE1 bit 0, Bank 1) into a uint8_t output
 * variable, via the same `movlb 1` bank-switch idiom as the
 * enable/disable macros above. Used by the shared interrupt dispatcher
 * to skip TIMER1_IRQHandler when TMR1IE is disabled: Timer1 free-runs
 * with its overflow interrupt off (epic-swuart needs the counter but
 * never the overflow), so TMR1IF latches at every 65536-cycle wrap and
 * would otherwise make every subsequent CCP event pay the full handler
 * cost before its own dispatch (the same hazard measured on PIC16F87XA,
 * see docs/superpowers/plans/probe-swuart-rx-hotpath.md). */
#define EPIC_PIE1_READ_TMR1IE(out_var)                                   \
    do {                                                                  \
        asm("movlb 1");                                                  \
        asm("movf PIE1,w");                                              \
        asm("movlb 0");                                                  \
        asm("movwf _epic_irq_pie_scratch");                              \
        (out_var) = epic_irq_pie_scratch;                                \
    } while (0)

/* Same shape as EPIC_PIE1_READ_TMR1IE, for the TXIE bit (PIE1 bit 4,
 * Bank 1). Used by the shared interrupt dispatcher to skip
 * USART_TX_IRQHandler when TXIE is disabled: TXIF is a read-only
 * status bit that stays set whenever TXREG is empty, so an un-gated
 * TXIF branch fires the TX handler on every ISR from any source. */
#define EPIC_PIE1_READ_TXIE(out_var) EPIC_PIE1_READ_TMR1IE(out_var)

/* Same shape, for the EEIE bit (PIE2 bit 4, Bank 1; all three PIEs
 * live in bank 1 on this family, DS41364B Tables 2-4). Used by the
 * shared interrupt dispatcher to skip EEPROM_IRQHandler when EEIE is
 * off: EEPROM completion is often polled with EEIE disabled, and an
 * unconditional dispatch would clear the polled flag from a live ISR
 * (combination-matrix C7 finding). */
#define EPIC_PIE2_READ_EEIE(out_var)                                   \
    do {                                                                \
        asm("movlb 1");                                                \
        asm("movf PIE2,w");                                            \
        asm("movlb 0");                                                \
        asm("movwf _epic_irq_pie_scratch");                            \
        (out_var) = epic_irq_pie_scratch;                              \
    } while (0)

#endif /* PIC16F193X_PLATFORM_H */
