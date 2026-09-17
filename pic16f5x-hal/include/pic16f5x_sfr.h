/* SFR map for the PIC16F5x family, 1-to-1 from DS41213D, cross-checked
 * against the DFP EDC (Microchip.PIC16Fxxx_DFP, edc/PIC16F54.PIC; the
 * five parts share the core block 0x00-0x06, per-part ports extend it).
 * Register addresses are regenerated from the EDC by scripts/gen-sfr.py;
 * bit positions and POR values are hand-maintained between the DS and
 * the DFP headers at compile time.
 * Regenerated via scripts/gen-sfr.py. */

#ifndef PIC16F5X_SFR_H
#define PIC16F5X_SFR_H

#include "pic16f5x_hal.h"

/* Core file registers (every part, DFP-verified addresses). */

#define PIC_REG_INDF          0x00U
#define PIC_REG_TMR0          0x01U
#define PIC_REG_PCL           0x02U
#define PIC_REG_STATUS        0x03U
#define PIC_REG_FSR           0x04U

/* I/O ports by part. PORTA (RA0..RA3, 4 output-capable pins) exists
 * on the 18/28-pin parts (16F54/57/59); PORTB is full 8-bit on every
 * part; PORTC on the 28-pin parts; PORTD/E on the 40-pin 16F59.
 * DS41213D §2.0. */
#if PIC16F5X_FAMILY_HAS_PORTA
#define PIC_REG_PORTA         0x05U
#endif
#define PIC_REG_PORTB         0x06U
#if PIC16F5X_FAMILY_HAS_PORTC
#define PIC_REG_PORTC         0x07U
#endif
#if PIC16F5X_FAMILY_HAS_PORTD
#define PIC_REG_PORTD         0x08U
#endif
#if PIC16F5X_FAMILY_HAS_PORTE
#define PIC_REG_PORTE         0x09U
#endif

/* 14-pin parts: OSCCAL takes file 0x05 (there is no PORTA). */
#if PIC16F5X_FAMILY_HAS_OSCCAL
#define PIC_REG_OSCCAL        0x05U
#endif

/* 16F506 comparator and ADC files (DS41268D Figure 4-3 register map,
 * Table 4-2 reset values; addresses cross-checked against the DFP
 * pic16f506.h). Only this part carries them, and both comparators plus
 * the ADC analog selects come out of reset enabled, which takes their
 * shared pins out of digital I/O (§9.1.2, §7.7): the GPIO driver
 * clears this block before configuring a digital pin. */
#if PIC16F5X_FAMILY_HAS_COMP_ADC
#define PIC_REG_CM1CON0       0x08U
#define PIC_REG_ADCON0        0x09U
#define PIC_REG_ADRES         0x0AU
#define PIC_REG_CM2CON0       0x0BU
#define PIC_REG_VRCON         0x0CU

/* CM1CON0/CM2CON0 (DS41268D Register 7-2/7-3): CxOUT is read-only,
 * CxON enables the comparator. */
#define PIC_CM1CON0_C1ON      EPIC_BIT(3)
#define PIC_CM2CON0_C2ON      EPIC_BIT(3)

/* ADCON0 (DS41268D Register 9-1): ANS<1:0> selects the analog input
 * pins (11 = AN2/AN1/AN0, the POR value) and stays in effect
 * regardless of ADON. */
#define PIC_ADCON0_ANS_MASK   0xC0U
#define PIC_ADCON0_ADON       EPIC_BIT(0)
#endif

/* STATUS register bits (DS41213D §3.0, Register 3-1). PA<2:0> is the
 * program-page select (PC<10:8>), not a bank select; the 12-bit core
 * banks its data through FSR<6:5>, never through STATUS. */
#define PIC_STATUS_C          EPIC_BIT(0)
#define PIC_STATUS_DC         EPIC_BIT(1)
#define PIC_STATUS_Z          EPIC_BIT(2)
#define PIC_STATUS_nPD        EPIC_BIT(3)
#define PIC_STATUS_nTO        EPIC_BIT(4)
#define PIC_STATUS_PA0        EPIC_BIT(5)
#define PIC_STATUS_PA1        EPIC_BIT(6)
#define PIC_STATUS_PA2        EPIC_BIT(7)

/* Reset values (POR), DS41213D §3.0 Table 3-1 / §2.2. */
#define PIC_STATUS_POR_VALUE      0x18U  /* nPD and nTO set; PA=0. */

/* TRIS and OPTION are control-space on this core: they are written
 * with the dedicated `tris <f>` / `option` instructions, not through
 * the file register map (DS41213D §12.0). They carry no PIC_REG_*
 * address and are accessed via the platform control helpers
 * (EPIC_TRIS_WRITE / EPIC_OPTION_WRITE, see the platform header). The
 * data direction convention is identical to classic PIC16: TRIS bit 1
 * = input, 0 = output. */

/* OPTION register bits (DS41213D §9.0, Register 9-1): Timer0 clock
 * source/edge, the shared prescaler assignment and ratio. There is no
 * RBPU (weak pull-ups) and no INTEDG on this core: RB0 has no
 * interrupt, and pull-ups do not exist on the baseline die. */
#define PIC_OPTION_PS_MASK    0x07U  /* PS<2:0>, prescaler ratio.      */
#define PIC_OPTION_PSA        EPIC_BIT(3)  /* 1 = prescaler to WDT.    */
#define PIC_OPTION_T0SE       EPIC_BIT(4)  /* 1 = T0CKI falling edge.  */
#define PIC_OPTION_T0CS       EPIC_BIT(5)  /* 1 = T0CKI external.      */

#define PIC_OPTION_POR_VALUE      0x3FU  /* PS=7, PSA=1, T0SE=1, T0CS=1;
                                          * bits 6-7 unimplemented (read 0,
                                          * EDC por="--111111"). */

/* FSR file-register access: INDF addresses `FSR` as a flat byte on the
 * baseline core, with the data-bank bits (FSR<6:5> on 16F57/505/506,
 * FSR<7:5> on 16F59, none on 16F54) selecting the banked GPR window
 * (DS41213D §3.6, Figure 3-3). */
#define PIC_FSR_INDIRECT_MASK 0x7FU  /* 7-bit offset+bank on all 5x parts. */

#endif /* PIC16F5X_SFR_H */
