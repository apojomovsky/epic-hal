# PIC16F188xx register reference

Per-peripheral register reference for the GPIO, interrupt-on-change,
Timer0..Timer2, CCP1..CCP5, PWM6/PWM7, EUSART, MSSP1, PPS, data EEPROM, NCO1 and
WDT/Sleep sources in
this HAL. Shared conventions (naming, handle pattern, harness, interrupt
model) are in
[common/MANUAL.md](../../../common/MANUAL.md). This file covers only
what is specific to this family: 16F18854/55/56/57 (28-pin) and
16F18875/76/77 (40-pin, adding PORTD and PORTE).

## Sources and verification

- Addresses, implemented-bit masks, POR values and bit positions come
  from the Microchip.PIC16F1xxxx_DFP 1.32.471 EDC (`PIC16F18855.PIC`)
  and proc header (`pic16f18855.h`), pinned by the manifest.
  `scripts/build/gen-sfr.py` rewrites each `PIC_REG_*` address the EDC
  defines. Do not hand-edit addresses. The bit masks are
  hand-maintained and cross-checked against the DFP proc header.
- Datasheet: DS40001802 (PIC16F18854/55/56/57/75/76/77). Its PDF is
  bot-walled against plain scripted fetch, so only the cites that name
  a checked page or register below were read against it. A value or
  cite without one is still DFP-derived and **unverified** against the
  datasheet.
- Timer0 encodings (clock sources, prescaler, postscaler, T0OUT) cite
  DS40001825F §27, Registers 27-1/27-2 instead: the sibling 18857/77
  datasheet documents this exact Timer0 module (EDC _modsrc
  DOS-01631_tmr_mid0_v1_8bit on both), read as text, not from a
  bot-walled PDF.
- Timer1 and Timer2 encodings (clock selects, gate sources, T2HLT
  modes, reset sources) and the per-pin PPS defaults cite DS40001802H,
  the family datasheet, §28 (Registers 28-1..28-4), §29 (Registers
  29-1..29-4) and Table 13-1. That revision downloads with a browser
  user agent, was read as text, and its Timer1/2 register bit tables
  match DS40001825F's (the 18857/77 parts share the modules).
- The data EEPROM (NVM), WDT, Sleep and Idle, PCON0 and NCO1 sections
  cite DS40001802D (PIC16F18855/75), read as text from a copy fetched
  with a browser user agent. The other parts are not in that document;
  their EDCs carry the same NCO and windowed-WDT module (_modsrc
  DOS-01444B_NCO_v1_a and DOS-02448_wdt_windowed_v1 on all seven) and
  the same register addresses, which the sfr-map audit checks.
- CCP and PWM encodings (modes, capture sources, timer couples, FMT
  alignment, PWMxCON and the duty layout) cite DS40001802D §30 and §19,
  Registers 19-1..19-3 and 30-1..30-6, and Table 13-3 for the output
  codes. That revision (the datasheet file in hand, page footers read
  DS40001802D) was read as text; its CCP tables were checked against the
  EDC impl and POR values.
- ADC, DAC1, FVR and comparator encodings (channels, references, gains,
  triggers, PPS output codes) cite DS40001802D §23 (Registers 23-1..23-8,
  Table 23-2), §25 (Registers 25-1/25-2), §16 (Register 16-1), §18
  (Registers 18-1..18-5) and Table 13-3, read as text from the same copy.
  ADCON0 bit 3 and CMxCON0 bit 2 are unimplemented (read 0) on the
  Register 23-1/18-1 diagrams but implemented in the EDC (ADCON0 impl
  0xDD, CMxCON0 impl 0xD7 with POR 1 on bit 2); the driver leaves both
  at 0, the MCC norm, and names neither bit.

## Implemented bits (from EDC impl masks)

| Register | Impl  | POR        | Meaning                                  |
|----------|-------|------------|------------------------------------------|
| PORTA    | 0xFF  | xxxxxxxx   | RA0..RA7                                 |
| LATA     | 0xFF  | xxxxxxxx   | Latch follows the PORTA pins             |
| TRISA    | 0xFF  | 0xFF       | Direction for RA0..RA7, all input        |
| ANSELA   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUA     | 0xFF  | 0x00       | Weak pull-up per PORTA pin               |
| IOCAP, IOCAN, IOCAF | 0xFF | 0x00 | Rising, falling, flag per PORTA pin |
| PORTB    | 0xFF  | xxxxxxxx   | RB0..RB7                                 |
| LATB     | 0xFF  | xxxxxxxx   | Latch follows the PORTB pins             |
| TRISB    | 0xFF  | 0xFF       | Direction for RB0..RB7, all input        |
| ANSELB   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUB     | 0xFF  | 0x00       | Weak pull-up per PORTB pin               |
| IOCBP, IOCBN, IOCBF | 0xFF | 0x00 | Rising, falling, flag per PORTB pin |
| PORTC    | 0xC0  | xxxxxxxx   | RC0..RC7 read mask; all eight RC fields are defined and TRISC/LATC report 0xFF, so the driver mask stays 0xFF |
| LATC     | 0xFF  | xxxxxxxx   | Latch follows the PORTC pins             |
| TRISC    | 0xFF  | 0xFF       | Direction for RC0..RC7, all input        |
| ANSELC   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUC     | 0xFF  | 0x00       | Weak pull-up per PORTC pin               |
| PORTD    | 0xFF  | xxxxxxxx   | RD0..RD7, 40-pin parts only              |
| LATD     | 0xFF  | xxxxxxxx   | Latch follows the PORTD pins             |
| TRISD    | 0xFF  | 0xFF       | Direction for RD0..RD7, all input        |
| ANSELD   | 0xFF  | 0xFF       | Analog select, all analog                |
| WPUD     | 0xFF  | 0x00       | Weak pull-up per PORTD pin               |
| PORTE    | 0x0F (0x08 on 28-pin, RE3 only) | ----xxxx | RE0..RE3 read; RE3 is input-only (MCLR), 40-pin parts add RE0..RE2 |
| LATE     | 0x07  | -----xxx  | Latch for RE0..RE2, 40-pin parts only    |
| TRISE    | 0x07  | -----111  | Direction for RE0..RE2, 40-pin parts only|
| ANSELE   | 0x07  | -----111  | Analog select RE0..RE2, 40-pin parts only|
| WPUE     | 0x0F (0x08 on 28-pin) | ----x000 | Weak pull-up RE0..RE2 (RE3 bit exists) |
| IOCEP, IOCEN, IOCEF | 0x08 | ----0--- | Rising, falling, flag for RE3 only |
| INTCON   | 0xC1  | 00-----1   | INTEDG, PEIE, GIE only                   |
| PIR0     | 0x31  | --00---0   | INTF, IOCIF, TMR0IF                      |
| PIE0     | 0x31  | --00---0   | INTE, IOCIE, TMR0IE                      |
| TMR0L    | 0xFF  | 00000000   | Counter low byte                         |
| TMR0H    | 0xFF  | 11111111   | Counter high byte, match value in 8-bit mode |
| T0CON0   | 0xBF  | 0-000000   | T0EN, T0OUT (read-only), T016BIT, T0OUTPS |
| T0CON1   | 0xFF  | 00000000   | T0CS, T0ASYNC, T0CKPS                    |
| TMR1L    | 0xFF  | xxxxxxxx   | Counter low byte; a read latches TMR1H while RD16 is set |
| TMR1H    | 0xFF  | xxxxxxxx   | Counter high byte (buffered while RD16 is set) |
| T1CON    | 0x37  | --00-000   | ON, RD16, SYNC (set: not synchronized), CKPS |
| T1GCON   | 0xFC  | 00000x--   | GE, GPOL, GTM, GSPM, GGO/DONE (hardware-cleared), GVAL (read-only) |
| T1GATE   | 0x1F  | ---00000   | GSS gate source select                   |
| T1CLK    | 0x0F  | ----0000   | CS clock source select                   |
| TMR2     | 0xFF  | 00000000   | Counter                                  |
| PR2      | 0xFF  | 11111111   | Period (EDC name T2PR)                   |
| T2CON    | 0xFF  | 00000000   | ON, CKPS (3 bits), OUTPS                 |
| T2HLT    | 0xFF  | 00000000   | PSYNC, CKPOL, CKSYNC, MODE (5 bits)      |
| T2CLKCON | 0x0F  | ----0000   | CS clock source select                   |
| T2RST    | 0x1F  | ---00000   | RSEL external reset signal select        |
| PIR4     | 0x3F  | --000000   | TMR1IF bit 0, TMR2IF bit 1 used (bits 2..5 are Timer3..6) |
| PIE4     | 0x3F  | --000000   | TMR1IE bit 0, TMR2IE bit 1 used          |
| PIR3     | 0x3F  | --000000   | SSP1IF, BCL1IF, SSP2IF, BCL2IF, TXIF (read-only), RCIF (read-only) |
| PIE3     | 0x3F  | --000000   | SSP1IE, BCL1IE, SSP2IE, BCL2IE, TXIE, RCIE |
| PIR7     | 0xF7  | 0000-000   | NCO1IF (bit 4), NVMIF (bit 5); the rest are CWG, CRC and scanner |
| PIE7     | 0xF7  | 0000-000   | NCO1IE (bit 4), NVMIE (bit 5)            |
| NVMADRL  | 0xFF  | 00000000   | NVM address low byte                     |
| NVMADRH  | 0x7F  | 10000000   | NVM address high byte; bit 7 is unimplemented and reads 1 |
| NVMDATL  | 0xFF  | 00000000   | NVM data low byte (the EEPROM byte)      |
| NVMDATH  | 0x3F  | --000000   | NVM data high byte, program Flash only   |
| NVMCON1  | 0x7F  | -0000000   | NVMREGS, LWLO, FREE, WRERR, WREN, WR, RD |
| NVMCON2  | write-only | 00000000 | Unlock pattern register, reads 0       |
| WDTCON0  | 0x3F  | --qqqqq0   | WDTPS<4:0>, SWDTEN                       |
| WDTCON1  | 0x77  | -qqq-qqq   | WDTCS<2:0>, WINDOW<2:0>                  |
| PCON0    | 0xFF  | 0011110x   | STKOVF, STKUNF, WDTWV, RWDT, RMCLR, RI, POR, BOR |
| CPUDOZE  | 0xF7  | 0000-000   | IDLEN, DOZEN, ROI, DOE, DOZE<2:0>        |
| NCO1ACCL, NCO1ACCH | 0xFF | 00000000 | Accumulator bits 7:0 and 15:8     |
| NCO1ACCU | 0x0F  | ----0000   | Accumulator bits 19:16                   |
| NCO1INCL | 0xFF  | 00000001   | Increment bits 7:0                       |
| NCO1INCH | 0xFF  | 00000000   | Increment bits 15:8                      |
| NCO1INCU | 0x0F  | ----0000   | Increment bits 19:16                     |
| NCO1CON  | 0xB1  | 0-00---0   | N1EN, N1OUT (read-only), N1POL, N1PFM    |
| NCO1CLK  | 0xE7  | 000--000   | N1PWS<2:0>, N1CKS<2:0>                   |
| RC1REG   | 0xFF  | 00000000   | Receive FIFO head (read-only), a read pops it |
| TX1REG   | 0xFF  | 00000000   | Transmit buffer, a write clears TXIF     |
| SP1BRGL, SP1BRGH | 0xFF | 00000000 | Baud generator low and high byte  |
| RC1STA   | 0xFF  | 00000000   | SPEN, RX9, SREN, CREN, ADDEN, FERR/OERR/RX9D (read-only) |
| TX1STA   | 0xFF  | 00000010   | CSRC, TX9, TXEN, SYNC, SENDB, BRGH, TRMT (read-only), TX9D |
| BAUD1CON | 0xDB  | 01-00-00   | ABDOVF/RCIDL (read-only), SCKP, BRG16, WUE, ABDEN |
| SSP1BUF  | 0xFF  | xxxxxxxx   | Receive buffer (read), transmit register (write) |
| SSP1ADD  | 0xFF  | 00000000   | Baud divisor for the two Fosc/(4*(n+1)) modes |
| SSP1STAT | 0xFF  | 00000000   | SMP, CKE writable; DA, P, S, RW, UA, BF status |
| SSP1CON1 | 0xFF  | 00000000   | WCOL, SSPOV (clear-only), SSPEN, CKP, SSPM |
| SSP1CON2 | 0xFF  | 00000000   | GCEN, ACKSTAT (read-only), ACKDT, ACKEN, RCEN, PEN, RSEN, SEN |
| PPSLOCK  | 0x01  | -------0   | PPSLOCKED                                |
| xxxPPS   | 0x1F (T0CKIPPS 0x0F) | per input | Input pin select: port in bits 4:3, pin in 2:0 |
| RxyPPS   | 0x3F  | --000000   | Output source select, 0 = port latch     |
| PIR6     | 0x1F  | ---00000   | CCP1IF..CCP5IF                           |
| PIE6     | 0x1F  | ---00000   | CCP1IE..CCP5IE                           |
| CCPxCON  | 0xBF  | 0-000000   | EN, OUT (read-only), FMT, MODE (4 bits)  |
| CCPxCAP  | 0x07  | -----000   | CTS capture input select                 |
| CCPRxL, CCPRxH | 0xFF | xxxxxxxx | Capture, compare or PWM duty byte    |
| CCPTMRS0 | 0xFF  | 01010101   | C1TSEL..C4TSEL timer couple per CCP      |
| CCPTMRS1 | 0x3F  | --010101   | C5TSEL, P6TSEL, P7TSEL                   |
| PWMxCON  | 0xB0  | 0-00----   | EN, OUT (read-only), POL                 |
| PWMxDCL  | 0xC0  | xx------   | Duty bits 1:0 in bits 7:6                |
| PWMxDCH  | 0xFF  | xxxxxxxx   | Duty bits 9:2                            |
| CCPxPPS  | 0x1F  | per input  | Capture pin select, port in bits 4:3 (CCP5PPS 0x3F on 16F18877) |
| ADRESL, ADRESH | 0xFF | 00000000 | Conversion result low and high byte |
| ADCON0   | 0xDD  | 00-000-0   | ADON, ADCONT, ADCS (FRC), ADFRM0, ADGO (hardware-cleared); bit 3 EDC-only, unnamed |
| ADCON1   | 0xE1  | 000----0   | ADPPOL, ADIPEN, ADGPOL, ADDSEN: basic mode, all clear |
| ADCON2   | 0xFF  | 00000000   | ADPSIS, ADCRS, ADACLR, ADMD: basic mode, all clear |
| ADCLK    | 0x3F  | --000000   | ADCCS FOSC divider (FOSC/(2*(n+1))) |
| ADACT    | 0x1F  | ---00000   | Auto-conversion trigger select (Table 23-2) |
| ADREF    | 0x13  | ---0--00   | ADNREF (VREF- pin), ADPREF (VDD, VREF+ pin, FVR) |
| ADPCH    | 0x3F  | --000000   | Channel: pins to 0x17 (0x22 on 40-pin), 0x3C..0x3F internal |
| PIR1     | 0xC3  | 00----00   | ADIF bit 0, ADTIF bit 1 (threshold, no driver) |
| PIE1     | 0xC3  | 00----00   | ADIE bit 0, ADTIE bit 1 (no driver) |
| PIR2     | 0x43  | -0----00   | C1IF bit 0, C2IF bit 1 |
| PIE2     | 0x43  | -0----00   | C1IE bit 0, C2IE bit 1 |
| FVRCON   | 0xFF  | 0xxxxxxx   | FVREN, FVRRDY (read-only, always 1 on 18855/75 once on), TSEN/TSRNG (off), CDAFVR, ADFVR |
| DAC1CON0 | 0x1C (EDC; EN, OE1, OE2, NSS per Register 25-1 and the proc header) | 0-0000-0 | DAC1EN, DAC1OE1 (RA2), DAC1OE2 (RB7), DAC1PSS, DAC1NSS |
| DAC1CON1 | 0x1F  | ---xxxxx   | DAC1R 5-bit code |
| CMOUT    | 0x03  | ------00   | MC2OUT, MC1OUT mirrors (read-only) |
| CMxCON0  | 0xD7  | 00-0-100   | ON, OUT (read-only), POL, HYS, SYNC; bit 2 EDC-only, unnamed |
| CMxCON1  | 0x03  | ------00   | INTP, INTN edge enables |
| CMxNSEL  | 0x07  | -----000   | NCH: 0..3 pins, 6 FVR buffer 2, 7 AVSS |
| CMxPSEL  | 0x07  | -----000   | PCH: 0..1 pins, 5 DAC, 6 FVR buffer 2, 7 AVSS |

## Register addresses

Addresses are the `PIC_REG_*` values in `include/pic16f188xx_sfr.h`
(generated). The table below only lists the ones this HAL uses, in the
canonical 0xFxx spelling: on 16F18857/16F18877 every banked address at
or above 0xF00 reads 0x1Fxx instead (same low 12 bits; the header
selects the spelling per part, and the sfr-map audit checks both).

| Name     | Address |
|----------|---------|
| INTCON   | 0x0B    |
| PORTA    | 0x0C    |
| PORTB    | 0x0D    |
| PORTC    | 0x0E    |
| TRISA    | 0x11    |
| TRISB    | 0x12    |
| TRISC    | 0x13    |
| LATA     | 0x16    |
| LATB     | 0x17    |
| LATC     | 0x18    |
| PIR0     | 0x70C   |
| PIE0     | 0x716   |
| ANSELA   | 0xF38   |
| WPUA     | 0xF39   |
| IOCAP    | 0xF3D   |
| IOCAN    | 0xF3E   |
| IOCAF    | 0xF3F   |
| ANSELB   | 0xF43   |
| WPUB     | 0xF44   |
| IOCBP    | 0xF48   |
| IOCBN    | 0xF49   |
| IOCBF    | 0xF4A   |
| ANSELC   | 0xF4E   |
| WPUC     | 0xF4F   |
| IOCCP    | 0xF53   |
| IOCCN    | 0xF54   |
| PORTD    | 0x0F    |
| TRISD    | 0x14    |
| LATD     | 0x19    |
| PORTE    | 0x10    |
| TRISE    | 0x15    |
| LATE     | 0x1A    |
| TMR0L    | 0x1C    |
| TMR0H    | 0x1D    |
| T0CON0   | 0x1E    |
| T0CON1   | 0x1F    |
| TMR1L    | 0x20C   |
| TMR1H    | 0x20D   |
| T1CON    | 0x20E   |
| T1GCON   | 0x20F   |
| T1GATE   | 0x210   |
| T1CLK    | 0x211   |
| TMR2     | 0x28C   |
| PR2      | 0x28D   |
| T2CON    | 0x28E   |
| T2HLT    | 0x28F   |
| T2CLKCON | 0x290   |
| T2RST    | 0x291   |
| PIR4     | 0x710   |
| PIE4     | 0x71A   |
| ANSELD   | 0xF59   |
| WPUD     | 0xF5A   |
| ANSELE   | 0xF64   |
| WPUE     | 0xF65   |
| PIR3     | 0x70F   |
| PIE3     | 0x719   |
| PIR7     | 0x713   |
| PIE7     | 0x71D   |
| NCO1ACCL | 0x58C (NCO1ACCH 0x58D, NCO1ACCU 0x58E) |
| NCO1INCL | 0x58F (NCO1INCH 0x590, NCO1INCU 0x591) |
| NCO1CON  | 0x592   |
| NCO1CLK  | 0x593   |
| WDTCON0  | 0x80C   |
| WDTCON1  | 0x80D   |
| PCON0    | 0x813   |
| NVMADRL  | 0x81A (NVMADRH 0x81B) |
| NVMDATL  | 0x81C (NVMDATH 0x81D) |
| NVMCON1  | 0x81E   |
| NVMCON2  | 0x81F   |
| CPUDOZE  | 0x88C   |
| RC1REG   | 0x119   |
| TX1REG   | 0x11A   |
| SP1BRGL  | 0x11B   |
| SP1BRGH  | 0x11C   |
| RC1STA   | 0x11D   |
| TX1STA   | 0x11E   |
| BAUD1CON | 0x11F   |
| SSP1BUF  | 0x18C   |
| SSP1ADD  | 0x18D   |
| SSP1STAT | 0x18F   |
| SSP1CON1 | 0x190   |
| SSP1CON2 | 0x191   |
| SSP1CON3 | 0x192   |
| PPSLOCK  | 0xE8F   |
| T0CKIPPS | 0xE91   |
| SSP1CLKPPS | 0xEC5 |
| SSP1DATPPS | 0xEC6 |
| SSP1SSPPS | 0xEC7  |
| RXPPS    | 0xECB   |
| RA0PPS   | 0xF10 (RB0PPS 0xF18, RC0PPS 0xF20, RD0PPS 0xF28; 8 consecutive per port) |
| CCPTMRS0 | 0x21E   |
| CCPTMRS1 | 0x21F   |
| CCPR1L   | 0x30C (CCPR1H 0x30D, CCP1CON 0x30E, CCP1CAP 0x30F; CCP2 from 0x310, CCP3 0x314, CCP4 0x318, CCP5 0x31C) |
| PWM6DCL  | 0x38C (PWM6DCH 0x38D, PWM6CON 0x38E; PWM7DCL 0x390, PWM7DCH 0x391, PWM7CON 0x392) |
| PIR6     | 0x712   |
| PIE6     | 0x71C   |
| CCP1PPS  | 0xEA1 (CCP2PPS 0xEA2 .. CCP5PPS 0xEA5) |
| ADRESL   | 0x8C (ADRESH 0x8D) |
| ADCON0   | 0x93    |
| ADCON1   | 0x94    |
| ADCON2   | 0x95    |
| ADCLK    | 0x98    |
| ADACT    | 0x99    |
| ADREF    | 0x9A    |
| ADPCH    | 0x9E    |
| PIR1     | 0x70D   |
| PIE1     | 0x717   |
| PIR2     | 0x70E   |
| PIE2     | 0x718   |
| FVRCON   | 0x90C   |
| DAC1CON0 | 0x90E   |
| DAC1CON1 | 0x90F   |
| CMOUT    | 0x98F   |
| CM1CON0  | 0x990 (CM1CON1 0x991, CM1NSEL 0x992, CM1PSEL 0x993; C2 from 0x994) |

## Interrupt-on-change bits

| Bit | Name     | Use                                                              |
|-----|----------|------------------------------------------------------------------|
| PIR0<4> | IOCIF | Set when any enabled IOCx flag is set. Cleared by the handler. |
| PIE0<4> | IOCIE | Gates the IOC source. The dispatcher calls IOC only when IOCIF and IOCIE are both set. |
| PIR0<5> | TMR0IF | Set on 8-bit match or 16-bit rollover, postscaled. Cleared by the handler. |
| PIE0<5> | TMR0IE | Gates the Timer0 source. Set by Init only with a callback. |
| PIR4<0> | TMR1IF | Set when TMR1H:TMR1L rolls over FFFFh to 0000h. Cleared by the handler. |
| PIE4<0> | TMR1IE | Gates the Timer1 source. Set by Init only with a callback. |
| PIR4<1> | TMR2IF | Set on the postscaled TMR2 = PR2 match. Cleared by the handler. |
| PIE4<1> | TMR2IE | Gates the Timer2 source. Set by Init only with a callback. |
| PIR1<0> / PIE1<0> | ADIF / ADIE | Set when a conversion completes. The handler clears the flag. ADTIF/ADTIE (bit 1) have no driver. |
| PIR2<0> / PIE2<0> | C1IF / C1IE | Set on an armed CxOUT edge. The handler clears the flag, then runs the callback. |
| PIR2<1> / PIE2<1> | C2IF / C2IE | Same for C2. |
| INTCON<6> | PEIE  | Set by `EPIC_IRQ_Enable` on any source. Exposes every PIRx source. |
| INTCON<7> | GIE   | Global interrupt enable.                                         |
| PIR3<0> / PIE3<0> | SSP1IF / SSP1IE | MSSP1 event. The handler clears the flag, then runs the callback. |
| PIR3<4> / PIE3<4> | TXIF / TXIE | Read-only flag, set while the transmitter is on and TX1REG is empty. TXIE is a one-shot the handler clears. |
| PIR3<5> / PIE3<5> | RCIF / RCIE | Read-only flag, set while the receive FIFO holds a byte. |
| PIR6<0> / PIE6<0> | CCP1IF / CCP1IE | CCP1 capture, compare match or PWM period. CCP2..CCP5 take bits 1..4. The handler clears the flag, then runs the callback. |

## Timer0 bits

Clock source (T0CON1<T0CS2:T0CS0>, DS40001825F Register 27-2):
000 T0CKIPPS, 001 T0CKIPPS inverted, 010 FOSC/4, 011 HFINTOSC,
100 LFINTOSC, 101 SOSC, 110 LC1_out, 111 reserved (no driver value).
T0CKIPPS counts edges on the pin `EPIC_PPS_MapInput(PPS_INPUT_T0CKI, ...)`
selects (POR pin RA4; PORTA or PORTB only).
Prescaler (T0CON1<T0CKPS3:T0CKPS0>): encoding N divides by 2^N,
0000=1:1 through 1111=1:32768. Postscaler (T0CON0<T0OUTPS3:T0OUTPS0>):
value N divides by N+1, 0000=1:1 through 1111=1:16. 8-bit mode
(T016BIT=0) matches TMR0L against TMR0H and resets on match; 16-bit
mode rolls TMR0H:TMR0L over. T0OUT toggles per postscaled
match/rollover (§27.7) and is read-only: a T0CON0 write never changes
it, so readbacks taken while the timer runs mask it out.

`EPIC_TIMER0_Init` sets T0ASYNC from the source: asynchronous rows
(T0CKIPPS, LFINTOSC, SOSC, LC1_out) run async, FOSC/4 and HFINTOSC
run synced. The handler clears TMR0IF first, then runs the owned
slot, with no cstack autos (ISR bank rule below).

Gate shape, learned the hard way: a stop-on-overflow callback always
beats main's post-Start readback (overflow at 8 ticks lands inside the
read window), so the gate proves setup callback-free and reads no
image after the armed Start. Either spin exits on its condition, so
the gate never depends on SIM wall-clock speed.

## Timer1 bits

Clock source (T1CLK<CS3:CS0>, DS40001802H Register 28-3): 0000
T1CKIPPS, 0001 FOSC/4, 0010 FOSC, 0011 HFINTOSC, 0100 LFINTOSC, 0101
MFINTOSC, 0110 SOSC, 0111 CLKR, 1000 TMR0 overflow, 1010 TMR3
overflow, 1011 TMR5 overflow, 1100..1111 LC1..LC4_out; 1001 is
reserved on Timer1 (no driver value). Prescaler (T1CON<CKPS1:CKPS0>)
divides by 1, 2, 4 or 8. FOSC clocks the counter four times per
instruction cycle (§28.2.1). The PPS inputs reset to T1CKIPPS RC0,
T1GPPS RB5 and T2INPPS RC3 (Table 13-1); `EPIC_PPS_MapInput` does not
expose these three inputs yet, so only those pins are usable.

Gate (T1GCON, T1GATE, §28.7): GE enables it, GPOL=1 counts while the
gate is high, GTM turns rising edges of the source into a toggled
level (measures a full cycle), GSPM lets one acquisition through per
GGO/DONE set, which hardware clears on the trailing edge, and GVAL
reports the gate state even with GE clear. Gate sources (T1GATE<GSS4:
GSS0>, Register 28-4): 00000 T1GPPS, 00001 TMR0 overflow, 00011
TMR2_postscaled, 00100 TMR3 overflow, 00101 TMR4_postscaled, 00110
TMR5 overflow, 00111 TMR6_postscaled, 01000/01001 SMT1/SMT2 match,
01010..01110 CCP1..5_out, 01111/10000 PWM6/7_out, 10001 DDS_out,
10010/10011 C1/C2OUT_sync, 10100 ZCD1, 10101..11000 LC1..4_out;
00010 (Timer1 itself) and 11001..11111 are reserved. The gate-event
interrupt (TMR1GIF, PIR5<0>) has no driver yet.

`EPIC_TIMER1_Init` sets RD16 so `EPIC_TIMER1_ReadCounter` (TMR1L, then
TMR1H) cannot tear on rollover, and never sets GGO/DONE: arm a single
pulse with `EPIC_TIMER1_GateArm`. Init programs the clock, gate and
prescaler registers halted; Start reloads the counter and writes
T1CON with ON. Image after Init with the default handle: T1CLK 0x01,
T1GATE 0x00, T1GCON 0x00, T1CON 0x02; after Start T1CON 0x03. T1GCON
images in the mdb gate mask GVAL, which tracks the pin live.

## Timer2 bits

Clock source (T2CLKCON<CS3:CS0>, DS40001802H Register 29-1): 0000
T2CKIPPS, 0001 FOSC/4, 0010 FOSC, 0011 HFINTOSC, 0100 LFINTOSC, 0101
MFINTOSC/16, 0110 SOSC, 0111 CLKR, 1000 NCO, 1001 ZCD1, 1010..1101
LC1..LC4_out; 1110 and 1111 are reserved. T2CON (Register 29-2): ON
bit 7, CKPS<2:0> divides by 2^N up to 1:128, OUTPS<3:0> divides by
N+1 up to 1:16. TMR2 counts 0..PR2 and clears on the clock after the
match, so TMR2IF fires every prescaler x (PR2+1) x postscaler input
cycles. Writing T2CON or TMR2 clears the prescaler and postscaler
counters; clearing ON resets all counters and the mode state machine
but leaves TMR2 untouched.

Hardware limit modes (T2HLT<MODE4:MODE0>, Register 29-3, Table 29-1):
the free-running group 0x00 (software gate), 0x01/0x02 (hardware gate
high/low), 0x03/0x04/0x05 (reset on any, rising or falling edge of
the reset signal), 0x06/0x07 (held in reset while low/high); the
one-shot group 0x08 (software start), 0x09..0x0B (edge start),
0x0C/0x0D (edge start with edge reset), 0x0E/0x0F (edge start with
level reset); the monostable group 0x11..0x13; and the level
triggered one-shots 0x16/0x17. 0x10, 0x14, 0x15 and 0x18..0x1F are
reserved. In every one-shot mode the match clears ON and stops TMR2
at 00h; monostable keeps ON and the next start edge runs again. A
pure edge start (0x09..0x0B) ignores edges mid-run, only the modes
with a reset row restart on them, and every start mode
needs a fresh edge or level after each ON cycle. T2HLT also carries
PSYNC (bit 7), CKPOL (bit 6) and CKSYNC (bit 5). The reset signal
(T2RST<RSEL4:RSEL0>, Register 29-4) is the
T2INPPS pin for 00000, TMR4/TMR6 postscaled for 00010/00011,
CCP1..5_out 00100..01000, PWM6/7_out 01001/01010, C1/C2OUT_sync
01011/01100, ZCD1 01101, LC1..4_out 01110..10001; 00001 (Timer2
itself) and 10010..11111 are reserved.

`EPIC_TIMER2_Init` programs T2CON ratios with ON clear, then T2CLKCON,
T2RST and T2HLT; Start writes PR2, zeroes TMR2 and writes T2CON with
ON. Image after Init with the default handle and PR2 7: T2CLKCON 0x01,
T2RST 0x00, T2HLT 0x00, T2CON 0x00; after Start T2CON 0x80.

Unlike the older enhanced mid-range parts, INTCON on this part carries
no IOCIF/IOCIE bits (impl 0xC1): the flag and enable live in PIR0/PIE0.
`IOCxF` flags are cleared in the handler one captured bit at a time
with a single-bit clear, never by a masked write of the whole register,
so a flag hardware sets during the handler is not overwritten. An edge
that lands after the clear re-raises IOCIF. A second edge on an
already-captured bit merges into that capture.

The handler clears all three ports' flags but reports PORTB only
(`IOCBF` mask plus the `PORTB` byte), matching the sibling families'
contract. IOCAF/IOCCF edges still vector and are still cleared, so no
armed edge sticks the vector; they just have no callback argument.

## CCP bits

CCP1..CCP5 are one module five times over (DS40001802D §30, Registers
30-1..30-6, read as text; EDC `_modsrc` DOS-02493_ccp_v4 on all five):
CCPxCON EN bit 7, OUT bit 5 (read-only), FMT bit 4, MODE value in bits
3:0. Instances sit at a 4-byte stride from CCPR1L 0x30C (CCPRxL, CCPRxH,
CCPxCON, CCPxCAP). Modes (CCPxCON<MODE3:MODE0>): 0000 off (resets the
module), 0001 toggle and clear TMR1, 0010 toggle, 0011 capture every
edge, 0100 falling, 0101 rising, 0110 every 4th rising, 0111 every 16th
rising, 1000 set on match, 1001 clear on match, 1010 pulse 0-1-0, 1011
pulse and clear TMR1, 1111 PWM; 1100..1110 are reserved and
`EPIC_CCP_Init` rejects them. Clearing CCPxCON also forces the compare
output latch low (the latch is not the PORT latch, §30.2.1).

Capture and compare use Timer1 (Timer3/Timer5 on the other couples) and
want it clocked from FOSC/4 or an external clock, synchronized, not
FOSC (§30.1.2, §30.2.2). CCPxCAP<CTS2:CTS0> picks the capture trigger
(Register 30-2): 000 CCPxPPS pin, 001 C1OUT, 010 C2OUT, 011 the IOC
event, 100..111 LC1..LC4_out. CCPTMRS0 holds C1TSEL..C4TSEL (bit pairs
1:0, 3:2, 5:4, 7:6) and CCPTMRS1 holds C5TSEL (1:0), P6TSEL (3:2) and
P7TSEL (5:4); 01 is the Timer1/Timer2 couple (the POR value, 0x55 and
0x15), 10 Timer3/Timer4, 11 Timer5/Timer6 and 00 is reserved (Registers
30-5, 30-6). CCPR1H:L holds the capture or compare value, or the PWM
duty.

PWM (§30.3): the period is 4 x (PR2 + 1) counts of 1/4 timer step
(Equation 30-1), the duty is a 10-bit count of the same unit (Equation
30-3), so 100% is 4 x (PR2 + 1) or more and the resolution is
log2(4 x (PR2 + 1)) bits, 10 at PR2 255. FMT clear keeps duty bits 9:8
in CCPRxH<1:0> and 7:0 in CCPRxL; FMT set keeps bits 9:2 in CCPRxH and
bits 1:0 in CCPRxL<7:6> (Figure 30-5). The duty reaches a 10-bit
buffer when TMR2 = PR2, so a write at any time is glitch-free and takes
effect at the next period; a 0 duty never raises the pin. The PWM
timer must run from FOSC/4 (§30.3.4) and the postscaler plays no part.
Setup order from §30.3.2: route the pin and keep its TRIS bit set, load
PR2, write CCPxCON and the duty, start the timer, and clear TRIS once
the first period has passed. CCPxIF sets every period.

`EPIC_CCP_Init` clears CCPxCON, writes CCPTMRS and CCPxCAP, loads CCPRxH:L
(the compare value, or the duty in the FMT layout), clears CCPxIF, arms
PIE6 (plus PEIE) only with a callback, and writes CCPxCON last with EN
set for every mode but OFF. It starts no timer. `EPIC_CCP_SetMode` keeps
FMT and the value; the datasheet advises writing OFF before changing a
capture prescaler and clearing CCPxIF after any mode change, because a
mode change can raise a false event (§30.1.3, §30.1.4). Images with the
default handle: CCPxCON 0x00, CCPxCAP 0x00, CCPTMRS0 0x55, CCPTMRS1
0x15; compare-set 0x1234 gives CCPxCON 0x88, CCPRxH 0x12, CCPRxL 0x34;
PWM with the duty 0x155 right-aligned gives 0x8F, 0x01, 0x55 and left
aligned 0x9F, 0x55, 0x40. CCPxCON.OUT is live read-only status and
masks out of the gate's CCPxCON images.

## PWM6 and PWM7 bits

Two dedicated 10-bit PWM modules (DS40001802D §19, Registers 19-1..19-3,
read as text; EDC `_modsrc` DOS-02719_pwm_10bit_v3_8Bit): PWMxCON EN bit
7, OUT bit 5 (read-only), POL bit 4 (set: active low; impl 0xB0),
PWMxDCH duty bits 9:2, PWMxDCL duty bits 1:0 in bits 7:6 (impl 0xC0).
PWM6 sits at 0x38C..0x38E and PWM7 at 0x390..0x392. The time base is
Timer2, 4 or 6 (P6TSEL, P7TSEL in CCPTMRS1, §30.3.3), clocked from
FOSC/4; the period, resolution and duty formulas are the CCP PWM ones
(Equations 19-1..19-4), and the duty latches at TMR2 = PR2 with no
interrupt flag. A disabled module sits at its idle level, 0 for POL
clear and 1 for POL set (§19.1). `EPIC_PWM_Init` follows §19.1.9: POL is
written with EN clear, then the time base and duty, and EN last. Image
for PWM6 with the duty 0x2A5, active high: PWM6CON 0x80, PWM6DCH 0xA9,
PWM6DCL 0x40.

## EUSART bits

Asynchronous mode only; the module matches the sibling PIC16F193X
EUSART (DS41364B §23). Bit positions are DFP-checked by the sfr-map
audit, the DS page cites are unchecked.

- Baud: Fosc / (D * (n + 1)), D = 64 (BRGH=0, BRG16=0), 16 (BRGH=1,
  BRG16=0 or BRGH=0, BRG16=1), 4 (BRGH=1, BRG16=1). `USART_ComputeSPBRG`
  rounds to the nearest divisor and returns 0xFFFF when n does not fit
  (255 on the 8-bit generator, 65535 on the 16-bit one).
- `Init` writes the port off first, loads SP1BRGL/H, BAUD1CON.BRG16 and
  TX1STA (BRGH, TX9, TXEN), and writes RC1STA (SPEN, CREN, RX9, ADDEN)
  last. ADDEN is set only with 9-bit frames.
- TXIF is read-only: a TX1REG write clears it, and it reads set again once
  the byte moves to the shift register. Because it stays set while the
  buffer is empty, `EPIC_USART_EnableTxInterrupt` is a one-shot: the handler
  clears TXIE before it runs the callback.
- RCIF is read-only and clears when RC1REG is read with the FIFO empty. The
  FIFO is two deep; a third byte raises OERR, cleared by dropping CREN
  (`EPIC_USART_ClearOverrun`).
- 9-bit: set TX9D before writing TX1REG. RX9D belongs to the byte at the
  FIFO head and must be read before RC1REG: `EPIC_USART_GetRX9D` does that
  by hand, the RX handler latches it for callbacks
  (`EPIC_USART_GetLastRX9D`).
- Pins: TX needs `EPIC_PPS_MapOutput(..., PPS_OUTPUT_EUSART_TX)` and a TRIS
  output, RX needs `EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, ...)` and a
  digital input (see PPS for the ports that work).
- Not driven: synchronous mode, auto-baud, break, wake-up, SCKP.

## MSSP bits

MSSP1 as SPI master or I2C master; the module matches the sibling
PIC16F193X MSSP (DS41364B). Bit positions are DFP-checked, the DS page
cites are unchecked.

| SSPM | Mode | Period (ticks of Fosc/4) |
|------|------|--------------------------|
| 0000 | SPI master Fosc/4 | 1 |
| 0001 | SPI master Fosc/16 | 4 |
| 0010 | SPI master Fosc/64 | 16 |
| 1000 | I2C master, Fosc/(4*(SSP1ADD+1)) | SSP1ADD + 1 |
| 1010 | SPI master, Fosc/(4*(SSP1ADD+1)) | SSP1ADD + 1 |

The Timer2 clock mode (0011) waits for the Timer2 port; the slave modes
are not driven.

- `Init` writes SSP1CON1 = 0 first, then SSP1STAT (SMP, CKE), SSP1ADD and
  SSP1CON2, and SSP1CON1 (SSPM, CKP, SSPEN) last. In I2C mode CKE and CKP
  stay clear and SMP is the slew-rate control: set for 100 kHz and 1 MHz,
  clear for 400 kHz. `SSP_ComputeADD` rounds the divisor up so the bus is
  never faster than asked.
- SPI: a SSP1BUF write starts a byte, BF and SSP1IF set when it finishes,
  and the read clears BF. A write during a transfer sets WCOL and sends
  nothing; a byte that ends while BF is still set sets SSPOV and is lost.
- I2C: each call sets one SSP1CON2 bit (SEN, RSEN, PEN, RCEN, or ACKDT then
  ACKEN) and returns. The hardware clears the bit when the operation ends
  and sets SSP1IF; `EPIC_SSP_I2C_IsIdle` also waits out a byte write
  (SSP1STAT.RW). ACKSTAT is 0 when the slave acknowledged the last written
  byte. Bus collision is PIR3.BCL1IF, polled, with no handler.
- Pins: SPI needs SCK and SDO outputs (`PPS_OUTPUT_SSP1_SCK`, `_SDO`) and
  SDI as an input (`PPS_INPUT_SSP1DAT`), I2C drives SCL and SDA through the
  same SCK and SDO output codes and reads them back through
  `PPS_INPUT_SSP1CLK` and `PPS_INPUT_SSP1DAT`; the bus needs external
  pull-ups.

## ADC bits

*DS40001802D §23, Registers 23-1 to 23-8, Table 23-2.* Basic polling
mode only: ADCON1 and ADCON2 stay at POR (no precharge, no double
sample, ADMD 000 basic), ADCONT is clear (one conversion per trigger).
Clock (ADCON0<ADCS>): FRC dedicated oscillator (the default, errata-safe
at 32 MHz) or FOSC through ADCLK<ADCCS> (value N divides FOSC by
2*(N+1), 000000 = FOSC/2 through 111111 = FOSC/128). Format
(ADCON0<ADFRM0>): left (result in ADRESH:ADRESL<7:6>) or right
(ADRESH:ADRESL); `EPIC_ADC_Read` returns the 10-bit value in both.
References (ADREF): ADPREF 00 VDD, 10 VREF+ pin, 11 FVR buffer 1 (01
reserved); ADNREF 0 AVSS, 1 VREF- pin. Channels (ADPCH): ANA0..ANA7
0x00..0x07, ANB 0x08..0x0F, ANC 0x10..0x17 on every part, AND 0x18..0x1F
and ANE0..ANE2 0x20..0x22 on the 40-pin parts, then 0x3C AVSS, 0x3D
temperature indicator, 0x3E DAC1 output, 0x3F FVR (Register 23-8; the
0x23..0x3B codes are reserved). `EPIC_ADC_SelectChannel` rejects a
reserved code and a pin code the part lacks. Trigger (ADACT, Table 23-2):
0x00 manual (`EPIC_ADC_Start`), 0x02..0x08 timers, 0x0B..0x11 CCP/PWM,
0x12/0x13 comparator outputs, 0x01 ADACTPPS pin, plus the SMT, IOC, CLC
and read-back sources. ADGO starts a conversion and hardware clears it;
ADIF (PIR1<0>) sets at completion and the handler clears it. The
threshold flag ADTIF and the computation registers (ADPRE, ADACQ,
ADACC, ADRPT, ADLTH/ADUTH, ADSTAT) have no driver.

## DAC1 bits

*DS40001802D §25, Registers 25-1/25-2.* 5-bit code in DAC1CON1<DAC1R>
(VOUT = (VSRC+ - VSRC-) * code/32 + VSRC-). Sources (DAC1CON0): PSS 00
VDD, 01 VREF+ pin, 10 FVR buffer 2 (11 reserved); NSS 0 VSS, 1 VREF-
pin. Pin enables OE1 (DAC1OUT1, RA2) and OE2 (DAC1OUT2, RB7); with both
clear the DAC still feeds the ADC (channel 0x3E) and the comparators
(PCH 5) internally. `EPIC_DAC_SetValue` changes the code with the DAC
running and rejects codes above 31.

## FVR bits

*DS40001802D §16, Register 16-1.* Two independent gain buffers off the
1.024V reference: ADFVR (buffer 1, to the ADC input channel and the
ADC positive reference) and CDAFVR (buffer 2, to the DAC and the
comparators), each 00 off, 01 1.024V, 10 2.048V, 11 4.096V (output
cannot exceed VDD). FVRRDY is read-only: hardware sets it once the
reference stabilizes, and it reads always 1 on 16F18855/75 once FVREN
is set. TSEN/TSRNG (temperature indicator) stay off, so the ADC
temperature channel (0x3D) has no stimulus from this HAL.

## Comparator bits

*DS40001802D §18, Registers 18-1 to 18-5.* One enable bit per instance
(CMxCON0<ON>), hysteresis (HYS), polarity (POL) and edge enables
(CMxCON1 INTP/INTN); SYNC stays asynchronous. Inputs: NCH 0..3 are
CxIN0- (RA0), CxIN1- (RA1), CxIN2- (RB3), CxIN3- (RB1), 6 is FVR
buffer 2, 7 is AVSS (4/5 unconnected); PCH 0 is CxIN0+ (RA2), 1 is
C1IN1+ (RA3) or C2IN1+ (RB0), 5 is the DAC1 output, 6 is FVR buffer 2,
7 is AVSS (2..4 unconnected). `EPIC_COMP_Init` rejects the unconnected
codes. The output reads on CMxCON0<OUT> or CMOUT<MCxOUT> (both
post-polarity, read-only) and reaches a pin only through RxyPPS
(`PPS_OUTPUT_C1OUT` 0x12, `PPS_OUTPUT_C2OUT` 0x13, Table 13-3); there is
no output-to-pin bit. CxIF (PIR2) sets on an armed OUT edge and the
handler clears it, then runs the instance slot. `EPIC_COMP_ReadOutput`
reads low for an unknown instance.

## PPS

PPS routes peripherals to pins; the HAL never touches TRIS or ANSEL, so
pair every mapping with `EPIC_GPIO_Init`. Addresses are 0xE8F.. and
0xF10.. on five parts and 0x1E8F.. and 0x1F10.. on 16F18857/16F18877. All
values below come from the DFP EDC (`ppsval`, POR and impl masks); the
datasheet tables are unchecked.

| Input | Register | POR pin | Field |
|-------|----------|---------|-------|
| T0CKI | T0CKIPPS | RA4 | 4 bits: port A or B |
| MSSP1 clock | SSP1CLKPPS | RC3 | port<<3 \| pin |
| MSSP1 data | SSP1DATPPS | RC4 | port<<3 \| pin |
| MSSP1 SS | SSP1SSPPS | RA5 | port<<3 \| pin |
| EUSART RX | RXPPS | RC7 | port<<3 \| pin |
| CCP1 capture | CCP1PPS | RC2 | port<<3 \| pin |
| CCP2 capture | CCP2PPS | RC1 | port<<3 \| pin |
| CCP3 capture | CCP3PPS | RB5 | port<<3 \| pin |
| CCP4 capture | CCP4PPS | RB0 | port<<3 \| pin |
| CCP5 capture | CCP5PPS | RA4 | port<<3 \| pin |

Port field: A 0, B 1, C 2, D 3 (40-pin parts). Output codes written to
RxyPPS: 0x00 port latch, 0x09..0x0D CCP1..CCP5, 0x0E PWM6, 0x0F PWM7,
0x10 EUSART TX, 0x14 MSSP1 SCK/SCL, 0x15 MSSP1 SDO/SDA, 0x19 NCO1
(DS40001802D Table 13-3; the EDC lists many more; they belong to unported
peripherals).

The EDC's `ppsports` attribute on each output's RemappablePin names the
ports it reaches: TX, SCK and SDO1 are B and C on every part; NCO1 is A and
C on 16F18854/55/56, A and B on 16F18857, A and D on 16F18875/76/77. MPLAB
SIM 6.35 agrees (W1600, "invalid attempt to map pin", for NCO1 on RC2 of a
16F18877) and raises W1600 ("invalid attempt to map pin") for TX, SCK and SDO on PORTA and
PORTD pins, for RX on PORTA and PORTD, and for SS1 on PORTB and PORTC;
every other combination tried (TX, SCK, SDO on PORTB and PORTC, RX on B
and C, SS1 on A and D, SSP1CLKPPS and SSP1DATPPS on any port) maps clean.
The driver writes any valid-looking pin and leaves the choice to the
caller: use PORTB or PORTC for the EUSART and MSSP outputs, and a port
from the list above for NCO1 (PORTA reaches it on every part).

The CCP inputs and outputs follow the same rule. The routes the gate
maps clean on 16F18877: CCP1 in on RB2, CCP2 in on RC6, CCP3 in on RB4,
CCP4 in on RB7, CCP5 in on RA1; CCP1 out on RC2, CCP2 out on RC1, CCP3
out on RB6, CCP4 out on RB7, CCP5 out on RA3, PWM6 out on RA1, PWM7 out
on RA2. W1600 fires for PWM6 on RB4, the CCP3 input on RC0 and the CCP5
output on RC0, so those routes stay out of the gate.

Lock: `EPIC_PPS_Unlock` and `EPIC_PPS_Lock` run the 0x55, 0xAA key writes
and then a single `bcf` or `bsf` on PPSLOCKED, consecutively with
interrupts masked, as a platform macro (`epic_pps_unlock_sequence` and
`epic_pps_lock_sequence`). The last step must be a bit instruction: XC8
compiles `reg &= ~1` to `movlw 0xFE; andwf`, and MPLAB SIM does not take
that (nor a whole-register `movwf`) as the unlock. PPSLOCKED is clear
after reset, so mapping needs no unlock. With PPS1WAY set (the XC8
default) the hardware lets the bit be cleared and set once; MPLAB SIM
accepts one sequence per reset, a Lock first, and leaves PPSLOCKED
unchanged for an Unlock before it or after it. With PPS1WAY OFF, which the
mdb gate's config sets, the cycle repeats. Map after reset and `Lock` once
when PPS1WAY is on.

## Data EEPROM

*DS40001802D §10.4, Registers 10-1 to 10-6, Table 10-3.* 256 bytes, read
and written through the NVM block, not through EEADR/EEDATA as on the
older families. NVMCON1<NVMREGS> = 1 selects the EEPROM, user ID and
configuration space; the EEPROM is NVMADRH:NVMADRL = 0xF000 + byte
(0xF000 to 0xF0FF, the EDC EEDataSector). `EPIC_EEPROM_*` always writes
NVMADRH = 0xF0.

Read: write NVMADR, set RD; the byte is in NVMDATL in the next
instruction and RD clears in hardware. `EPIC_EEPROM_ReadByte` waits for a
running write first: NVMADR is in use until WR clears.

Write: set WREN, load NVMADR and NVMDATL, then the unlock sequence: 0x55
to NVMCON2, 0xAA to NVMCON2, set WR in NVMCON1, with nothing between. The
sequence is the `epic_nvm_start_write` platform macro (target: `banksel`,
two key writes and a `bsf NVMCON1,1`; the PPS lock sequence uses the same
shape), run with GIE off. The write includes its own erase (FREE is not
used) and takes about 5.6 ms (EDC wait time). The CPU keeps running: WR
clears in hardware and NVMIF sets when it finishes, with an interrupt if
NVMIE is set. `EPIC_EEPROM_WriteByte` returns as soon as the write has
started, EPIC_BUSY while one is running, and the caller polls
`EPIC_EEPROM_IsWriteComplete` or passes a callback to `EPIC_EEPROM_Init`.
The driver writes NVMCON1 in one store (NVMREGS and WREN, so FREE, LWLO and
WRERR clear) before each write, and `IsWriteComplete` drops WREN once WR is
clear, so a write stays enabled only while it runs.

WRERR sets when the unlock sequence is interrupted or WR is set for a
write-protected address (the CPD configuration bit); the hardware does not
clear it. `EPIC_EEPROM_HasWriteError` reads it, so call it after
`IsWriteComplete` and before the next `WriteByte`, which clears it.

## NCO1

*DS40001802D §24, Registers 24-1 to 24-8.* A 20-bit accumulator adds a
20-bit increment on every input clock; the carry is the raw output, so
the overflow frequency is clock x increment / 2^20. FDC mode (N1PFM = 0)
toggles the output on each overflow: 50% duty at half the overflow
frequency. PFM mode makes it active for 2^N1PWS input clocks per
overflow (a longer pulse than the overflow period is undefined). N1POL
inverts the output; N1OUT reads it back. N1OUT is live read-only status,
and MPLAB SIM leaves it at its last level once the module is disabled, so
the gate masks it out of the NCO1CON images.

Clock select (NCO1CLK<N1CKS2:N1CKS0>) on this part: 000 FOSC, 001
HFINTOSC, 010 to 101 LC1_out to LC4_out, 110 and 111 reserved (other
NCO parts order these differently). The increment registers are double
buffered: written enabled, the three bytes load together on the second
NCO clock after the NCO1INCL write, so write NCO1INCU and NCO1INCH first
(`EPIC_NCO_SetIncrement` does). Disabled, they load at once. The
accumulator has no atomic access: `EPIC_NCO_ReadAccumulator` may straddle
an addition while running, and a write while running gives undefined
results, so `EPIC_NCO_WriteAccumulator` refuses it (EPIC_BUSY). NCO1IF
(PIR7<4>) sets on each overflow, needs N1EN, NCO1IE, PEIE and GIE, and is
cleared in the handler. The NCO keeps running in Sleep on a clock that
runs there. A Reset clears every NCO register except NCO1INCL (1).

## WDT, Sleep and Idle

*DS40001802D §9.0 (Registers 9-1 to 9-5), §8.2, §8.2.4, Register 8-2 and
§5.13 Register 5-2.* The CONFIG2 WDTE bits pick the mode: 11 always on, 10
on except in Sleep, 01 under software control of WDTCON0<SWDTEN>, 00 off.
`EPIC_WDT_Enable` and `EPIC_WDT_Disable` only change SWDTEN, so they act
in the 01 mode; in the others the bit is ignored. WDTPS (bits 5:1) is a
2^(N+5) divider of the 31 kHz clock: 1 ms at 0, 2 s at 11 (the reset value
when software controls it), 256 s at 18; values 19 to 31 are reserved and
give the minimum. WDTPS is writable only while CONFIG3 WDTCPS = 11111 (the
XC8 default); `EPIC_WDT_Enable` reads it back and returns EPIC_ERROR when
a fixed period overrides the request. Any write to WDTCON0 clears the WDT.

The window (CONFIG3 WDTCWS, or WDTCON1<WINDOW> when WDTCWS = 111) is not
configured by this HAL. In a windowed configuration the WDT has to be
armed by a WDTCON0 read before a CLRWDT counts (§9.5.1), so the target
`EPIC_WDT_Refresh` reads WDTCON0 and then executes `clrwdt`.

`EPIC_Sleep_Enter` is `sleep` followed by `nop` (§8.2.1: the instruction
after SLEEP runs before any interrupt vector). With CPUDOZE<IDLEN> set
the same instruction enters Idle: the CPU and flash stop, peripherals on
FOSC keep running (§8.2.4), and any enabled interrupt ends it, with GIE
off or on. `EPIC_Idle_Enter` sets IDLEN around one `EPIC_Sleep_Enter` and
restores it to the value it had. A WDT time-out in Sleep or Idle wakes the
part without a Reset (§9.6).

PCON0 reset flags POR and BOR are active low: hardware clears the bit on
the event and software sets it back (§5.13). `EPIC_POR_GetStatus` and
`EPIC_BOR_GetStatus` return 1 while the bit reads clear;
`EPIC_POR_ClearFlag` and `EPIC_BOR_ClearFlag` set it.

## ISR bank rule

XC8 -O2 emits a bank select for every volatile SFR and file-scope
static access, but never for cstack autos: an auto read or written
while BSR holds another bank hits the wrong address. Read in the
family's own `.s` (the `ioc_capture` switch form first): the restore
`movlb` is missing, `IOCBF` never clears, and the mdb gate stays red.
The same `.s` shows a static pinned to bank 2 getting a correct
`movlb 2` before every access, so the rule is proven both ways.
Therefore ISR-reachable code (the vector, the dispatcher, the IOC,
Timer0..Timer2, ADC, C1/C2, CCP1..CCP5, EUSART and MSSP handlers) keeps
no cstack autos: flag snapshots are file-scope statics, tests run on
WREG, and every SFR names its register literally. The five CCP and two
comparator handlers follow it with one scalar callback slot each, and
the CCP1 and CCP5 `.s` show the PIR6 clear as a single `andwf` and a
`movlb` before the slot load.
Normal (non-ISR) driver code is unaffected: calls reset the tracking
there, and the gate's register readbacks prove it.

## SIM gaps

MPLAB SIM ignores a software write to `PIR0.IOCIF`: the flag never
sets, so a software-triggered IOC never vectors in the gate. The
`gpio-sim` probe checks the write reads back first and runs its live
vector section (callback runs, both flags clear) only where it does:
the host sim and real silicon. On the SIM gate it covers the register
images and the IOC enable path. A software write to `IOCxF` does land
in SIM (the gate observes `IOCBF` set), only the aggregate `IOCIF`
does not follow it.

Timer0 runs freely under `run`: an 8-tick overflow lands inside
main's post-Start read window, so the gate never asserts a post-Start
image with the stop-callback armed (see Timer0 bits). The LC1 row has no
stimulus model anywhere and never ticks; T0CKIPPS edges come from pins
the host sim drives, and the SIM gate drives none.

MPLAB SIM models the EUSART (TXIF, TRMT, the one-shot TX interrupt all
behave as on the host sim) but does not shift MSSP1 data: after a
SSP1BUF write BF and SSP1IF never set, so the gate checks the SPI and I2C
register images and runs the SPI byte only where BF sets (host sim,
silicon). A software write to `PIR3.SSP1IF` does read back and vector,
unlike `PIR0.IOCIF`. The RX interrupt has no stimulus on the SIM gate (RCIF is
read-only), so its handler is checked in the generated `.s`: both latches
are statics, every SFR access carries its own bank select, no autos.
PPS mappings the SIM rejects print W1600 and still write the register
(see PPS); the gate maps only combinations that map clean.

Timer1 and Timer2 follow the Timer0 gate shape (callback-free setup
proof, then a vectored proof whose callback stops the timer). Their
gate proofs use RB5 (T1G) and RC3 (T2IN) configured as outputs: MPLAB
SIM feeds the output latch back through the POR PPS mapping, so the
gate level is deterministic without a stimulus file. MPLAB SIM prints
a W0106 partial-support warning for TMR5 at load; it is unrelated to
the Timer1/Timer2 sections. The gate-event interrupt (TMR1GIF) has no
driver. Every defined Timer2 mode runs in the host sim. MPLAB SIM
holds the measured start-controlled modes at zero and never starts
them (2026-10 on the 16F18877 for 0x09, 0x11 and 0x16: images and
the armed hold pass, no edge or level starts the counter), so the
gate covers mode 0x09 register images and the armed hold only.

The gate image is built -O2. Probe sections live in small helper
functions that share spin loops and handle resets, since inlining every
readback into main overflowed the 16F18855's 8K words of flash (link error
1347). With the EEPROM, NCO, WDT, Idle, CCP and PWM sections added the
image is 13746 words and fits only the 16K-word 16F18856 or larger; the
CI gate runs on the 16F18877 (42% of its flash), and the other parts
still build the blink example and the demo.

A CPU-executed EEPROM write completes under MPLAB SIM 6.35 on the
16F18877 (2026-10): WR clears at once, the byte reads back through NVM,
and NVMIF stays clear. This differs from the PIC18 families, where the
write never completes, so the gate reads its writes back and asserts no
NVMIF; its bounded wait is only a guard. MPLAB SIM enters Idle on SLEEP
with IDLEN set but does not run Timer0 there (TMR0 stayed frozen and the
CPU never woke), so the gate wakes Idle with a WDT time-out instead: its
config has WDTE=SWDTEN, and the watchdog runs only between
`EPIC_WDT_Enable` and `EPIC_WDT_Disable`. The NCO is checked as register
images only, no output timing.

CCP1..CCP5 and PWM6/PWM7 in MPLAB SIM 6.35 (probed 2026-10): a software
write to `PIR6.CCPxIF` reads back and vectors, the compare-set match
flags and vectors on its own, and PWM runs against Timer2 (CCPxCON.OUT
and PWM6CON.OUT follow the pulse; high-sample ratios of 7/32 and 16/32
for 25% and 50% duty). Capture never fired in any probe (every edge,
rising with CCP1PPS remapped, every 4th rising edge, RC2 driven through
its latch), so the gate has no capture proof. The timer-clearing and
pulse compare modes did not read like the host model: at the flag TMR1H
read 0 and OUT read low for toggle-with-clear, pulse and
pulse-with-clear, while plain toggle read OUT high and TMR1H at the
match count. The gate pins their CCPxCON images only. W1600 fires for
PWM6OUT on RB4, for the CCP3 input on RC0 and for the CCP5 output on RC0;
the gate maps only routes that map clean (PPS tables above).

The analog gate sections are images only. MPLAB SIM was not probed for
ADC completion timing (ADGO clear, ADIF set) or for comparator/FVR
live status (CxOUT, FVRRDY), so the gate asserts the control images
with ADGO, CxOUT and FVRRDY masked and never waits on a conversion;
the host probes cover completion, triggers and edges instead. The top
pin channel goes analog input first (RE2/ANE2, RC7/ANC7): selecting it
while digital prints SIM warnings W0214/W0215.

## Weak pull-ups

Each port's `WPUx` register enables its own pull-ups directly; the pack
ships no `OPTION_REG` for this part, so there is no global enable to
manage. Reset state is `WPUx=0x00`: pull-ups stay off until set.

## Interrupt vector

Single vector, no priority. Hardware saves context automatically
(`common/MANUAL.md` §6). The ISR vector is in
`src/target/pic16f188xx_isr_vector.c`.

## Host simulation model

`pic16f188xx_sim_step` refreshes PORTx from latch and driven inputs,
then samples IOC on every port: an edge matching the port's IOCxP/IOCxN
mask sets that port's IOCxF flags and PIR0.IOCIF. The sim callback runs
only under IOCIF, IOCIE, PEIE, and GIE. Pin reads are not gated on
ANSEL (see README known limits). Timer0 advances per tick with
prescaler, 8/16-bit counting, postscaler and T0OUT toggle modelled;
only the internal sources (FOSC/4, HFINTOSC, LFINTOSC, SOSC) tick, and
delivery to the dispatcher gates on TMR0IF, TMR0IE, PEIE and GIE like
silicon. One divergence: a full T0CON0 write clears T0OUT in the sim
array, while silicon preserves the read-only bit; tests pin T0CON0
images only where no full write intervenes.

The EUSART, MSSP1 and PPSLOCK models are in `pic16f188xx_sim_serial.c`.
Registers with side effects go through `epic_sfr_read8`/`epic_sfr_write8`,
which the host platform header routes to hooks (`EPIC_REG8` stays the raw
array cell, and a direct `EPIC_REG8` poke skips the hook): a TX1REG write
loads the transmitter, an RC1REG read pops the receive FIFO, an SSP1BUF
write starts a byte, and the read-only bits (TRMT, RX9D, OERR, FERR,
RCIDL, the SSP1STAT status bits, ACKSTAT, TXIF, RCIF) keep their hardware
value across a software write. Time is instruction ticks: the EUSART bit
time is D * (n + 1) / 4 ticks and an SPI or I2C period is 1, 4, 16 or
SSP1ADD + 1 ticks per the SSPM mode. Probes stimulate it with
`pic16f188xx_sim_usart_rx_inject`, `_spi_set_miso`/`_set_loopback` and one
scripted I2C slave (`_i2c_set_slave`) whose bus events land in a trace
(`_i2c_trace`). Not modelled: pin routing (the serial models ignore
PPS), framing errors, bus collision, the slave modes and PPS1WAY.

Timer1 and Timer2 advance per tick the same way. Timer1 counts for the
internal sources (FOSC/4, HFINTOSC, LFINTOSC, MFINTOSC, SOSC, and FOSC
at four counts per tick), for each Timer0 overflow pulse, and for each
rising edge of RC0 (the POR T1CKIPPS pin); its gate reads RB5 (POR
T1GPPS), the Timer0 overflow pulse or the Timer2 postscaled pulse, and
models GPOL, GTM (toggle flip-flop cleared while the timer is off),
GSPM with GGO/DONE (cleared on the trailing edge) and GVAL.
Timer2 ticks the same internal sources and models every defined mode
against RC3 (POR T2INPPS): edge-start waits for its edge and the match
clears ON, monostable restarts on the edge with ON kept, level modes
follow ers, and a start latch cleared by each ON cycle keeps a stale
start from leaking across Start calls. Reserved codes and every
unmodelled clock row (T2CKIPPS included) freeze the counter, and every
unmodelled gate and reset row reads low, which holds a gated counter
except in active-low gate mode, where a low signal lets it run.
The pin rows read the PORTx array at the POR PPS defaults (the PPS
select registers of these three inputs are not modelled, and
`EPIC_PPS_MapInput` does not expose them), so a test drives them with
`pic16f188xx_sim_drive_input` ('C' 0, 'B' 5, 'C' 3) or by making the
pin an output; an output pin reads back its latch, and pin reads are
not gated on ANSEL. The sim does not model the RD16 buffer or the
prescaler clear on a TMR1L/TMR2 write: counter registers are plain
memory, and the prescaler clears only while the timer is off.

The EEPROM, NCO1 and WDT/Sleep models are in `pic16f188xx_sim_sys.c`,
on the same hook pair. NVMCON2 collects the 0x55, 0xAA key state and an
NVMCON1 write that sets WR consumes it: without WREN the request is
ignored, without the keys (or outside the data EEPROM) it sets WRERR, and
otherwise it stores the byte after `pic16f188xx_sim_eeprom_set_write_ticks`
ticks (default 0: inside the write, NVMIF set). A read loads NVMDATL at
once. The EEPROM erases to 0xFF at `pic16f188xx_sim_reset`;
`pic16f188xx_sim_eeprom_peek` reads the backing store. NCO1 adds the
buffered increment once per tick for the FOSC and HFINTOSC clocks (the LCx
clocks freeze it; one NCO clock per instruction tick, so the accumulator
arithmetic is exact and only the clock ratio is simplified), models FDC
toggling, PFM pulses and polarity, the increment double buffer (two clocks
after an enabled NCO1INCL write) and NCO1IF. `EPIC_WDT_Refresh`,
`EPIC_Sleep_Enter` and Idle entries only count
(`pic16f188xx_sim_wdt_refresh_count`, `_sleep_count`, `_idle_count`); the
simulator has no WDT time-out, and `pic16f188xx_sim_wdt_set_fixed_period`
pins WDTPS the way CONFIG3 WDTCPS does. PCON0 resets to 0x3D, WDTCON0 to
0x16 and WDTCON1 to 0x07. Not modelled: program Flash, user ID and
configuration NVM access, the one-instruction read stall, the NVM
write protect, and the WDT window.

CCP1..CCP5 and PWM6/PWM7 are stepped each tick after the timers, in
`pic16f188xx_sim_ccp.c`. Only the Timer1/Timer2 couple (CCPTMRS value
01) runs: capture and compare use TMR1, PWM uses a 10-bit time base
built from TMR2 (TMR2 x 4 plus the prescaler position in quarter
steps) and runs only while Timer2 is on and clocked from FOSC/4; any
other couple, or a Timer2 clock other than FOSC/4, holds the output
low (high for an active-low PWM6/PWM7). Compare matches on the tick
TMR1 reaches the compare value (an edge: a halted timer cannot match
again), sets CCPxIF and drives the output; the pulse modes hold the
output high for one tick and the two clear modes zero TMR1. Capture
reads the pin CCPxPPS selects (the PPS select is modelled, and
`EPIC_PPS_MapInput` writes it) when CCPxCAP selects it; the other
capture sources read low. Every CCPxCON write that changes EN or MODE
resets the output latch, the pulse and the capture prescaler, and OUT
is read-only (`pic16f188xx_sim_ccp_write`, reached through
`epic_sfr_write8`). PWM latches the duty at the TMR2 = PR2 match, sets
CCPxIF there, and reads high until the 10-bit position reaches the
duty; a 0 duty never raises the output. Probes read the outputs with
`pic16f188xx_sim_ccp_out` and `pic16f188xx_sim_pwm_out` (the CCP
output latch, and the PWM pin level after polarity); pin routing is
not modelled, so no output reaches a PORTx bit.

The ADC, FVR, DAC1 and comparators are stepped each tick after the
CCP model, in `pic16f188xx_sim_analog.c`, with delivery to the
dispatcher on ADIF with ADIE and on CxIF with CxIE (gated on PEIE and
GIE like silicon). A set ADGO with the module on and no conversion
running arms a two-tick conversion: ADRES loads the injected sample
for the selected channel (`pic16f188xx_sim_adc_set_sample`, per ADPCH
code, 10-bit), ADGO clears and ADIF sets. The manual, Timer0-overflow,
Timer2-match and comparator-output-rising triggers start conversions;
every other ADACT source is a SIM gap. FVRRDY follows FVREN. DAC1 has
no dynamic state. Each comparator evaluates its NSEL/PSEL sources in
millivolts at VDD 5000 mV (pins from the digital PORTx level, FVR
buffer 2 from CDAFVR, DAC1 from DAC1CON0/1, AVSS 0), updates CxOUT and
CMOUT with polarity applied, and raises CxIF on armed OUT edges. Not
modelled: acquisition timing, computation modes, the threshold
interrupt, hysteresis voltage, SYNC, and the temperature indicator.
