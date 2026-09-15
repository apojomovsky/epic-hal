/*
 * SFR map, foundation subset: every address/mask/reset 1-to-1 from
 * DS39631E Table 5-1, cross-checked against the DFP header. All in the
 * Access Bank (0xF60-0xFFF), no BSR. No LATD/E or TRISD/E (RE3 only),
 * no USB/SPP (Table 1-1). Timer1-3/CCP/MSSP/EUSART/ADC/EEPROM SFRs
 * land with their drivers in phases 2-4.
 */

#ifndef PIC18F2520_SFR_H
#define PIC18F2520_SFR_H

#include "pic18f2520_hal.h"

/* Access Bank SFR addresses. */
/* DS39631E Table 5-1, SFR Map. All in the Access Bank (0xF60-0xFFF). */

/* Core CPU status, DS39631E Register 5-2. */
#define PIC_REG_STATUS        0xFD8U   /**< ALU status (N/OV/Z/DC/C).       */
#define PIC_REG_BSR           0xFE0U   /**< Bank Select Register (low nibble). */
#define PIC_REG_RCON          0xFD0U   /**< Reset Control (IPEN/TO/PD/POR/BOR). */

/* I/O ports, DS39631E §10.0, Table 10-1..10-3.
 * PORTx is the pin input sample; LATx is the output latch; TRISx is the
 * data-direction register. PIC18 exposes LATx as its own mapped register
 * (§10.0), so GPIO writes go through LATx, not PORTx. The 28-pin 2520
 * has PORTA/B/C (RA0..RA7, RB0..RB7, RC0..RC7) plus a single RE3/MCLR
 * input on PORTE bit 3 (DS39631E Table 1-1, Table 10-3) with no LAT/TRIS
 * register for it; there is no PORTD. */
#define PIC_REG_PORTA         0xF80U
#define PIC_REG_PORTB         0xF81U
#define PIC_REG_PORTC         0xF82U
#define PIC_REG_PORTE         0xF84U   /* RE3/MCLR input only, no LAT/TRIS. */

#define PIC_REG_LATA          0xF89U
#define PIC_REG_LATB          0xF8AU
#define PIC_REG_LATC          0xF8BU

#define PIC_REG_TRISA         0xF92U
#define PIC_REG_TRISB         0xF93U
#define PIC_REG_TRISC         0xF94U

/* Interrupt control, DS39631E §9.0, Register 9-1/9-2/9-3. */
#define PIC_REG_INTCON        0xFF2U   /**< GIE/GIEL/TMR0IE/INT0IE/RBIE + flags. */
#define PIC_REG_INTCON2       0xFF1U   /**< Pull-ups, INT edge, TMR0IP, RBIP.   */
#define PIC_REG_INTCON3       0xFF0U   /**< INT1/INT2 enable, flag, priority.   */

/* Peripheral interrupt flag/enable/priority, DS39631E Register 9-5/9-6/9-8. */
#define PIC_REG_PIR1          0xF9EU   /**< Peripheral Interrupt Flag Register 1. */
#define PIC_REG_PIE1          0xF9DU   /**< Peripheral Interrupt Enable Reg 1.    */
#define PIC_REG_IPR1          0xF9FU   /**< Peripheral Interrupt Priority Reg 1.  */
#define PIC_REG_PIR2          0xFA1U   /**< Peripheral Interrupt Flag Register 2 (TMR3IF, CCP2IF, ...). */
#define PIC_REG_PIE2          0xFA0U   /**< Peripheral Interrupt Enable Reg 2.    */
#define PIC_REG_IPR2          0xFA2U   /**< Peripheral Interrupt Priority Reg 2 (reset 0xFF, all high). */

/* Timer0, DS39631E §11.0, Register 11-1. */
#define PIC_REG_T0CON         0xFD5U   /**< Timer0 control (on/8-16bit/src/edge/PSA/PS). */
#define PIC_REG_TMR0L         0xFD6U   /**< Timer0 low byte (also 8-bit mode value). */
#define PIC_REG_TMR0H         0xFD7U   /**< Timer0 high byte (16-bit mode only).    */

/* STATUS bits (Register 5-2). */
#define PIC_STATUS_N          EPIC_BIT(4)   /**< Negative / borrow complement. */
#define PIC_STATUS_OV         EPIC_BIT(3)   /**< Overflow.                     */
#define PIC_STATUS_Z          EPIC_BIT(2)   /**< Zero.                         */
#define PIC_STATUS_DC         EPIC_BIT(1)   /**< Digit carry/borrow.           */
#define PIC_STATUS_C          EPIC_BIT(0)   /**< Carry/borrow.                 */

/* RCON bits (Register 4-1 / 9-10). */
#define PIC_RCON_IPEN         EPIC_BIT(7)   /**< Interrupt Priority Enable.    */
#define PIC_RCON_SBOREN       EPIC_BIT(6)   /**< BOR software enable.          */
#define PIC_RCON_RI           EPIC_BIT(4)   /**< RESET instruction flag.       */
#define PIC_RCON_TO           EPIC_BIT(3)   /**< WDT time-out flag (1=not).    */
#define PIC_RCON_PD           EPIC_BIT(2)   /**< Power-down (Sleep) flag (1=not). */
#define PIC_RCON_POR          EPIC_BIT(1)   /**< Power-on Reset status.        */
#define PIC_RCON_BOR          EPIC_BIT(0)   /**< Brown-out Reset status.       */

/* INTCON bits (Register 9-1). */
#define PIC_INTCON_GIE        EPIC_BIT(7)   /**< Global Int Enable / high-priority GIE. */
#define PIC_INTCON_GIEH       PIC_INTCON_GIE
#define PIC_INTCON_PEIE       EPIC_BIT(6)   /**< Peripheral Int Enable / low-priority GIEL. */
#define PIC_INTCON_GIEL       PIC_INTCON_PEIE
#define PIC_INTCON_TMR0IE     EPIC_BIT(5)   /**< Timer0 overflow interrupt enable. */
#define PIC_INTCON_INT0IE     EPIC_BIT(4)   /**< INT0 external interrupt enable.   */
#define PIC_INTCON_RBIE       EPIC_BIT(3)   /**< RB<7:4> change interrupt enable.  */
#define PIC_INTCON_TMR0IF     EPIC_BIT(2)   /**< Timer0 overflow interrupt flag.   */
#define PIC_INTCON_INT0IF     EPIC_BIT(1)   /**< INT0 external interrupt flag.     */
#define PIC_INTCON_RBIF       EPIC_BIT(0)   /**< RB<7:4> change interrupt flag.    */

/* INTCON2 bits (Register 9-2). */
#define PIC_INTCON2_RBPU      EPIC_BIT(7)   /**< PORTB pull-up enable (active-low). */
#define PIC_INTCON2_INTEDG0   EPIC_BIT(6)   /**< INT0 edge select (1=rising).       */
#define PIC_INTCON2_INTEDG1   EPIC_BIT(5)   /**< INT1 edge select.                  */
#define PIC_INTCON2_INTEDG2   EPIC_BIT(4)   /**< INT2 edge select.                  */
#define PIC_INTCON2_TMR0IP    EPIC_BIT(2)   /**< Timer0 overflow priority (1=high). */
#define PIC_INTCON2_RBIP      EPIC_BIT(0)   /**< RB change priority (1=high).       */

/* INTCON3 bits (Register 9-3). */
#define PIC_INTCON3_INT2IP    EPIC_BIT(7)   /**< INT2 priority (1=high).            */
#define PIC_INTCON3_INT1IP    EPIC_BIT(6)   /**< INT1 priority (1=high).            */
#define PIC_INTCON3_INT2IE    EPIC_BIT(4)   /**< INT2 external interrupt enable.    */
#define PIC_INTCON3_INT1IE    EPIC_BIT(3)   /**< INT1 external interrupt enable.    */
#define PIC_INTCON3_INT2IF    EPIC_BIT(1)   /**< INT2 external interrupt flag.      */
#define PIC_INTCON3_INT1IF    EPIC_BIT(0)   /**< INT1 external interrupt flag.      */

/* PIR1 / PIE1 / IPR1 bits (Reg 9-5/9-6/9-8). */
/* Same bit layout across the three registers: flag / enable / priority.
 * PIR1 has no SPPIF on this part (no SPP); ADIF is the top bit. */
#define PIC_PIR1_ADIF         EPIC_BIT(6)   /**< A/D conversion complete flag.  */
#define PIC_PIR1_RCIF         EPIC_BIT(5)   /**< EUSART receive flag.           */
#define PIC_PIR1_TXIF         EPIC_BIT(4)   /**< EUSART transmit flag.          */
#define PIC_PIR1_SSPIF        EPIC_BIT(3)   /**< MSSP flag.                     */
#define PIC_PIR1_CCP1IF       EPIC_BIT(2)   /**< CCP1 flag.                     */
#define PIC_PIR1_TMR2IF       EPIC_BIT(1)   /**< Timer2 match flag.             */
#define PIC_PIR1_TMR1IF       EPIC_BIT(0)   /**< Timer1 overflow flag.          */
#define PIC_PIE1_ADIE         EPIC_BIT(6)
#define PIC_PIE1_RCIE         EPIC_BIT(5)
#define PIC_PIE1_TXIE         EPIC_BIT(4)
#define PIC_PIE1_SSPIE        EPIC_BIT(3)
#define PIC_PIE1_CCP1IE       EPIC_BIT(2)
#define PIC_PIE1_TMR2IE       EPIC_BIT(1)
#define PIC_PIE1_TMR1IE       EPIC_BIT(0)
#define PIC_IPR1_ADIP         EPIC_BIT(6)
#define PIC_IPR1_RCIP         EPIC_BIT(5)
#define PIC_IPR1_TXIP         EPIC_BIT(4)
#define PIC_IPR1_SSPIP        EPIC_BIT(3)
#define PIC_IPR1_CCP1IP       EPIC_BIT(2)
#define PIC_IPR1_TMR2IP       EPIC_BIT(1)
#define PIC_IPR1_TMR1IP       EPIC_BIT(0)

/* T0CON bits (Register 11-1). */
#define PIC_T0CON_TMR0ON      EPIC_BIT(7)   /**< Timer0 on/off.                  */
#define PIC_T0CON_T08BIT      EPIC_BIT(6)   /**< 1=8-bit, 0=16-bit mode.         */
#define PIC_T0CON_T0CS        EPIC_BIT(5)   /**< 0=internal Fosc/4, 1=T0CKI pin. */
#define PIC_T0CON_T0SE        EPIC_BIT(4)   /**< Counter mode edge select.       */
#define PIC_T0CON_PSA         EPIC_BIT(3)   /**< 0=prescaler assigned, 1=not.    */
#define PIC_T0CON_T0PS_MASK   0x07U         /**< T0PS2:T0PS0, prescaler ratio.   */

/* PIR2 / PIE2 / IPR2 bits (Reg 9-5/9-7/9-9). */
/* Same bit layout across the three registers: flag / enable / priority.
 * PIR2 has no USBIF on this part (no USB); OSCFIF is the top bit. */
#define PIC_PIR2_OSCFIF       EPIC_BIT(7)   /**< Oscillator fail flag.            */
#define PIC_PIR2_CMIF         EPIC_BIT(6)   /**< Comparator flag.                 */
#define PIC_PIR2_EEIF         EPIC_BIT(4)   /**< EEPROM write done flag.          */
#define PIC_PIR2_BCLIF        EPIC_BIT(3)   /**< MSSP bus collision flag.         */
#define PIC_PIR2_HLVDIF       EPIC_BIT(2)   /**< High/Low-voltage detect flag.    */
#define PIC_PIR2_TMR3IF       EPIC_BIT(1)   /**< Timer3 overflow flag.            */
#define PIC_PIR2_CCP2IF       EPIC_BIT(0)   /**< CCP2 flag.                       */
#define PIC_PIE2_OSCFIE       EPIC_BIT(7)
#define PIC_PIE2_CMIE         EPIC_BIT(6)
#define PIC_PIE2_EEIE         EPIC_BIT(4)
#define PIC_PIE2_BCLIE        EPIC_BIT(3)
#define PIC_PIE2_HLVDIE       EPIC_BIT(2)
#define PIC_PIE2_TMR3IE       EPIC_BIT(1)
#define PIC_PIE2_CCP2IE       EPIC_BIT(0)
#define PIC_IPR2_OSCFIP       EPIC_BIT(7)
#define PIC_IPR2_CMIP         EPIC_BIT(6)
#define PIC_IPR2_EEIP         EPIC_BIT(4)
#define PIC_IPR2_BCLIP        EPIC_BIT(3)
#define PIC_IPR2_HLVDIP       EPIC_BIT(2)
#define PIC_IPR2_TMR3IP       EPIC_BIT(1)
#define PIC_IPR2_CCP2IP       EPIC_BIT(0)

/* Reset values (POR), DS39631E Table 5-1 + Register 4-1 notes. RCON
 * POR value 0x57 (IPEN=0, SBOREN=1, TO/PD/POR/BOR set; the sim uses this
 * image). IPR1/IPR2 default high. T0CON to 0xFF; INTCON2 to 0xFB;
 * INTCON3 to 0xC0 (INT1/2 priority). */
#define PIC_STATUS_POR_VALUE     0x00U
#define PIC_BSR_POR_VALUE        0x00U
#define PIC_RCON_POR_VALUE       0x57U
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_INTCON2_POR_VALUE    0xFBU
#define PIC_INTCON3_POR_VALUE    0xC0U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_IPR1_POR_VALUE       0xFFU
#define PIC_T0CON_POR_VALUE      0xFFU
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_IPR2_POR_VALUE       0xFFU
#define PIC_TRIS_POR_VALUE       0xFFU   /* All pins inputs after POR. */
#define PIC_LAT_POR_VALUE        0x00U   /* Output latches clear after POR. */
#define PIC_PORT_POR_VALUE       0x00U

#endif /* PIC18F2520_SFR_H */
