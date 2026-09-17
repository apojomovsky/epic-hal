/* epic-cc dispatch, blink tier (Timer0 + RB change only: the body
 * below references INTCON/PORTB alone, so it links where the shared
 * pic16_irq_dispatch_epiccc.c does not fit, be it the die's PIR1 map
 * lacking its Timer2/CCP1 flags or a slice kept at the blink
 * minimum). */

#define EPICCC_IRQ_TMR0 1
#define EPICCC_IRQ_RB 1
#include "pic16_irq_dispatch_tiers_inc.h"
