/* epic-cc dispatch, serial + tick + EEPROM tier (the PIC18 combo
 * firmwares): USART RX/TX, the TIMER2 timebase and the EEPROM
 * write-complete dispatch. Sources outside this tier's gates are not
 * in the HAL subset and cannot vector (PIE off). Thin wrapper over the
 * shared tiers body; see pic18_irq_dispatch_tiers_inc.h for the
 * dispatch code and gating semantics, and
 * pic18_irq_dispatch_epiccc_tick.c for the sibling tier that adds the
 * TIMER0 tick source. */

#define EPICCC_IRQ_TMR2  1
#define EPICCC_IRQ_USART 1
#define EPICCC_IRQ_EE    1
#include "pic18_irq_dispatch_tiers_inc.h"
