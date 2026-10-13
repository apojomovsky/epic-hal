/**
 * SFR address map for PIC16F1508/1509. PIC_REG_* addresses, CLC included,
 * are generated from the pinned DFP EDC by scripts/gen-sfr.py (identical for
 * both parts; --check detects drift). Bit masks are hand-maintained and cite
 * the DS40001609E register they come from (see MANUAL.md).
 */

#ifndef PIC16F1508_SFR_H
#define PIC16F1508_SFR_H

#include "pic16f1508.h"

/* Core and interrupt control. */
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_OPTION        0x95U
#define PIC_REG_PIR1          0x11U
#define PIC_REG_PIR2          0x12U
#define PIC_REG_PIR3          0x13U
#define PIC_REG_PIE1          0x91U
#define PIC_REG_PIE2          0x92U
#define PIC_REG_PIE3          0x93U

/* GPIO (PORTA..PORTC). */
#define PIC_REG_PORTA         0x0CU
#define PIC_REG_PORTB         0x0DU
#define PIC_REG_PORTC         0x0EU
#define PIC_REG_LATA          0x10CU
#define PIC_REG_LATB          0x10DU
#define PIC_REG_LATC          0x10EU
#define PIC_REG_TRISA         0x8CU
#define PIC_REG_TRISB         0x8DU
#define PIC_REG_TRISC         0x8EU
#define PIC_REG_ANSELA        0x18CU
#define PIC_REG_ANSELB        0x18DU
#define PIC_REG_ANSELC        0x18EU
#define PIC_REG_WPUA          0x20CU
#define PIC_REG_WPUB          0x20DU

/* PORTA and PORTB interrupt-on-change (PORTB is the only port the
 * shared EPIC_GPIO change-detect contract exposes). */
#define PIC_REG_IOCAP         0x391U
#define PIC_REG_IOCAN         0x392U
#define PIC_REG_IOCAF         0x393U
#define PIC_REG_IOCBP         0x394U
#define PIC_REG_IOCBN         0x395U
#define PIC_REG_IOCBF         0x396U

/* CLC: four configurable logic cells. CLCDATA mirrors LCxOUT of all four. */
#define PIC_REG_CLCDATA       0xF0FU
#define PIC_REG_CLC1CON       0xF10U
#define PIC_REG_CLC1POL       0xF11U
#define PIC_REG_CLC1SEL0      0xF12U
#define PIC_REG_CLC1SEL1      0xF13U
#define PIC_REG_CLC1GLS0      0xF14U
#define PIC_REG_CLC1GLS1      0xF15U
#define PIC_REG_CLC1GLS2      0xF16U
#define PIC_REG_CLC1GLS3      0xF17U
#define PIC_REG_CLC2CON       0xF18U
#define PIC_REG_CLC2POL       0xF19U
#define PIC_REG_CLC2SEL0      0xF1AU
#define PIC_REG_CLC2SEL1      0xF1BU
#define PIC_REG_CLC2GLS0      0xF1CU
#define PIC_REG_CLC2GLS1      0xF1DU
#define PIC_REG_CLC2GLS2      0xF1EU
#define PIC_REG_CLC2GLS3      0xF1FU
#define PIC_REG_CLC3CON       0xF20U
#define PIC_REG_CLC3POL       0xF21U
#define PIC_REG_CLC3SEL0      0xF22U
#define PIC_REG_CLC3SEL1      0xF23U
#define PIC_REG_CLC3GLS0      0xF24U
#define PIC_REG_CLC3GLS1      0xF25U
#define PIC_REG_CLC3GLS2      0xF26U
#define PIC_REG_CLC3GLS3      0xF27U
#define PIC_REG_CLC4CON       0xF28U
#define PIC_REG_CLC4POL       0xF29U
#define PIC_REG_CLC4SEL0      0xF2AU
#define PIC_REG_CLC4SEL1      0xF2BU
#define PIC_REG_CLC4GLS0      0xF2CU
#define PIC_REG_CLC4GLS1      0xF2DU
#define PIC_REG_CLC4GLS2      0xF2EU
#define PIC_REG_CLC4GLS3      0xF2FU

/* CLCDATA: MCLCnOUT is the live output of cell n (bit n-1); bits 7:4 read as
 * 0. DS40001609E Register 24-9 (page 271). */
#define PIC_CLCDATA_MCLC1OUT  EPIC_BIT(0)
#define PIC_CLCDATA_MCLC2OUT  EPIC_BIT(1)
#define PIC_CLCDATA_MCLC3OUT  EPIC_BIT(2)
#define PIC_CLCDATA_MCLC4OUT  EPIC_BIT(3)

/* CLCxCON bit positions: DS40001609E Register 24-1 (page 263). */
#define PIC_CLCCON_MODE_MASK  0x07U
#define PIC_CLCCON_INTN       EPIC_BIT(3)
#define PIC_CLCCON_INTP       EPIC_BIT(4)
#define PIC_CLCCON_OUT        EPIC_BIT(5)
#define PIC_CLCCON_OE         EPIC_BIT(6)
#define PIC_CLCCON_EN         EPIC_BIT(7)

/* CLCxPOL: G1POL..G4POL in bits 3:0 (gate inversion), POL in bit 7 (output
 * inversion), bits 6:4 unimplemented. DS40001609E Register 24-2 (page 264). */
#define PIC_CLCPOL_G1POL      EPIC_BIT(0)
#define PIC_CLCPOL_G2POL      EPIC_BIT(1)
#define PIC_CLCPOL_G3POL      EPIC_BIT(2)
#define PIC_CLCPOL_G4POL      EPIC_BIT(3)
#define PIC_CLCPOL_POL        EPIC_BIT(7)

/* CLCxSEL0 and CLCxSEL1: D1S (SEL0) and D3S (SEL1) in bits 2:0, D2S (SEL0)
 * and D4S (SEL1) in bits 6:4, bits 7 and 3 unimplemented. DS40001609E
 * Registers 24-3 and 24-4 (pages 265 and 266). */
#define PIC_CLCSEL_HIGH_SHIFT 4U

/* CLCxGLSn: gate n data input i has its negate bit at 2*(i-1) and its
 * true-enable bit at 2*(i-1)+1. DS40001609E Registers 24-5..24-8 (pages 267
 * to 270). */
#define PIC_CLCGLS_D1N        EPIC_BIT(0)
#define PIC_CLCGLS_D1T        EPIC_BIT(1)
#define PIC_CLCGLS_D2N        EPIC_BIT(2)
#define PIC_CLCGLS_D2T        EPIC_BIT(3)
#define PIC_CLCGLS_D3N        EPIC_BIT(4)
#define PIC_CLCGLS_D3T        EPIC_BIT(5)
#define PIC_CLCGLS_D4N        EPIC_BIT(6)
#define PIC_CLCGLS_D4T        EPIC_BIT(7)

/* INTCON bits (same positions as the 16F1xxx enhanced mid-range core). */
#define PIC_INTCON_IOCIF      EPIC_BIT(0)
#define PIC_INTCON_IOCIE      EPIC_BIT(3)
#define PIC_INTCON_PEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* PIR3 and PIE3 carry the CLC1..CLC4 flag and enable in bits 0..3, bits 7:4
 * unimplemented: DS40001609E Register 7-7 (PIR3, page 81) and Register 7-4
 * (PIE3, page 78). Addresses 0x13 and 0x93: Table 3-9 (page 27). */
#define PIC_PIR3_CLC1IF       EPIC_BIT(0)
#define PIC_PIR3_CLC2IF       EPIC_BIT(1)
#define PIC_PIR3_CLC3IF       EPIC_BIT(2)
#define PIC_PIR3_CLC4IF       EPIC_BIT(3)
#define PIC_PIE3_CLC1IE       EPIC_BIT(0)
#define PIC_PIE3_CLC2IE       EPIC_BIT(1)
#define PIC_PIE3_CLC3IE       EPIC_BIT(2)
#define PIC_PIE3_CLC4IE       EPIC_BIT(3)

/* OPTION_REG bit 7 is nWPUEN: active low, 0 enables the per-pin
 * weak pull-ups in WPUx. */
#define PIC_OPTION_nWPUEN     EPIC_BIT(7)

#endif /* PIC16F1508_SFR_H */
