/* Shared PIC14 mid-range IRQ table contract: the per-source
 * flag/enable descriptor, the PIR address selector and the table
 * itself. Each family defines the table against its own IRQn enum in
 * src/core/pic16_irq_table.c; the shared pic14_irq.c body consumes it
 * through this header. pir_reg_addr stays a macro: a function-call
 * boundary here lost the returned address before it reached the
 * caller's read/write (XC8 codegen gotcha, see the family README).
 * PIR1 = 0x0C, PIR2 = 0x0D (single-PIR parts have PIR1 only). */

#ifndef PIC14_IRQ_COMMON_H
#define PIC14_IRQ_COMMON_H

#include "pic14_midrange.h"

/* Per-IRQ descriptor: which register the enable / flag bit lives in and
 * at which position. Flags and enables sit in INTCON, PIR1/PIR2 (Bank
 * 0), and PIE1/PIE2 (Bank 1); Bank-1 mirrors of PIR1/PIR2 are at
 * 0x8C/0x8D. */
typedef struct {
    uint8_t flag_mask;     /**< PIR/INTCON bit to test/clear. */
    uint8_t enable_mask;   /**< PIE/INTCON bit to set/clear. */
    uint8_t in_intcon;     /**< 1 = INTCON, 0 = PIR1/PIR2. */
    uint8_t pir_is_pir2;   /**< 1 = PIR2, 0 = PIR1. (Ignored if in_intcon.) */
} irq_desc_t;

#if PIC14MIDRANGE_HAS_PIR2
#define pir_reg_addr(d) ((d)->pir_is_pir2 ? PIC_REG_PIR2 : PIC_REG_PIR1)
#else
/* Single-PIR parts (628A): the table never sets pir_is_pir2, and the
 * register does not exist, so do not name it. */
#define pir_reg_addr(d) (PIC_REG_PIR1)
#endif

extern const irq_desc_t irq_table[];
extern const unsigned IRQ_TABLE_SIZE;

#endif /* PIC14_IRQ_COMMON_H */
