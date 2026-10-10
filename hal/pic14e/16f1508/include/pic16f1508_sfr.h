/**
 * Special Function Register (SFR) address map for the PIC16F1508/1509.
 *
 * The `PIC_REG_*` addresses are generated from the pinned DFP EDC
 * (scripts/gen-sfr.py); the EDC is identical for 16F1508 and 16F1509 for
 * every register listed here. Bit masks below are hand-maintained and
 * cite the register name, not a datasheet page number, until the
 * datasheet is checked against them (see MANUAL.md).
 */

#ifndef PIC16F1508_SFR_H
#define PIC16F1508_SFR_H

#include "pic16f1508.h"

/* Core and interrupt control. */
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_OPTION        0x95U

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

/* INTCON bits (same positions as the 16F1xxx enhanced mid-range core). */
#define PIC_INTCON_IOCIF      EPIC_BIT(0)
#define PIC_INTCON_IOCIE      EPIC_BIT(3)
#define PIC_INTCON_PEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* OPTION_REG bit 7 is nWPUEN: active low, 0 enables the per-pin
 * weak pull-ups in WPUx. */
#define PIC_OPTION_nWPUEN     EPIC_BIT(7)

#endif /* PIC16F1508_SFR_H */
