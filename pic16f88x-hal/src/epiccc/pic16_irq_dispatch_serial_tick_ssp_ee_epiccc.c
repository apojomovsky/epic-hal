/* epic-cc dispatch, serial + tick + SSP + EEPROM tier
 * (epic-combo-uart-ssp): USART RX/TX, the TIMER2 timebase and the
 * MSSP/EEPROM flags under their polled-protocol drivers, the rest
 * cleared. */

#define EPICCC_IRQ_USART 1
#define EPICCC_IRQ_TMR2 1
#define EPICCC_IRQ_SSP 1
#define EPICCC_IRQ_EE 1
#include "pic16_irq_dispatch_tiers_inc.h"
