/* SFR map for the PIC16F83/84/84A family, 1-to-1 from DS35007B (16F84A)
 * and DS30189 (16F83/84), cross-checked against the DFP EDC
 * (Microchip.PIC16Fxxx_DFP, edc/PIC16F84A.PIC; the three parts carry a
 * byte-identical map). Register addresses are regenerated from the EDC
 * by scripts/gen-sfr.py; bit positions and POR values are
 * hand-maintained between the DS and the DFP headers at compile time.
 * Regenerated via scripts/gen-sfr.py. */

#ifndef PIC16F83_84_SFR_H
#define PIC16F83_84_SFR_H

#include "pic16f83_84_hal.h"

/* Bank 0, core SFRs. */

#define PIC_REG_INDF          0x00U
#define PIC_REG_PCL           0x02U
#define PIC_REG_STATUS        0x03U
#define PIC_REG_FSR           0x04U

/* I/O ports: PORTA (RA0..RA4, 5 pins) + PORTB (8 pins), 13 I/O total
 * (DS35007B §1.0). */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U

/* Data EEPROM, data pair in Bank 0 (unlike the 87XA's Banks 2/3 and
 * the 628A's all-Bank-1 block). */
#define PIC_REG_EEDATA        0x08U
#define PIC_REG_EEADR         0x09U

/* Core CPU control, DS35007B §14.0. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU

/* Timer0. */
#define PIC_REG_TMR0          0x01U

/* Bank 1. */
#define PIC_REG_OPTION        0x81U   /* Bank 1. */
#define PIC_REG_TRISA         0x85U   /* Bank 1. */
#define PIC_REG_TRISB         0x86U   /* Bank 1. */
#define PIC_REG_EECON1        0x88U   /* Bank 1. */
#define PIC_REG_EECON2        0x89U   /* Bank 1. */

#define PIC_REG_OPTION        0x81U   /* Bank 1. */
/* Alias under the DFP's full name: the shared drivers' banked macros
 * paste the SFR token (EPIC_BANK1_READ8(OPTION_REG, ...) wants
 * PIC_REG_OPTION_REG on builds where those macros are array forms). */
#define PIC_REG_OPTION_REG    PIC_REG_OPTION

#define PIC_STATUS_C          EPIC_BIT(0)
#define PIC_STATUS_DC         EPIC_BIT(1)
#define PIC_STATUS_Z          EPIC_BIT(2)
#define PIC_STATUS_PD         EPIC_BIT(3)
#define PIC_STATUS_TO         EPIC_BIT(4)
#define PIC_STATUS_RP0        EPIC_BIT(5)
#define PIC_STATUS_RP1        EPIC_BIT(6)
/* No IRP: 8-bit FSR, two data banks only (DS35007B §2.2). */

/* INTCON register bits. No PEIE and no PIR/PIE pair on this family:
 * the EEPROM write-complete interrupt enable EEIE is an INTCON
 * resident (DS35007B §14.11, Register 14-1); its flag EEIF lives in
 * EECON1<4> (below), unlike every bigger 14-bit part. */

#define PIC_INTCON_RBIF       EPIC_BIT(0)
#define PIC_INTCON_INTF       EPIC_BIT(1)
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)
#define PIC_INTCON_RBIE       EPIC_BIT(3)
#define PIC_INTCON_INTE       EPIC_BIT(4)
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)
#define PIC_INTCON_EEIE       EPIC_BIT(6)
#define PIC_INTCON_GIE        EPIC_BIT(7)

/* Reset values (POR), DS35007B §14.0 Table 14-4 / §2.2. */

#define PIC_STATUS_POR_VALUE     0x18U  /* TO and PD set. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_OPTION_POR_VALUE     0xFFU
#define PIC_TRISA_POR_VALUE      0xFFU  /* 1 = input on every pin. */
#define PIC_TRISB_POR_VALUE      0xFFU
#define PIC_EECON1_POR_VALUE     0x00U

/* OPTION_REG bits (Timer0 + WDT prescaler + RBPU/INTEDG). */

#define PIC_OPTION_RBPU         EPIC_BIT(7)
#define PIC_OPTION_INTEDG       EPIC_BIT(6)
#define PIC_OPTION_T0CS         EPIC_BIT(5)
#define PIC_OPTION_T0SE         EPIC_BIT(4)
#define PIC_OPTION_PSA          EPIC_BIT(3)
#define PIC_OPTION_PS_MASK      0x07U

/* EECON1 bits (EEPROM, DS35007B §3.0). EEIF is EECON1<4> on this
 * family: the write-done flag lives here, not in a PIR register. */

#define PIC_EECON1_RD          EPIC_BIT(0)
#define PIC_EECON1_WR          EPIC_BIT(1)
#define PIC_EECON1_WREN        EPIC_BIT(2)
#define PIC_EECON1_WRERR       EPIC_BIT(3)
#define PIC_EECON1_EEIF        EPIC_BIT(4)

/* Bank-selection helper (RP1:RP0 in STATUS). A macro, not a static
 * inline: a call boundary corrupts Bank 1 writes under XC8 v4.00. */
#define pic_select_bank(bank)                                          \
    do {                                                               \
        uint8_t pic_select_bank_status_ = EPIC_REG8(PIC_REG_STATUS);   \
        pic_select_bank_status_ &=                                     \
            (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);               \
        pic_select_bank_status_ |= (uint8_t)(((bank) & 0x03U) << 5);   \
        EPIC_REG8(PIC_REG_STATUS) = pic_select_bank_status_;           \
    } while (0)

#endif /* PIC16F83_84_SFR_H */
