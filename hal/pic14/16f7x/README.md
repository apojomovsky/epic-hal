# PIC16F7x HAL

Classic mid-range PIC16 family (RP0/RP1 banking, single vector at
0x0004) on the shared `pic14-midrange-core`. Nine parts in two
generations: DS30325 (16F72-77) and DS30498 (16F737-777). The 16F77
canonical (40-pin, 8 KW flash, 368 B SRAM) proves the full tier end to
end: host-sim blink, real XC8 build, the MPLAB SIM banked-SFR probe and
the epic-cc gate all pass, and the remaining eight variants build the
same host-sim blink. Capability macros per part select ports, USART,
CCP2, PSP, PIR2 and the ADC result shape; see `MANUAL.md` for the shape
table.

## Peripheral tier

GPIO (PORTA/PORTB/PORTC, PORTD/PORTE on the 40-pin parts), Timer0/1/2,
CCP1 (CCP2 on every part except 16F72), USART (no USART on the 16F72), SPI-only SSP, 8-bit or 10-bit
ADC (per generation), WDT/Sleep/BOR/POR. No data EEPROM (the PM*
program-memory block replaces it), no comparator, no Vref, no I2C
(SPI-only MSSP, no SSPCON2/BCL). The shared IRQ dispatch rows for
comparator/EEPROM/BCL compile out via the `PIC14MIDRANGE_HAS_*`
selectors.

## Build and test

Host sim: `cmake -B build && cmake --build build`, run any
`build/example_*` directly. Real target: `make xc8-build
MODULE=pic16f7x-hal MCU=16F77` (blink) and the USART mdb gate `make
mdb-test MODULE=pic16f7x-hal MCU=16F77 DEVICE=PIC16F77` (banked-SFR
probe, MODE=uart). Epic-cc gate: `make epiccc-build MODULE=pic16f7x-hal
MCU=16F77 EPIC_CC_HOST=1` against the p16f77 target.

## XC8 codegen gotchas (live)

- Banked SFRs span Banks 1/2/3 on the larger parts (Bank 1: TRISx,
  PIE1/PIE2, TXSTA/SPBRG, ADCON1; Bank 2: PMDATA/PMADR; Bank 3:
  PMCON1). Every value-from-C-local write goes through the
  `EPIC_BANKn_*` literal-token macros in
  `include/target/pic16f7x_platform.h`; plain C access while banked
  misdirects (the 87XA precedent).
- The 16F72 has no PIR2/PIE2. Its `EPIC_PIE_ENABLE/DISABLE_BIT` use the
  PIR2-gated variants in the platform header so no PIE2 assembler symbol
  is referenced on a die that lacks it.
- ADC result shape differs across the family: the DS30325 8-bit single
  `ADRES` vs the DS30498 10-bit `ADRESH:ADRESL`. The shared driver and
  sim backend select the read path through
  `PIC14MIDRANGE_HAS_ADC_10BIT`.
