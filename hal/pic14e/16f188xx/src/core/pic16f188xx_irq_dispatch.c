#include "core/pic16f188xx_irq.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_adc.h"
#include "peripherals/pic16f188xx_comp.h"
#include "peripherals/pic16f188xx_ssp.h"
#include "peripherals/pic16f188xx_usart.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"
#include "peripherals/pic16f188xx_timer4.h"
#include "peripherals/pic16f188xx_eeprom.h"
#include "peripherals/pic16f188xx_nco.h"
#include "peripherals/pic16f188xx_ccp.h"

/* Flags with a driver: IOCIF and TMR0IF in PIR0, ADIF in PIR1, C1IF and
 * C2IF in PIR2, SSP1IF, TXIF and RCIF in PIR3, TMR1IF, TMR2IF and TMR4IF
 * in PIR4, CCP1IF..CCP5IF in PIR6, NVMIF and NCO1IF in PIR7. INT, BCL1IF,
 * Timer3, Timer5 and Timer6 have no handler and no API enables them, so
 * no unowned source can reach the vector. */
#define IOC_PIR0_MASK ((uint8_t)PIC_PIR0_IOCIF)
#define TMR0_PIR0_MASK ((uint8_t)PIC_PIR0_TMR0IF)
#define AD_PIR1_MASK ((uint8_t)PIC_PIR1_ADIF)
#define C1_PIR2_MASK ((uint8_t)PIC_PIR2_C1IF)
#define C2_PIR2_MASK ((uint8_t)PIC_PIR2_C2IF)
#define SSP1_PIR3_MASK ((uint8_t)PIC_PIR3_SSP1IF)
#define TX_PIR3_MASK ((uint8_t)PIC_PIR3_TXIF)
#define RX_PIR3_MASK ((uint8_t)PIC_PIR3_RCIF)
#define TMR1_PIR4_MASK ((uint8_t)PIC_PIR4_TMR1IF)
#define TMR2_PIR4_MASK ((uint8_t)PIC_PIR4_TMR2IF)
#define TMR4_PIR4_MASK ((uint8_t)PIC_PIR4_TMR4IF)
#define CCP1_PIR6_MASK ((uint8_t)PIC_PIR6_CCP1IF)
#define CCP2_PIR6_MASK ((uint8_t)PIC_PIR6_CCP2IF)
#define CCP3_PIR6_MASK ((uint8_t)PIC_PIR6_CCP3IF)
#define CCP4_PIR6_MASK ((uint8_t)PIC_PIR6_CCP4IF)
#define CCP5_PIR6_MASK ((uint8_t)PIC_PIR6_CCP5IF)
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
    if ((EPIC_REG8(PIC_REG_PIR1) & AD_PIR1_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE1) & AD_PIR1_MASK) != 0U)
        {
            ADC_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR2) & C1_PIR2_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE2) & C1_PIR2_MASK) != 0U)
        {
            CMP1_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR2) & C2_PIR2_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE2) & C2_PIR2_MASK) != 0U)
        {
            CMP2_IRQHandler();
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
    if ((EPIC_REG8(PIC_REG_PIR4) & TMR4_PIR4_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE4) & TMR4_PIR4_MASK) != 0U)
        {
            TIMER4_IRQHandler();
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
    if ((EPIC_REG8(PIC_REG_PIR6) & CCP1_PIR6_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE6) & CCP1_PIR6_MASK) != 0U)
        {
            CCP1_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR6) & CCP2_PIR6_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE6) & CCP2_PIR6_MASK) != 0U)
        {
            CCP2_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR6) & CCP3_PIR6_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE6) & CCP3_PIR6_MASK) != 0U)
        {
            CCP3_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR6) & CCP4_PIR6_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE6) & CCP4_PIR6_MASK) != 0U)
        {
            CCP4_IRQHandler();
        }
    }
    if ((EPIC_REG8(PIC_REG_PIR6) & CCP5_PIR6_MASK) != 0U)
    {
        if ((EPIC_REG8(PIC_REG_PIE6) & CCP5_PIR6_MASK) != 0U)
        {
            CCP5_IRQHandler();
        }
    }
}
