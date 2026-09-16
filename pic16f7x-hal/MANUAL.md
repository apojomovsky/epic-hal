# PIC16F7x HAL manual

Register facts for the PIC16F7x family that are not shared conventions
(those live in `epic-common/MANUAL.md`) and not driver behavior (each
driver cites its own datasheet sections). Everything here is cited to
DS30325 (16F72-77) and DS30498 (16F737-777), and cross-checked against
the DFP (`Microchip.PIC16Fxxx_DFP`, `edc/PIC16Fnnn.PIC` and
`xc8/pic/include/proc/pic16f77.h`).

## Family shape

Nine parts in two die generations (parsed from each part's EDC SFRDefs,
consistent with DS30325/DS30498):

| part | pins | flash | SRAM | USART | ADC result | CCP | SSP | PSP | PIR2 |
|---|---|---|---|---|---|---|---|---|---|
| 16F72 | 28 | 2 KW | 128 B | - | 8-bit ADRES | CCP1 | Y | - | - |
| 16F73 | 28 | 4 KW | 192 B | Y | 8-bit ADRES | CCP1+CCP2 | Y | - | Y |
| 16F74 | 40 | 4 KW | 192 B | Y | 8-bit ADRES | CCP1+CCP2 | Y | Y | Y |
| 16F76 | 28 | 8 KW | 368 B | Y | 8-bit ADRES | CCP1+CCP2 | Y | - | Y |
| 16F77 | 40 | 8 KW | 368 B | Y | 8-bit ADRES | CCP1+CCP2 | Y | Y | Y |
| 16F737 | 28 | 4 KW | 368 B | Y | 10-bit ADRESH/L | CCP1+CCP2 | Y | - | Y |
| 16F747 | 40 | 4 KW | 368 B | Y | 10-bit ADRESH/L | CCP1+CCP2 | Y | Y | Y |
| 16F767 | 28 | 8 KW | 368 B | Y | 10-bit ADRESH/L | CCP1+CCP2 | Y | - | Y |
| 16F777 | 40 | 8 KW | 368 B | Y | 10-bit ADRESH/L | CCP1+CCP2 | Y | Y | Y |

The DS30325 parts (72-77) carry one 8-bit ADC result register (`ADRES`,
Bank 0 address 0x1E, no ADRESH/ADRESL, no ADCON2, no ADFM/ADCS2). The
DS30498 parts (737-777) carry the 10-bit ADRESH/ADRESL pair plus
ADCON2/ADCON1<ADFM>:ADCS2. The shared `pic14_adc.c` reads the result
through `PIC16F7X_FAMILY_ADC_10BIT` to pick the 8-bit single-register
or 10-bit pair path. `16F72` is the minimal die: no USART, no CCP2, no
PIR2/PIE2 (its interrupt surface is PIR1-only); 0x10D is PMADRL, with
no PMDATA program-memory byte.

The 28-pin parts (72/73/76/737/767) have no PortD/PortE and no PSP;
the 40-pin parts (74/77/747/777) carry PORTD/TRISD, PORTE/TRISE and the
PSP (the shared driver gates on `PIC16F7X_FAMILY_HAS_PSP`).

## No data EEPROM

This family has no data EEPROM on any part (DFP-verified: no EECON*
registers). The Bank-2 program-memory block (PMDATA 0x10C, PMADR 0x10D,
PMDATH 0x10E, PMADRH 0x10F, PMCON1 0x18C) replaces the EEPROM SFRs.
The `pic14_eeprom.c` driver is not in this family's manifest slice, and
the shared IRQ dispatch keeps its EEPROM rows gated off via
`PIC14MIDRANGE_HAS_EE_PIR1`/`HAS_EE_PIR2` set to 0.

## Register placement (DS30325)

Core SFRs sit in Bank 0 (0x00..0x1F): PIR1 0x0C, PIR2 0x0D (40-pin and
CCP2 parts), TMR1/T1CON, TMR2/T2CON, SSP, CCP1/CCP2, USART, ADC, ADCON0
0x1F. Bank 1 (0x80..0x9F): OPTION 0x81, TRISx, PIE1 0x8C, PIE2 0x8D,
PCON 0x8E, PR2 0x92, SSPADD/SSPSTAT, TXSTA 0x98, SPBRG 0x99, ADCON1 0x9F.
Bank 2/3: the PM* program-memory block (PMDATA/PMADR/PMDATH/PMADRH at
0x10C..0x10F, PMCON1 at 0x18C). No EEPROM bank.

Bank-1 writes (TRISx, PIE1/PIE2, TXSTA, SPBRG, ADCON1) go through the
`EPIC_BANK1_*` literal-token macros in
`include/target/pic16f7x_platform.h` under XC8 v4.00, the same
bank-misdirection fix as the 87XA/88X families. The 16F72 (PIE1-only)
uses the PIR2-gated variants of the PIE macros so no PIE2 assembler
symbol is referenced on a die that lacks it.

## Interrupt map

PIR1/PIE1 carry the RB/INT/TMR0/TMR1/TMR2/CCP1/SSP/TX/RX/ADC/PSP rows.
PIR2/PIE2 (40-pin CCP2 parts and the DS30498 28-pin parts) carry only
CCP2IF (DFP-verified; no BCL, no EEPROM flag, no comparator). The
translation table `src/core/pic16_irq_table.c` enumerates the rows that
exist on the die; the 16F72 table has no CCP2 row (no PIR2).
