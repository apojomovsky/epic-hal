# PIC16F83_84-family HAL

The bare classic mid-range tier (PIC16F83, PIC16F84, PIC16F84A; 18-pin,
0.5/1/1 KW flash, 36/68/68 B RAM, 64 B data EEPROM; DS35007B for the
84A, DS30189 for the 83/84). Same die peripherals on every part: the
SFR map is byte-identical across all three (DFP-verified), so every
driver comes from the shared `pic14-midrange-core/` and this directory
holds only the part-specific shim: SFR map, platform headers, IRQ
table, sim backend, harnesses, tests, and docs.

## Peripheral coverage

GPIO (PORTA 5-pin + PORTB), Timer0, data EEPROM (64 B), WDT, Sleep.
No USART, CCP, Timer1/2, comparator, VREF, ADC, SSP, PSP, PIR/PIE
registers, or PCON/BOR (absent silicon on the whole family). The
EEPROM interrupt is the family's signature quirk: the enable (EEIE)
is INTCON<6> and the flag (EEIF) is EECON1<4>.

## Register placement deltas vs the other 14-bit families (DFP-verified)

EEDATA/EEADR live in Bank 0 (the 628A keeps all four EEPROM registers
in Bank 1, the 87XA/88X in Banks 2/3); there is no PIR1/PIE1, so the
shared dispatcher's PIR section compiles out and the EEPROM event is
gated from EECON1/INTCON directly; common RAM is 0x40..0x4F (not
0x70..0x7F), which moves the ISR scratch pins. The shared drivers
select all of this through `PIC14MIDRANGE_HAS_*` (see
`pic14_midrange.h`).

## Verification

Host simulation (`cmake -B build && cmake --build build`, run
`./build/example_*`), real-target XC8 (`make xc8-build
MODULE=pic16f83_84-hal MCU=16F84A`), and the `mdb` gate (`make
mdb-test MODULE=pic16f83_84-hal MCU=16F84A DEVICE=PIC16F84A`, MODE=gpio
over RA0; no USART to print markers).
`docs/adding-a-device.md` §4 is the mandatory per-peripheral gate.
