# PIC16F818_819-family HAL

The PIC16F818/16F819 two-part mid-range family (18-pin PDIP/SOIC,
20-pin SSOP, 28-pin QFN; 16 I/O pins; 1/2 KW flash, 128/256 B RAM,
128/256 B data EEPROM; DS39598F). Same die peripherals on both parts:
the SFR map is byte-identical across the two (DFP-verified), only the
flash/RAM/EEPROM sizes differ, so every driver comes from the shared
`pic14-midrange-core/` and this directory holds only the part-specific
shim: SFR map, platform headers, IRQ table, sim backend, harnesses,
tests, and docs.

## Peripheral coverage

GPIO (PORTA + PORTB), Timer0/1/2, CCP1 (classic capture/compare/PWM),
SSP (SPI master/slave and I2C slave), 10-bit 5-channel ADC (AN0..AN4),
data EEPROM (128 B; 256 B on the 16F819), WDT, Sleep, PCON (nBOR/nPOR),
OSCCON/OSCTUNE/BOR. No USART, no comparator, no VREF module (the ADC
reference is VDD/AVSS or the external VREF+/VREF- pins on AN3/AN2), no
CCP2, no SSPCON2/SSPMSK (so no I2C master), no ANSEL (analog select is
the ADCON1 PCFG3:PCFG0 table), no WDTCON (the WDT is configuration
only), no PORTC/PORTD/PORTE, no PSP, and PIR2/PIE2 carry the EEPROM
event only (EEIF/EEIE bit 4).

## Register placement deltas vs the other 14-bit families (DFP-verified)

The ADC (ADCON0/ADCON1/ADRESH/ADRESL), CCP1, Timer1, Timer2, SSP and
data EEPROM blocks share the 87XA addresses and bit layouts, and the
EEPROM keeps the 87XA placement: the Bank 2 data pair plus the high
bytes (EEDATA/EEADR/EEDATH/EEADRH at 0x10C..0x10F), the Bank 3 control
pair (EECON1/EECON2 at 0x18C/0x18D) and the write-complete flag in
PIR2<4>. The deltas are the absent USART block (0x18..0x1A), the absent
CCP2 block, OSCCON/OSCTUNE at 0x8F/0x90, PORTB as the only second port
(RA5 is MCLR/VPP only), and common RAM at 0x70..0x7F on the 16F819 (the
16F818 widens the common window to 0x40..0x7F, DS39598F §12.11), so the
ISR scratch pins land at 0x70/0x71 on either part, as on the 87XA/88X.
The shared drivers select all of this through the `PIC14MIDRANGE_*`
capability macros, defined in `include/pic14_midrange.h`.

## Verification

Host simulation (`cmake -B build && cmake --build build`, then run each
`./build/example_*`), real-target XC8 (`make xc8-build
MODULE=pic16f818_819-hal MCU=16F819`, and `MCU=16F818` for the small
part), and the `mdb` gate (`make mdb-test MODULE=pic16f818_819-hal
MCU=16F819 DEVICE=PIC16F819 MODE=gpio`; the family has no USART, so the
harness reports the pass/fail verdict on RA0). `docs/adding-a-device.md`
§4 is the mandatory per-peripheral gate.
