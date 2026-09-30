/* PIC16F818/819 IRQ translation table (DS39598F §12.10).
 * Consumed by the shared pic14-midrange-core pic14_irq.c body. The
 * INTCON trio needs no bank switch; TMR1/TMR2/CCP1/SSP/ADC ride the
 * PIR1 pair and the EEPROM event rides PIR2, which is why only the
 * EEPROM row sets pir_is_pir2. */

#include "core/pic16_irq.h"
#include "core/pic14_irq_common.h"

const irq_desc_t irq_table[] = {
    [PIC16_IRQ_RB]     = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 1, 0 },
    [PIC16_IRQ_INT]    = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 1, 0 },
    [PIC16_IRQ_TMR0]   = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 1, 0 },
    [PIC16_IRQ_TMR1]   = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0, 0 },
    [PIC16_IRQ_TMR2]   = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0, 0 },
    [PIC16_IRQ_CCP1]   = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0, 0 },
    [PIC16_IRQ_SSP]    = { PIC_PIR1_SSPIF,    PIC_PIE1_SSPIE,    0, 0, 0 },
    [PIC16_IRQ_ADC]    = { PIC_PIR1_ADIF,     PIC_PIE1_ADIE,     0, 0, 0 },
    [PIC16_IRQ_EEPROM] = { PIC_PIR2_EEIF,     PIC_PIE2_EEIE,     0, 0, 1 },
};
