/* PIC16F63x/67x/68x IRQ translation table (DS40001262F §14.0,
 * DS40300 §14.0). PORTA/B change, INT and TMR0 live in INTCON; TMR1
 * in PIR1; EEPROM completion lives in PIR2 on the 4-bank parts and in
 * PIR1<7> on the 2-bank parts. C1/C2/OSF rows name PIR2 constants on
 * every part but only the dual-comparator shapes enable them.
 * Consumed by the shared pic14-midrange-core pic14_irq.c body. */

#include "core/pic16_irq.h"
#include "core/pic14_irq_common.h"

const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]     = { PIC_INTCON_RBIF, PIC_INTCON_RBIE, 1, 1, 0 },
    [PIC16_IRQ_INT]    = { PIC_INTCON_INTF, PIC_INTCON_INTE, 1, 1, 0 },
    [PIC16_IRQ_TMR0]   = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 1, 0 },
    [PIC16_IRQ_TMR1]   = { PIC_PIR1_TMR1IF, PIC_PIE1_TMR1IE, 0, 0, 0 },
#if PIC14MIDRANGE_HAS_EE_PIR1
    [PIC16_IRQ_EEPROM] = { PIC_PIR1_EEIF,   PIC_PIE1_EEIE,   0, 0, 0 },
#else
    [PIC16_IRQ_EEPROM] = { PIC_PIR2_EEIF,   PIC_PIE2_EEIE,   0, 0, 1 },
#endif
    [PIC16_IRQ_C1]     = { PIC_PIR2_C1IF,   PIC_PIE2_C1IE,   0, 0, 1 },
    [PIC16_IRQ_C2]     = { PIC_PIR2_C2IF,   PIC_PIE2_C2IE,   0, 0, 1 },
    [PIC16_IRQ_OSF]    = { PIC_PIR2_OSFIF,  PIC_PIE2_OSFIE,  0, 0, 1 },
};
