/* epic-cc dispatch, serial + RB + tick tier (epic-combo-rb-uart):
 * USART RX/TX, the RB port-change dispatch and the TIMER2 timebase,
 * the rest cleared. */

#define EPICCC_IRQ_USART 1
#define EPICCC_IRQ_RB 1
#define EPICCC_IRQ_TMR2 1
#include "pic16_irq_dispatch_tiers_inc.h"
