/* PIC16F88X IRQ translation table (DS40001291H Figure 14-10 / §14.11).
 * Consumed by the shared pic14-midrange-core pic14_irq.c body. */

#include "core/pic16_irq.h"
#include "core/pic14_irq_common.h"

const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]       = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 0 },
    [PIC16_IRQ_INT]      = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 0 },
    [PIC16_IRQ_TMR0]     = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 0 },
    [PIC16_IRQ_TMR1]     = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0 },
    [PIC16_IRQ_TMR2]     = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0 },
    [PIC16_IRQ_CCP1]     = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0 },
    [PIC16_IRQ_CCP2]     = { PIC_PIR2_CCP2IF,   PIC_PIE2_CCP2IE,   0, 1 },
    [PIC16_IRQ_SSP]      = { PIC_PIR1_SSPIF,    PIC_PIE1_SSPIE,    0, 0 },
    [PIC16_IRQ_BCL]      = { PIC_PIR2_BCLIF,    PIC_PIE2_BCLIE,    0, 1 },
    [PIC16_IRQ_USART_TX] = { PIC_PIR1_TXIF,     PIC_PIE1_TXIE,     0, 0 },
    [PIC16_IRQ_USART_RX] = { PIC_PIR1_RCIF,     PIC_PIE1_RCIE,     0, 0 },
    [PIC16_IRQ_ADC]      = { PIC_PIR1_ADIF,     PIC_PIE1_ADIE,     0, 0 },
    [PIC16_IRQ_EEPROM]   = { PIC_PIR2_EEIF,     PIC_PIE2_EEIE,     0, 1 },
    [PIC16_IRQ_C1]       = { PIC_PIR2_C1IF,     PIC_PIE2_C1IE,     0, 1 },
    [PIC16_IRQ_C2]       = { PIC_PIR2_C2IF,     PIC_PIE2_C2IE,     0, 1 },
    [PIC16_IRQ_ULPWU]    = { PIC_PIR2_ULPWUIF,  PIC_PIE2_ULPWUIE,  0, 1 },
    [PIC16_IRQ_OSF]      = { PIC_PIR2_OSFIF,    PIC_PIE2_OSFIE,    0, 1 },
};
