/* epic-cc dispatch, blink tier (Timer0 + RB change only, for
 * families without Timer2/USART/SSP/ADC/CCP: the shared
 * pic16_irq_dispatch_epiccc.c names their PIR1 flags and calls their
 * handlers unconditionally, so it cannot compile there; the tiered
 * body below references INTCON/PORTB only). */

#define EPICCC_IRQ_TMR0 1
#define EPICCC_IRQ_RB 1
#include "pic16_irq_dispatch_tiers_inc.h"
