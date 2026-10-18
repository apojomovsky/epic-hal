#include "core/pic16f188xx_irq.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_ssp.h"
#include "peripherals/pic16f188xx_usart.h"
#include "peripherals/pic16f188xx_eeprom.h"
#include "peripherals/pic16f188xx_nco.h"

/* Flags with a driver: IOCIF and TMR0IF in PIR0, SSP1IF, TXIF and RCIF in
 * PIR3, NVMIF and NCO1IF in PIR7. INT and BCL1IF have no handler and no
 * API enables them, so no unowned source can reach the vector. */
#define IOC_PIR0_MASK ((uint8_t)PIC_PIR0_IOCIF)
#define TMR0_PIR0_MASK ((uint8_t)PIC_PIR0_TMR0IF)
#define SSP1_PIR3_MASK ((uint8_t)PIC_PIR3_SSP1IF)
#define TX_PIR3_MASK ((uint8_t)PIC_PIR3_TXIF)
#define RX_PIR3_MASK ((uint8_t)PIC_PIR3_RCIF)
#define NVM_PIR7_MASK ((uint8_t)PIC_PIR7_NVMIF)
#define NCO1_PIR7_MASK ((uint8_t)PIC_PIR7_NCO1IF)

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
    if ((EPIC_REG8(PIC_REG_PIR3) & SSP1_PIR3_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE3) & SSP1_PIR3_MASK) != 0U)
        {
            SSP_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR3) & TX_PIR3_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE3) & TX_PIR3_MASK) != 0U)
        {
            USART_TX_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR3) & RX_PIR3_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE3) & RX_PIR3_MASK) != 0U)
        {
            USART_RX_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR7) & NVM_PIR7_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE7) & NVM_PIR7_MASK) != 0U)
        {
            EEPROM_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR7) & NCO1_PIR7_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE7) & NCO1_PIR7_MASK) != 0U)
        {
            NCO_IRQHandler();
        }
    }
}
