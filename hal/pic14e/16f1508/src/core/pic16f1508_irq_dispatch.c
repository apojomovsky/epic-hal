#include "core/pic16f1508_irq.h"
#include "peripherals/pic16f1508_gpio.h"

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. IOCIF is level-style
 * status, so it is checked together with IOCIE to honour a disabled source.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);

    if ((intcon & (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE)) == (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE))
    {
        IOC_IRQHandler();
    }
}
