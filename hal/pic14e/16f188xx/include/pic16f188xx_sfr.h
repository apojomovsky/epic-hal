/**
 * SFR address map for PIC16F18855. PIC_REG_* addresses from the pinned
 * Microchip.PIC16F1xxxx_DFP 1.32.471 EDC (PIC16F18855.PIC); bit positions
 * from the same pack's proc header (pic16f18855.h). DS40001802 is the
 * datasheet; its PDF is bot-walled against scripted fetch, so every
 * address and bit below names the DFP as ground truth instead.
 */

#ifndef PIC16F188XX_SFR_H
#define PIC16F188XX_SFR_H

#include "pic16f188xx.h"

/* Core and interrupt control. INTCON carries only INTEDG, PEIE and GIE
 * (impl 0xC1); the peripheral flags and enables live in PIR0/PIE0. */
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR0          0x70CU
#define PIC_REG_PIE0          0x716U

/* GPIO (PORTA..PORTC). */
#define PIC_REG_PORTA         0x0CU
#define PIC_REG_PORTB         0x0DU
#define PIC_REG_PORTC         0x0EU
#define PIC_REG_TRISA         0x11U
#define PIC_REG_TRISB         0x12U
#define PIC_REG_TRISC         0x13U
#define PIC_REG_LATA          0x16U
#define PIC_REG_LATB          0x17U
#define PIC_REG_LATC          0x18U
#define PIC_REG_ANSELA        0xF38U
#define PIC_REG_WPUA          0xF39U
#define PIC_REG_ODCONA        0xF3AU
#define PIC_REG_SLRCONA       0xF3BU
#define PIC_REG_INLVLA        0xF3CU
#define PIC_REG_IOCAP         0xF3DU
#define PIC_REG_IOCAN         0xF3EU
#define PIC_REG_IOCAF         0xF3FU
#define PIC_REG_ANSELB        0xF43U
#define PIC_REG_WPUB          0xF44U
#define PIC_REG_ODCONB        0xF45U
#define PIC_REG_SLRCONB       0xF46U
#define PIC_REG_INLVLB        0xF47U
#define PIC_REG_IOCBP         0xF48U
#define PIC_REG_IOCBN         0xF49U
#define PIC_REG_IOCBF         0xF4AU
#define PIC_REG_ANSELC        0xF4EU
#define PIC_REG_WPUC          0xF4FU
#define PIC_REG_ODCONC        0xF50U
#define PIC_REG_SLRCONC       0xF51U
#define PIC_REG_INLVLC        0xF52U
#define PIC_REG_IOCCP         0xF53U
#define PIC_REG_IOCCN         0xF54U
#define PIC_REG_IOCCF         0xF55U

/* INTCON bits: INTEDG bit 0, PEIE bit 6, GIE bit 7. */
#define PIC_INTCON_INTEDG       EPIC_BIT(0)
#define PIC_INTCON_PEIE         EPIC_BIT(6)
#define PIC_INTCON_GIE          EPIC_BIT(7)

/* PIR0/PIE0 carry the INT, IOC and TMR0 flags and enables in bits 0, 4
 * and 5; the other bits are unimplemented (impl 0x31). */
#define PIC_PIR0_INTF         EPIC_BIT(0)
#define PIC_PIR0_IOCIF        EPIC_BIT(4)
#define PIC_PIR0_TMR0IF       EPIC_BIT(5)
#define PIC_PIE0_INTE         EPIC_BIT(0)
#define PIC_PIE0_IOCIE        EPIC_BIT(4)
#define PIC_PIE0_TMR0IE       EPIC_BIT(5)

#endif /* PIC16F188XX_SFR_H */
