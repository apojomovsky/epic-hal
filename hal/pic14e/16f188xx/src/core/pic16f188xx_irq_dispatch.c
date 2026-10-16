#include "core/pic16f188xx_irq.h"
#include "peripherals/pic16f188xx_gpio.h"

/* PIR0 flags with a driver: IOCIF only. INT and TMR0 have no driver and
 * no API enables them, so no unowned source can reach the vector. */
#define IOC_PIR0_MASK ((uint8_t)PIC_PIR0_IOCIF)

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. IOC runs its handler
 * only when PIR0.IOCIF and PIE0.IOCIE are both set. A disabled IOC keeps
 * its flag as status for polling through EPIC_IRQ_GetFlag. Each test
 * reads one register into WREG, so the dispatcher keeps no RAM
 * temporaries at all (see the snapshot note in pic16f188xx_gpio.c).
 */
void epic_dispatch_all_irqs(void)
{
    if ((EPIC_REG8(PIC_REG_PIR0) & IOC_PIR0_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE0) & IOC_PIR0_MASK) != 0U)
        {
            IOC_IRQHandler();
        }
    }
}
