/* PIC16F628A IRQ translation table (DS40044G §14.0). Single PIR/PIE
 * pair: comparator and EEPROM sources live in PIR1. Consumed by the
 * shared pic14-midrange-core pic14_irq.c body. */

#include "core/pic16_irq.h"
#include "core/pic14_irq_common.h"

const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]       = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 0 },
    [PIC16_IRQ_INT]      = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 0 },
    [PIC16_IRQ_TMR0]     = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 0 },
    [PIC16_IRQ_TMR1]     = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0 },
    [PIC16_IRQ_TMR2]     = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0 },
    [PIC16_IRQ_CCP1]     = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0 },
    [PIC16_IRQ_USART_TX] = { PIC_PIR1_TXIF,     PIC_PIE1_TXIE,     0, 0 },
    [PIC16_IRQ_USART_RX] = { PIC_PIR1_RCIF,     PIC_PIE1_RCIE,     0, 0 },
    [PIC16_IRQ_EEPROM]   = { PIC_PIR1_EEIF,     PIC_PIE1_EEIE,     0, 0 },
    [PIC16_IRQ_CMP]      = { PIC_PIR1_CMIF,     PIC_PIE1_CMIE,     0, 0 },
};

