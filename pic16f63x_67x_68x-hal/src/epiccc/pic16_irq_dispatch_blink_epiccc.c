/* epic-cc dispatch, blink tier (Timer0 + RB change only). This family
 * has no Timer2/USART/SSP/ADC/CCP, so the shared
 * pic16_irq_dispatch_epiccc.c (which names their PIR1 flags and calls
 * their handlers unconditionally) cannot compile here; the tiered body
 * below references INTCON/PORTB only. */

#define EPICCC_IRQ_TMR0 1
#define EPICCC_IRQ_RB 1
#include "pic16_irq_dispatch_tiers_inc.h"
