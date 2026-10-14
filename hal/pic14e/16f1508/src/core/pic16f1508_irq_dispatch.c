#include "core/pic16f1508_irq.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_gpio.h"
#include "peripherals/pic16f1508_timer1.h"
#include "peripherals/pic16f1508_timer2.h"

/* PIR3 flags with a driver: CLC1IF..CLC4IF. */
#define CLC_PIR3_MASK ((uint8_t)(PIC_PIR3_CLC1IF | PIC_PIR3_CLC2IF | PIC_PIR3_CLC3IF | PIC_PIR3_CLC4IF))

/* PIR1 flags with a driver: TMR1IF and TMR2IF. */
#define TMR_PIR1_MASK ((uint8_t)(PIC_PIR1_TMR1IF | PIC_PIR1_TMR2IF))

/**
 * @brief Clear one bit of a register using a constant mask for that index.
 * @details A runtime mask makes the clear a load, AND and store of the whole
 * byte, which can erase a flag the hardware sets in that window. Constant masks
 * let XC8 emit one BCF per bit.
 * @param reg Pointer to the register to modify.
 * @param bit Bit index, 0 to 7.
 */
static void epic_clear_bit(volatile uint8_t *reg, uint8_t bit)
{
    switch (bit)
    {
    case 0U: EPIC_BIT_CLR(*reg, 0x01U); break;
    case 1U: EPIC_BIT_CLR(*reg, 0x02U); break;
    case 2U: EPIC_BIT_CLR(*reg, 0x04U); break;
    case 3U: EPIC_BIT_CLR(*reg, 0x08U); break;
    case 4U: EPIC_BIT_CLR(*reg, 0x10U); break;
    case 5U: EPIC_BIT_CLR(*reg, 0x20U); break;
    case 6U: EPIC_BIT_CLR(*reg, 0x40U); break;
    default: EPIC_BIT_CLR(*reg, 0x80U); break;
    }
}

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
        if ((pending & EPIC_BIT(bit)) != 0U)
        {
            epic_clear_bit(pie, bit);
            epic_clear_bit(pir, bit);
        }
    }
}

/**
 * @brief Test whether a CLC source's flag and enable are both still set.
 * @details Evaluated at each call site: a callback that ran earlier in the same
 * dispatch may have disabled a later source.
 * @param mask PIR3 bit of the CLC source.
 * @return Non-zero when the source is pending and enabled, zero otherwise.
 */
static uint8_t clc_live(uint8_t mask)
{
    return (uint8_t)(EPIC_REG8(PIC_REG_PIR3) & EPIC_REG8(PIC_REG_PIE3) & mask);
}

/**
 * @brief Test whether a timer source's flag and enable are both still set.
 * @details Same shape as clc_live: evaluated at each call site, since a
 * callback that ran earlier in the same dispatch may have disabled a later
 * source.
 * @param mask PIR1 bit of the timer source.
 * @return Non-zero when the source is pending and enabled, zero otherwise.
 */
static uint8_t tmr_live(uint8_t mask)
{
    return (uint8_t)(EPIC_REG8(PIC_REG_PIR1) & EPIC_REG8(PIC_REG_PIE1) & mask);
}

/**
 * @brief Route the pending interrupt to its handler.
 * @details Called from the single interrupt vector. A source runs its handler
 * only when its flag and its enable are both set. A disabled source keeps its
 * flag as status for polling through EPIC_IRQ_GetFlag. With PEIE set, enabled
 * PIR1, PIR2 and non-CLC PIR3 sources without a driver are acknowledged
 * without a handler.
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);

    if ((intcon & (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE)) == (PIC_INTCON_IOCIF | PIC_INTCON_IOCIE))
    {
        IOC_IRQHandler();
    }

    /* Read PIR3 and PIE3 per CLC, not once per dispatch: a callback that runs
     * earlier in this dispatch may have disabled a later source. */
    if (clc_live(PIC_PIR3_CLC1IF) != 0U) CLC1_IRQHandler();
    if (clc_live(PIC_PIR3_CLC2IF) != 0U) CLC2_IRQHandler();
    if (clc_live(PIC_PIR3_CLC3IF) != 0U) CLC3_IRQHandler();
    if (clc_live(PIC_PIR3_CLC4IF) != 0U) CLC4_IRQHandler();
    /* PIR1 sources reach the vector only through PEIE, so the timer handlers
     * stay gated on it: without PEIE a pending TMRnIF is left for polling. */
    if ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U)
    {
        if (tmr_live(PIC_PIR1_TMR1IF) != 0U) TIMER1_IRQHandler();
        if (tmr_live(PIC_PIR1_TMR2IF) != 0U) TIMER2_IRQHandler();
    }

    /* Timers 1 and 2 and the CLC cells own their PIR1/PIR3 bits. With PEIE
     * set, any other enabled flag reaches the vector and re-enters it on
     * every return unless acknowledged here. Without PEIE those sources
     * cannot reach the vector. */
    if ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U)
    {
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR1), EPIC_SFR_PTR(PIC_REG_PIE1), (uint8_t)~TMR_PIR1_MASK);
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR2), EPIC_SFR_PTR(PIC_REG_PIE2), 0xFFU);
        epic_ack_unowned(EPIC_SFR_PTR(PIC_REG_PIR3), EPIC_SFR_PTR(PIC_REG_PIE3), (uint8_t)~CLC_PIR3_MASK);
    }
}
