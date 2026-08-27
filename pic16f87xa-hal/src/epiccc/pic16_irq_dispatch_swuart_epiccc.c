/* epic-cc dispatch, software-UART tier (epic-swuart): Timer1 overflow,
 * CCP1 capture (RX) and CCP2 compare (TX) dispatch, the rest cleared.
 * The software UART bit-times off CCP on a free-running Timer1, so the
 * TMR1 gate mirrors the full fan-out. */

#define EPICCC_IRQ_TMR1 1
#define EPICCC_IRQ_CCP1 1
#define EPICCC_IRQ_CCP2 1
#include "pic16_irq_dispatch_tiers_inc.h"
