# PIC16F818_819 HAL manual

Register facts for the PIC16F818/16F819 family that are not shared
conventions (those live in `epic-common/MANUAL.md`) and not driver
behavior (each shared `pic14-midrange-core/` driver cites its own
datasheet sections). DS39598F is authoritative for this family; every
address below is cited to it and cross-checked against the pinned DFP
(`Microchip.PIC16Fxxx_DFP`, `edc/PIC16F818.PIC` and
`edc/PIC16F819.PIC`).

## Family shape

18-pin PDIP/SOIC, 20-pin SSOP and 28-pin QFN packages, 16 I/O pins and
an 8-level hardware stack (DS39598F §1.0, Figure 1-1). Table 1-1 gives
1 KW flash, 128 B RAM and 128 B data EEPROM on the PIC16F818, and 2 KW,
256 B and 256 B on the PIC16F819. Both parts carry the same 42 SFRs at
the same addresses (DFP-verified, Figures 2-3/2-4), so only the memory
sizes separate them.

PORTA: RA0..RA4 are GPIO, RA5 is MCLR/VPP only (an input-only pin whose
TRISA bit has no effect and reads as 1, Table 2-1 note 3), and RA6/RA7
carry the oscillator/CLKO function (Table 1-2), usable as I/O in the
INTRC and external-clock modes. PORTB: RB0/INT, RB4..RB7
interrupt-on-change with weak pull-ups on the port, RB1/RB2/RB4/RB5 as
the SSP pins (SDI/SDA, SDO, SCK/SCL, SS), RB2/RB3 as the two CCP1 pin
options (CCPMX selects which), and RB6/RB7 shared with the debugger and
the ICSP clock/data lines.

Peripherals: Timer0, Timer1 (no gate input: T1CON has no TMR1GE or
T1GINV), Timer2, CCP1 (classic capture/compare/PWM, not ECCP), SSP (SPI
master/slave plus I2C slave), 10-bit 5-channel ADC on AN0..AN4 and
128/256 B data EEPROM. WDT, Sleep, PCON (nBOR/nPOR, Register 2-8),
OSCCON (IRCF2:IRCF0, IOFS; Register 4-2) and OSCTUNE (TUN5:TUN0;
Register 4-1) complete the nanoWatt surface. Absent on the whole die:
USART, comparator (no CMCON anywhere in any bank), VREF module (no
CVRCON), CCP2, SSPCON2, SSPMSK, ANSEL/ANSELH, WDTCON, PORTC/PORTD/PORTE,
BCL, ULPWU, OSF, and any HAL path that self-writes flash program memory.

Core register references, all DS39598F register numbers: STATUS 2-1,
OPTION_REG 2-2, INTCON 2-3, PIE1 2-4, PIR1 2-5, PIE2 2-6, PIR2 2-7 and
PCON 2-8 in the memory chapter, OSCTUNE 4-1 and OSCCON 4-2 in the
oscillator chapter, then T1CON 7-1, T2CON 8-1, CCP1CON 9-1, SSPSTAT
10-1, SSPCON 10-2 and ADCON0 11-1, one per peripheral chapter.

## EEPROM placement

The data-EEPROM registers keep the PIC16F87XA placement (DFP-verified,
Table 3-1): EEDATA 0x10C, EEADR 0x10D, EEDATH 0x10E and EEADRH 0x10F
form the Bank 2 data pair plus the high bytes, and EECON1 with EECON2
(not a physical register) form the Bank 3 control pair at 0x18C/0x18D.
EECON1 is Register 3-1: its EEPGD/FREE pair is what would steer the same
registers at program memory, and the HAL's EEPROM driver never sets it.
The write-complete flag is PIR2 bit 4, EEIF (Register 2-7), with its
enable EEIE at PIE2 bit 4 (Register 2-6) and the row in Table 3-1: the
87XA/88X placement, not the EECON1<4> flag of the 83/84/84A and not the
PIR1<7> flag of the 628A.

Addresses run 0x00..0x7F on the 128 B part and 0x00..0xFF on the 256 B
part, both fully inside EEADR alone; EEDATH/EEADRH are the program-memory
high bytes, and the driver leaves them clear. The write sequence is the
mandatory 0x55/0xAA EECON2 unlock, and the driver reaches the Bank 2 and
Bank 3 registers through the literal-token `EPIC_BANK2_*`/`EPIC_BANK3_*`
macros rather than a runtime address: on the 628A the runtime-address
form compiled clean, passed on the host and silently misdirected on
silicon (see `pic16f628a-hal/MANUAL.md`), which is the same class of
failure the family gate exists to catch.

## ADC reference and channel selection

DS39598F §11.0, Registers 11-1/11-2 and Table 11-1.

ADCON0 (address 1Fh) holds ADCS1:ADCS0 in bits 7:6, CHS2:CHS0 in bits
5:3, GO/DONE in bit 2 and ADON in bit 0. ADCON1 (address 9Fh) holds ADFM
in bit 7, ADCS2 in bit 6 and PCFG3:PCFG0 in bits 3:0. There is no ANSEL
and no ADCON2 here: PCFG does both the analog select and the reference
select.

PCFG3:PCFG0 is the whole table (Register 11-2). Each of its rows states
which of AN4..AN0 are analog and whether the reference pair is AVDD/AVSS
or the external pins, with a summary column giving the analog-channel and
reference counts per row: 011x makes every pin digital, 0000 is all five
channels analog on AVDD/AVSS, and the rows that use an external reference
take VREF+ on AN3 and VREF- on AN2. Those reference pins are the AN pins
themselves (Table 1-2: RA2/AN2/VREF-, RA3/AN3/VREF+), so selecting an
external reference spends that input channel; there is no internal
reference ladder to select instead.

The channel is CHS2:CHS0, 000..100 for AN0..AN4, and both parts
implement five channels (Table 1-1). Every pin used as an analog input
also needs its TRIS bit set as an input (§11.3); the ADC works
independently of the CHS and TRIS bits, so a pin left as a digital
output converts its own output level.

The conversion clock is ADCS2:ADCS1:ADCS0, selecting 2, 4, 8, 16, 32 or
64 TOSC or the module's own RC oscillator (Table 11-1); a 10-bit
conversion takes 9.0 TAD (§11.2), and TAD must stay within 1.6 us to
6.4 us. Only the RC source lets a conversion run and complete in Sleep
(§11.5), and the CCP1 special event trigger (CCP1M3:CCP1M0 = 1011)
starts a conversion while resetting Timer1 (§11.7).

The result is 10 bits in ADRESH:ADRESL, right-justified with ADFM = 1
(ADRESH reads six zeros) and left-justified with ADFM = 0 (§11.4.1). A
completed conversion clears GO/DONE and sets ADIF, PIR1<6>. The shared
driver's `ADC_ReferenceTypeDef` enumerators are the raw PCFG value (see
`pic14-midrange-core/include/peripherals/pic14_adc.h`), so `EPIC_ADC_Init`
programs PCFG, ADCS2 and ADFM in the one ADCON1 write; read each
enumerator as its PCFG row, because the names carry the 87XA channel
counts (8CH, 7CH, and so on) while this die implements five channels.

## Common RAM

Bank-independent GPR is the top of each bank window, one physical block
mirrored into Banks 1, 2 and 3, and its size is part-specific: the upper
64 bytes (0x40..0x7F) on the PIC16F818 and the upper 16 bytes
(0x70..0x7F) on the PIC16F819 (DS39598F §12.11, Figures 2-3/2-4).
`PIC14MIDRANGE_COMMON_RAM_BASE` is 0x70, the window both parts share, so
the ISR scratch bytes `epic_irq_pie_scratch` and `epic_bank1_scratch`
pin to 0x70/0x71 on either part (defined in
`pic14-midrange-core/src/target/pic16_isr_vector.c`, whose literals the
target platform header repeats), the same addresses the 87XA/88X/7x use.

## Interrupts

Single vector at 0x0004, no priority (Figures 2-1/2-2, §12.10), and no
`__at(0x900)` page pin: the 1 KW and 2 KW parts are both single-page,
unlike the 4 KW-and-up families.

Nine sources: RB<7:4> change, the RB0/INT pin and Timer0 overflow in
INTCON (Register 2-3); Timer1, Timer2, CCP1, SSP and the ADC in
PIR1/PIE1 (Registers 2-4/2-5, with ADIF/ADIE at bit 6, bits 4-5 being
the USART pair this die does not have); and the EEPROM write-complete
event alone in PIR2/PIE2 (Registers 2-6/2-7, bit 4). `PIC16_IRQn` is
therefore `RB, INT, TMR0, TMR1, TMR2, CCP1, SSP, ADC, EEPROM` with
`IRQ_TABLE_SIZE = 9U`, and the dispatcher's PIR2 block reduces to the
single EEPROM row.

Reading PORTB is what ends the RB<7:4> mismatch condition, so RBIF must
be cleared after that read, not before it; the shared GPIO handler
already orders it that way.

## Flash and RAM budget

1/2 KW flash, 128/256 B RAM and 128/256 B data EEPROM (Table 1-1). The
PIC16F819 is the canonical part: the manifest's variant list ends on it,
and the scaffold gate, the reference project and the real-target smoke
all build it, while the PIC16F818 is carried as the small variant with
its own per-device build.

Budget figures are measured on the exemplar ladder (XC8 v4.00, -O2 for
the real-target smokes), and they stay there: this file records the
mechanism, not a number that goes stale the next time the ladder moves.
The mechanism worth remembering is the 83_84 lesson: a peripheral driver
called from an ISR permanently reserves that call path in XC8's compiled
stack, so the ISR call graph, not the application code, is usually the
RAM pivot. Where a ladder entry cannot be trimmed to fit the PIC16F818's
1 KW / 128 B, that part carries a measured exclusion on the family's
pseudo-module rather than an assumed fit. Peripheral-heavy applications
must watch both budgets; see `docs/adding-a-device.md` §3.2.
