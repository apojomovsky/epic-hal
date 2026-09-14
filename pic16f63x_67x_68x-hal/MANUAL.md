# PIC16F63x/67x/68x HAL manual

Register facts for the PIC16F63x/67x/68x family that are not shared
conventions (those live in `epic-common/MANUAL.md`) and not driver
behavior (each driver cites its own datasheet sections). Everything
here is cited to DS40001262F (631/677/685/689/690) and cross-checked
against the DFP (`Microchip.PIC16Fxxx_DFP`, `edc/PIC16Fnnn.PIC` and
`xc8/pic/include/proc/pic16f631.h`).

## Family shape

Nine parts in two die shapes (parsed from each part's EDC SFRDefs,
consistent with DS40001262F Table 1):

| part | pins | flash | SRAM | EEPROM | USART | ADC | CCP | SSP | COMP | T2 | PIR2 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 16F630 | 14 | 1 KW | 64 B | 128 B | - | - | - | - | CMCON | - | - |
| 16F631 | 20 | 1 KW | 64 B | 128 B | - | - | - | - | CM1CON0 | - | Y |
| 16F639 | 20 | 2 KW | 128 B | 256 B | - | - | - | - | CMCON0/1+LVD | - | - |
| 16F676 | 14 | 1 KW | 64 B | 128 B | - | Y | - | - | CMCON | - | - |
| 16F677 | 20 | 2 KW | 128 B | 256 B | - | Y | - | Y | CM1CON0 | - | Y |
| 16F684 | 14 | 2 KW | 128 B | 256 B | - | Y | Y | - | CMCON0/1 | Y | - |
| 16F685 | 20 | 4 KW | 256 B | 256 B | - | Y | Y | - | CM1CON0 | Y | Y |
| 16F688 | 14 | 4 KW | 256 B | 256 B | Y | Y | - | - | CMCON0/1 | - | - |
| 16F689 | 20 | 4 KW | 256 B | 256 B | Y | Y | - | Y | CM1CON0 | - | Y |

The 14-pin shapes are two-bank parts (single PIR1/PIE1); the 20-pin
shapes spread SFRs across all four banks (PIR1+PIR2). EUSART appears
only on 688/689, ECCP only on 684/685. The 16F677 canonical proves
the full tier with the bank probe; the 639/684/685/688/689 siblings
prove their shapes the same way. The three 1 KW parts (630/631/676)
cannot link the tier: the 1058-word 676 blink overflows flash (XC8
error 1347, measured), the 630 has no common RAM for banked access
and its 942-word blink leaves no headroom, and the bank probe
cannot fit 64 B RAM (error 1250 measured on the same-die 631), so
they are manifest-excluded variants with host-sim coverage only.
The -1 DFP spellings (16F631-1 and siblings) are a non-goal: XC8
v4.00 rejects every -mcpu spelling with error 2043. Only the
CM1CON0 dual-comparator shape has a driver; the CMCON single
shape (630/676) and the CMCON0/CMCON1 shape (639/684/688, plus
LVD and CRC on the 639) are never enabled and undispatched.
Likewise ADC, SSP, CCP, Timer2 and USART silicon is present on
some parts with no driver yet. Timer1 gating is compiled in on
the 4-bank shapes only (87XA precedent); the 2-bank parts count
unconditionally.

## 16F631 register map

Bank 0: INDF/TMR0/PCL/STATUS/FSR/PORTA/B/C, PCLATH/INTCON/PIR1/PIR2,
TMR1L/H, T1CON. Bank 1: OPTION/TRISA/B/C, PIE1/PIE2, PCON, OSCCON,
OSCTUNE, WPUA, IOCA, WDTCON. Bank 2: EEDATA/EEADR, WPUB, IOCB, VRCON,
CM1CON0/CM2CON0/CM2CON1, ANSEL. Bank 3: EECON1/EECON2, SRCON.

PORTB is RB4..RB7 only (TRISB/POR = 0xF0); PORTA is RA0..RA5 (0x3F).
ANSEL covers ANS0/1/4/5/6/7 (AN2/AN3 do not exist). The PORTA/B-change
flag is RABIF/RABIE (INTCON<0>/<3>); PIR1 holds TMR1IF only; PIR2
holds EEIF/C1IF/C2IF/OSFIF at bits 4..7. CM2CON1 carries C2SYNC,
T1GSS, MC2OUT/MC1OUT (no CxRSEL bits, unlike the 88X); the reference
select is CxR in CMxCON0 plus CxVREN in VRCON. WDTCON (SWDTEN +
4-bit WDTPS) is Bank 1 at 0x97.

## Interrupts

Single vector at 0x0004, no priority. Eight sources on the exemplar:
RB (PORTA/B change), INT (RA2), TMR0, TMR1, EEPROM, C1, C2, OSF.
Single-page flash (1 KW), so the dispatcher carries no page pin.

## Flash and RAM budget

16F631: 1 KW flash, 64 B SRAM (Bank-0 GPR 0x40..0x6F plus common
0x70..0x7F), 128 B data EEPROM. 16F677: 2 KW flash, 128 B SRAM,
256 B data EEPROM. Measured on the canonical 677 (XC8 v4.00): the
blink smoke uses 1321 words / 62 B (64.5%/48.4% of flash/RAM, clear
of the 20%-headroom bar); no 1 KW/64 B part can hold either image
(see the exclusion notes above). The ISR scratch bytes pin to
common RAM at 0x70/0x71. The blink ISR toggles the port latch
directly (the GPIO driver call path in the ISR partition tips the
64 B RAM allocation over the limit; the 83_84 probe records the same
