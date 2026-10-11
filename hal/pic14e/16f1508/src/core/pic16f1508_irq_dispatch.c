#include "core/pic16f1508_irq.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_gpio.h"

/* PIR3 flags with a driver: CLC1IF..CLC4IF. */
#define CLC_PIR3_MASK ((uint8_t)(PIC_PIR3_CLC1IF | PIC_PIR3_CLC2IF | PIC_PIR3_CLC3IF | PIC_PIR3_CLC4IF))

/**
 * @brief Acknowledge the enabled, pending sources of one PIR/PIE pair with no driver.
 * @details Clears each acknowledged source's PIE bit, then its flag, one bit at
 * a time. A whole-byte write of PIR would erase a flag the hardware sets during
 * the handler, since PIR flags clear on a written 0.
 * @param pir Pointer to the PIR register of the pair.
 * @param pie Pointer to the PIE register of the pair.
 * @param unowned Mask of sources in this pair that have no driver.
 */
static void epic_ack_unowned(volatile uint8_t *pir, volatile uint8_t *pie, uint8_t unowned)
{
    uint8_t pending = (uint8_t)(*pir & *pie & unowned);
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        uint8_t mask = EPIC_BIT(bit);

        if ((pending & mask) != 0U)
        {
            EPIC_BIT_CLR(*pie, mask);
            EPIC_BIT_CLR(*pir, mask);
        }
    }
}

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. A source runs its handler
 * only when its flag and its enable are both set. A disabled source keeps its
 * flag as status for polling through EPIC_IRQ_GetFlag. With PEIE set, enabled
 * PIR1, PIR2 and non-CLC PIR3 sources are acknowledged without a handler.
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

    /* No driver owns PIR1, PIR2 or PIR3 bits other than CLC. With PEIE set, an
     * enabled flag reaches the vector and re-enters it on every return unless
     * acknowledged here. Without PEIE those sources cannot reach the vector. */
    if ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U)
    {
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR1), EPIC_SFR_PTR(PIC_REG_PIE1), 0xFFU);
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR2), EPIC_SFR_PTR(PIC_REG_PIE2), 0xFFU);
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR3), EPIC_SFR_PTR(PIC_REG_PIE3), (uint8_t)~CLC_PIR3_MASK);
    }
}
