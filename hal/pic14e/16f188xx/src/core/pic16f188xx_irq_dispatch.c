#include "core/pic16f188xx_irq.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"

/* PIR0 flags with a driver: IOCIF and TMR0IF; PIR4 flags: TMR1IF and
 * TMR2IF. INT has no driver and no API enables it, and the other PIR4
 * bits (Timer3..6) have none either, so no unowned source can reach
 * the vector. */
#define IOC_PIR0_MASK ((uint8_t)PIC_PIR0_IOCIF)
#define TMR0_PIR0_MASK ((uint8_t)PIC_PIR0_TMR0IF)
#define TMR1_PIR4_MASK ((uint8_t)PIC_PIR4_TMR1IF)
#define TMR2_PIR4_MASK ((uint8_t)PIC_PIR4_TMR2IF)

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. Each source runs
 * its handler only when its PIRx flag and PIEx enable are both set.
 * A disabled source keeps its flag as status for polling through
 * EPIC_IRQ_GetFlag. Each test reads one register into WREG, so the
 * dispatcher keeps no RAM temporaries at all (see the snapshot note
 * in pic16f188xx_gpio.c).
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
    if ((EPIC_REG8(PIC_REG_PIR0) & TMR0_PIR0_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE0) & TMR0_PIR0_MASK) != 0U)
        {
            TIMER0_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR4) & TMR1_PIR4_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE4) & TMR1_PIR4_MASK) != 0U)
        {
            TIMER1_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR4) & TMR2_PIR4_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE4) & TMR2_PIR4_MASK) != 0U)
        {
            TIMER2_IRQHandler();
        }
    }
}
