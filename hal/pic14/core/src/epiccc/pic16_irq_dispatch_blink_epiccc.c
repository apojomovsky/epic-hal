/* epic-cc dispatch, blink tier (Timer0 + RB change only: the body
 * below references INTCON/PORTB alone, so it links where a die's
 * PIR1 map has no TMR1IF token for the shared
 * pic16_irq_dispatch_epiccc.c fan-out, or a slice is kept at the
 * blink minimum). */

#define EPICCC_IRQ_TMR0 1
#define EPICCC_IRQ_RB 1
#include "pic16_irq_dispatch_tiers_inc.h"
