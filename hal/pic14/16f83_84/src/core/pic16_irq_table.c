/* PIC16F83/84/84A IRQ translation table (DS35007B §14.11, DS30189
 * §14.11). No PIR/PIE pair: Timer0, INT and RB are INTCON residents
 * and the EEPROM enable (EEIE) is INTCON<6>, while its flag (EEIF) is
 * EECON1<4>. Consumed by the shared pic14-midrange-core pic14_irq.c
 * body. */

#include "core/pic16_irq.h"
#include "core/pic14_irq_common.h"

const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]       = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 1, 0 },
    [PIC16_IRQ_INT]      = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 1, 0 },
    [PIC16_IRQ_TMR0]     = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 1, 0 },
    [PIC16_IRQ_EEPROM]   = { PIC_EECON1_EEIF,   PIC_INTCON_EEIE,   0, 1, 0 },
};
