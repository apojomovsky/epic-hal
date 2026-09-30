/* epic-cc dispatch, serial + all-timers tier (epic-combo-multitimer):
 * USART RX/TX and the TIMER0/1/2 overflow dispatch, the rest cleared. */

#define EPICCC_IRQ_USART 1
#define EPICCC_IRQ_TMR0 1
#define EPICCC_IRQ_TMR1 1
#define EPICCC_IRQ_TMR2 1
#include "pic16_irq_dispatch_tiers_inc.h"
