# PIC16F628A HAL

Single-part family (PIC16F628A, 18-pin, 2 KW flash, 224 B RAM, 128 B
data EEPROM, DS40044G). Every driver comes from the shared
`pic14-midrange-core/`; this directory holds only the part-specific
shim: SFR map, platform headers, IRQ table, sim backend, harnesses,
tests, and docs.

## Peripheral coverage

GPIO (PORTA/B only), Timer0/1/2, CCP1, USART (8-bit BRG), comparators
(single CMCON, Bank 0), VREF (VRCON), data EEPROM (128 B, Bank 1),
WDT/BOR/POR. No ADC, SSP, CCP2, PSP, PIR2 (absent silicon).

## Register placement deltas vs PIC16F87XA (DFP-verified)

CMCON lives in Bank 0 (0x1F, not Bank 1); the EEPROM block lives in
Bank 1 (EEDATA/EEADR/EECON1/EECON2 at 0x9A..0x9D, not Banks 2/3);
comparator and EEPROM interrupt flags live in PIR1/PIE1 (CMIF/CMIE
bit 6, EEIF/EEIE bit 7; no PIR2/PIE2). The shared drivers select all
of this through `PIC14MIDRANGE_HAS_*` (see `pic14_midrange.h`).

## Verification

Host simulation (`cmake -B build && cmake --build build`, run
`./build/example_*`), real-target XC8 (`make xc8-build
MODULE=pic16f628a-hal MCU=16F628A`), and the `mdb` gate (`make
mdb-test MODULE=pic16f628a-hal MCU=16F628A DEVICE=PIC16F628A`).
`docs/adding-a-device.md` §4 is the mandatory per-peripheral gate.
