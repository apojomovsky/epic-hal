/* epic-cc dispatch, serial tier (epic-serial, epic-console): USART RX/TX
 * dispatch, every other source cleared. Excludes GPIO/Timer0/Timer2/SSP
 * so their drivers stay out of the 368-byte GPR overlay entirely. */

#define EPICCC_IRQ_USART 1
#include "pic16_irq_dispatch_tiers_inc.h"
