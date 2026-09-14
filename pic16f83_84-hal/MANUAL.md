# PIC16F83_84 HAL manual

Register facts for the PIC16F83/84/84A family that are not shared
conventions (those live in `epic-common/MANUAL.md`) and not driver
behavior (each shared core driver cites its own datasheet sections).
Everything here is cited to DS35007B (16F84A) or DS30189 (16F83/84) and
cross-checked against the DFP.

## Family shape

18 pins, 13 I/O (RA0..RA4 + RB0..RB7), 0.5/1/1 KW flash, 36/68/68 B
RAM (two banks: Bank 0 GPR 0x0C.., Bank 1 mirror), 64 B data EEPROM,
Timer0, WDT, Sleep. The interrupt surface is four sources: INT pin,
RB<7:4> change, Timer0 overflow, EEPROM write complete. No
USART/CCP/timers 1-2/comparator/VREF/ADC/SSP/PSP, no PIR/PIE
registers, no PCON (no BOR), no PEIE, no IRP.

## EEPROM placement and the EEIF quirk

EEDATA/EEADR are Bank 0 (0x08/0x09); EECON1/EECON2 are Bank 1
(0x88/0x89). The write-complete flag EEIF is EECON1<4> (DS35007B
Register 3-2), not a PIR bit and not INTCON: the shared dispatcher and
the EEPROM driver gate on EECON1<EEIF> and INTCON<EEIE> through
`PIC14MIDRANGE_HAS_PIR1 == 0`. The Bank-0 data pair is written through
the `EPIC_BANK0_*` literal-token macros for the same reason the 628A
routes its Bank-1 pair through `EPIC_BANK1_*`: a runtime-address or
plain RMW access misdirects under XC8 on banked parts.

## Common RAM

Bank-independent GPR is 0x40..0x4F on the 84/84A (mirrored at
0xC0..0xCF in Bank 1); the bigger 14-bit parts use 0x70..0x7F. The ISR
scratch bytes (`epic_irq_pie_scratch`/`epic_bank1_scratch`) pin to
0x40/0x41 via `PIC14MIDRANGE_COMMON_RAM_BASE` (the target platform
header repeats the literals because including pic14_midrange.h from a
platform header cycles back through the family umbrella). The 16F83's
36 B map ends at 0x2F, so nothing can pin there and no smoke fits the
part (see the manifest exclusion).

## Interrupts

Single vector at 0x0004, no priority (as on the 87XA). Four sources:
RB, INT, TMR0, EEPROM. All parts are single-page flash, so the
dispatcher carries no `__at(0x900)` page pin.

## Flash and RAM budget

0.5/1 KW flash, 36/68 B RAM, 64 B data EEPROM. Measured on the
exemplar (XC8 v4.00, -O2): the blink smoke uses 844 words / 46 B on
the 84/84A; the 16F83 (512 words / 36 B) cannot hold it on either
axis, which is its manifest exclusion. The ISR-only call graph is the
RAM pivot: the blink ISR toggles the port latch directly because
XC8's compiled stack would permanently reserve the
`EPIC_GPIO_TogglePin` call path, and that path alone tips the
allocation over 68 B. Peripheral-heavy applications must watch both
budgets; see `docs/adding-a-device.md` §3.2.
