#include "core/pic16f188xx_irq.h"

/**
 * @brief Single interrupt vector. Hardware saves context on this core, so no
 * manual save or restore is needed here.
 */
void __interrupt() PIC16F188XX_IRQ_Handler(void)
{
    epic_dispatch_all_irqs();
}
