# PIC16F63x/67x/68x HAL

Classic mid-range PIC16 family (RP0/RP1 banking, single vector at
0x0004) on the shared `pic14-midrange-core`. Onboarded on this
ticket: 16F631 (20-pin, 1 KW flash, 64 B SRAM, 128 B EEPROM,
manifest-excluded: the tier does not fit) and 16F677 canonical
(20-pin, 2 KW flash, 128 B SRAM, 256 B EEPROM, ADC + SSP), which
proves the full tier. The #152
siblings (630/639/676/684/685/688/689) reuse the tree under
capability macros.

## Peripheral tier (631/677)

GPIO (PORTA RA0..RA5, PORTB RB4..RB7, PORTC RC0..RC7), Timer0, Timer1
with gate, dual comparators C1/C2 with VRCON reference, data EEPROM,
fail-safe clock monitor flag, WDT with software enable, Sleep. No
ADC, USART, SSP, CCP, or Timer2 on this die (DS40001262F Table 1);
those peripherals arrive with the siblings that carry them.

## Build and test

Host sim: `cmake -B build && cmake --build build`, run any
`build/example_*` directly. Real target: `make xc8-build
MODULE=pic16f63x_67x_68x-hal MCU=16F631` (blink) and the `MODE=gpio`
mdb gate `make mdb-test MODULE=pic16f63x_67x_68x-hal MCU=16F631`
(bank probe, RA0 marker). No USART exists, so every mdb gate on this
family is MODE=gpio.

## XC8 codegen gotchas (live)

- Banked SFRs span all four banks (Bank 1: OPTION/TRIS/PIE/PCON/
  OSCCON/WPUA/IOCA/WDTCON; Bank 2: EEDATA/EEADR/WPUB/IOCB/VRCON/CMx/
  ANSEL; Bank 3: EECON1/EECON2/SRCON). Every value-from-C-local write
  goes through the `EPIC_BANKn_*` literal-token macros in
  `include/target/pic16f63x_67x_68x_platform.h`; plain C access while
  banked misdirects (the 628A EEPROM precedent).
- WDTCON is Bank 1 here, Bank 2 on the 88X: the shared software-WDT
  control selects the bank through `PIC14MIDRANGE_HAS_WDTCON_BANK1`.
- The RB-change flag is RABIF (PORTA/B change, INTCON<0>), not RBIF;
  shared code spells it RBIF through a compat alias on the same bit.
- C1OUT/C2OUT are read-only live outputs: mask them out of register
  comparisons (see `docs/adding-a-device.md` §4 step 8).
