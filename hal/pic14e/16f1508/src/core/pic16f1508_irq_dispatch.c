#include "core/pic16f1508_irq.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_gpio.h"

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. A source runs its handler
 * only when its flag and its enable are both set. A disabled source keeps its
 * flag as status for polling through EPIC_IRQ_GetFlag.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);

    if ((intcon & (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE)) == (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE))
    {
        IOC_IRQHandler();
    }

    /* Re-read after the IOC handler: its user callback may disable a CLC source. */
    uint8_t clc_pending = EPIC_REG8(PIC_REG_PIR3) & EPIC_REG8(PIC_REG_PIE3);

    /* CLCnIF and CLCnIE share bit n-1; PIR3 & PIE3 keeps only enabled, pending CLCs. */
    if (clc_pending & PIC_PIR3_CLC1IF) CLC1_IRQHandler();
    if (clc_pending & PIC_PIR3_CLC2IF) CLC2_IRQHandler();
    if (clc_pending & PIC_PIR3_CLC3IF) CLC3_IRQHandler();
    if (clc_pending & PIC_PIR3_CLC4IF) CLC4_IRQHandler();
}
