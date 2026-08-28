/* epic-cc dispatch, serial + Timer1 + ADC tier (epic-combo-adc-uart):
 * USART RX/TX, the TIMER1 overflow dispatch and the conversion-done
 * flag, the rest cleared. */

#define EPICCC_IRQ_USART 1
#define EPICCC_IRQ_TMR1 1
#define EPICCC_IRQ_ADC 1
#include "pic16_irq_dispatch_tiers_inc.h"
