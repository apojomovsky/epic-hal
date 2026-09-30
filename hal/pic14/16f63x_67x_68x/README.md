# PIC16F63x/67x/68x HAL

Classic mid-range PIC16 family (RP0/RP1 banking, single vector at
0x0004) on the shared `pic14-midrange-core`. All nine parts are
classified: the 16F677 canonical (20-pin, 2 KW flash, 128 B SRAM,
256 B EEPROM) proves the full tier with the bank probe, and the
639/684/685/688/689 siblings prove their shapes the same way. The
three 1 KW parts (630/631/676) are manifest-excluded (the tier does
not fit 1 KW/64 B) with host-sim coverage only. Capability macros
per part select ports, ANSEL/ANSELH, comparator shape, EEPROM and
WDTCON homes, and PIR2; see `MANUAL.md` for the shape table.

## Peripheral tier

GPIO (PORTA RA0..RA5; PORTB RB4..RB7 and PORTC RC0..RC7 where the
shape has them), Timer0, Timer1 (gate on the 4-bank shapes),
dual comparators C1/C2 with VRCON reference (CM1CON0 shapes only),
data EEPROM, fail-safe clock monitor flag, WDT with software
enable, Sleep. ADC, SSP, CCP, Timer2, EUSART, and the legacy
CMCON/CMCON0 comparators are silicon on some parts with no driver
yet; their interrupt flags stay undispatched.

## Build and test

Host sim: `cmake -B build && cmake --build build`, run any
`build/example_*` directly. Real target: `make xc8-build
MODULE=pic16f63x_67x_68x-hal MCU=16F677` (blink) and the `MODE=gpio`
mdb gate `make mdb-test MODULE=pic16f63x_67x_68x-hal MCU=16F677`
(bank probe, RA0 marker). No USART exists, so every mdb gate on this
family is MODE=gpio.

## XC8 codegen gotchas (live)

- Banked SFRs span all four banks on the 20-pin shapes (Bank 1:
  OPTION/TRIS/PIE/PCON/OSCCON/WPUA/IOCA/WDTCON; Bank 2:
  EEDATA/EEADR/WPUB/IOCB/VRCON/CMx/ANSEL; Bank 3: EECON1/EECON2/
  SRCON). The 2-bank shapes keep EEPROM/VRCON in Bank 1, ANSEL in
  Bank 1 on the 14-pin ADC parts, and WDTCON in Bank 0; the
  audit's `BANK_VARIANT_ADDRS` pins those homes. Every
  value-from-C-local write goes through the `EPIC_BANKn_*`
  literal-token macros in
  `include/target/pic16f63x_67x_68x_platform.h`; plain C access while
  banked misdirects (the 628A EEPROM precedent). The macros
  stringify the token for asm, so they take real SFR names: the
  `ANSEL_BANK1`/`WDTCON_BANK0` aliases exist only for the host/sim
  constant path.
- WDTCON is Bank 1 on the 4-bank shapes and Bank 0 on the 2-bank WDT
  shapes (Bank 2 on the 88X): the shared software-WDT control
  selects the home through `PIC14MIDRANGE_HAS_WDTCON_BANK1/BANK0`.
- The RB-change flag is RABIF (PORTA/B change, INTCON<0>), not RBIF;
  shared code spells it RBIF through a compat alias on the same bit.
- C1OUT/C2OUT are read-only live outputs: mask them out of register
  comparisons (see `docs/adding-a-device.md` §4 step 8).
