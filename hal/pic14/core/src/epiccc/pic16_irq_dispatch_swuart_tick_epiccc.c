/* epic-cc dispatch, software-UART + tick tier (epic-combo-swuart-tick):
 * the swuart bit-times off CCP on a free-running Timer1, so the TMR1
 * and CCP gates mirror the full fan-out, plus the TIMER2 tick. */

#define EPICCC_IRQ_TMR1 1
#define EPICCC_IRQ_CCP1 1
#define EPICCC_IRQ_CCP2 1
#define EPICCC_IRQ_TMR2 1
#define EPICCC_IRQ_USART 1
#include "pic16_irq_dispatch_tiers_inc.h"
