# PIC16F193X HAL Manual

Per-family register-level reference for the PIC16F193X HAL. Every
register fact here is cited to **DS41364B** (the PIC16F193X/LF193X data
sheet). The family-agnostic conventions, the status codes, the
handle pattern, the harness, and the interrupt model's shared half live
in [`../pic8-common/MANUAL.md`](../pic8-common/MANUAL.md); this file
covers only what is genuinely PIC16F193X-specific and points back there
instead of repeating.

The current Microchip revision splits the family across DS41364C
(1934/1936/1937), DS40001574D (1938/1939), DS41575 (1933). The
SFR/peripheral register layout is identical across all six, so this
manual cites the local rev-B DS41364B (which documented all six) for
register facts and notes the per-part differences (flash/RAM/pin count)
where they matter.

## 1. What this is

The PIC16F193X HAL is the repo's third family, implementing the shared
`pic8-common` contract with bodies for the Enhanced Mid-range core. It
is a new family, not a variant of `pic16f87xa-hal`, because the
addressing model (BSR), interrupt entry (auto context save), and I/O
model (LAT/ANSEL) differ (DS41364B §2.2, §4.0, §6.0).

## 2. The big picture

Single interrupt vector at 0x0004, no priority. Hardware saves
W/STATUS/BSR/FSR0/FSR1/PCLATH to shadow registers on entry and restores
them on RETFIE (DS41364B §4.1), so ISRs need no manual push/pop. The
`pic8_dispatch_all_irqs` fan-out (one call per peripheral handler) is
shared by the target vector and the host sim IRQ callback, same shape
as the other families.

## 3. Quick start

Host simulation (no hardware, no DFP):

```sh
cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure
./build/example_blink
```

Real target (needs the PIC12-16F1xxx DFP):

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f193x-mplabx MCU=16F1937
```

## 4. Build systems

Host build: `CMakeLists.txt`, a thin caller of
`pic8-common/cmake/pic8_family.cmake`. Include path puts `include/host`
first (memory-backed SFR), links the `_sim.c` halves. Target build:
`mcu/pic16f193x-mplabx/Makefile`, a thin caller of
`pic8-common/mk/pic8_family.mk`, puts `include/target` first
(volatile-deref SFR), links the `_target.c` halves. The split is by
include path + linked file, never `#ifdef`.

## 5. Datasheet citations

Every register address, bit, and POR value in the SFR map
(`include/pic16f193x_sfr.h`) cites its DS41364B section or register
table. Peripheral headers cite their own section. The citation form is
`DS41364B §N.M` or `DS41364B Register N-N`, matching the repo's
convention.

## 6. What the simulator models

Foundation: Timer0 (OPTION_REG prescaler, TMR0 increment, TMR0IF on
overflow), the GPIO pin-level model (writes to LATx, reads from PORTx
refreshed per tick from LATx + driven inputs), and the PORTB
interrupt-on-change (IOCBP/IOCBN edge detection, IOCBF, IOCIF). Other
peripherals are added by their phases.

## 7. Interrupts

23 sources (DS41364B §4.0, Figure 4-1/4-2): INTCON holds IOC/INT/TMR0;
PIR1/PIE1 holds TMR1, TMR2, CCP1, SSP, USART_TX, USART_RX, ADC, TMR1G;
PIR2/PIE2 holds CCP2, LCD, BCL, EEPROM, CMP1, CMP2, OSF; PIR3/PIE3
holds TMR4, TMR6, CCP3, CCP4, CCP5. `HAL_IRQ_SetPriority` is a no-op
(single vector, no priority), the no-op half of the shared contract;
PIC18 implements it for real. See `../pic8-common/MANUAL.md` §6 for the
shared model; the family-specific half is the `PIC16F193X_IRQn` enum and
the 3-PIE-bank table in `src/core/pic16f193x_irq.c`.

## 8. Core: WDT, Sleep, BOR/POR

`HAL_WDT_Refresh` = `clrwdt` (target) / no-op (host). `HAL_Sleep_Enter` =
`sleep` (target) / no-op (host). BOR/POR status reads PCON<0>/<1>
(DS41364B §3.0, Register 3-3). The watchdog is enabled by the WDTEN
config bits or by SWDTEN in WDTCON (DS41364B §24.1).

## 9. GPIO

`HAL_GPIO_Init` programs TRISx (direction) + ANSELx (analog/digital) +
LATx (output start low). Writes go to LATx (`HAL_GPIO_WritePin` /
`TogglePin` / `WritePort`), reads come from PORTx (`ReadPin` / `ReadPort`)
(DS41364B §6.0). PORTB weak pull-ups are per-pin via WPUB, gated by the
global WPUEN (OPTION_REG<7>, active-low). The PORTB interrupt-on-change
is per-pin via IOCBP (positive edge) / IOCBN (negative edge), flag in
IOCBF, summarized by IOCIF (DS41364B §7.0). PORTD/PORTE are 40/44-pin
only (1934/1937/1939); PORTE is 4-bit.

## 10. Timer0

`HAL_TIMER0_*` configures OPTION_REG (T0CS/T0SE/PSA/PS<2:0>) and TMR0
(DS41364B §15.0). The prescaler is shared with the WDT; PSA=1 assigns it
to the WDT and Timer0 runs 1:1. Writing TMR0 clears the prescaler. The
overflow flag/enable are INTCON<TMR0IF>/<TMR0IE>.

## 11. The SFR layer

`include/pic16f193x_sfr.h` defines `PIC_REG_*` addresses, `PIC_*_BIT`
masks, and `PIC_*_POR_VALUE` reset values, all DS41364B-cited. The
platform pair (`include/host` + `include/target`, same name, include-path
selected) defines `PIC8_REG8` / `pic8_sfr_read8` / `pic8_sfr_write8` /
`PIC8_SFR_PTR` and the per-PIE-bank `PIC8_PIE_ENABLE_BIT` /
`PIC8_PIE_DISABLE_BIT` macros (`pir_index` 0/1/2 for PIE1/2/3).

## 12. Device selection

`include/pic16f193x.h` selects exactly one of 1933/1934/1936/1937/1938/
1939 via a `-D` define, default 1937, and sets per-device capability
macros (`PIC16F193X_FAMILY_HAS_PORTD`/`_PORTE` on 40/44-pin parts, plus
flash/RAM/EEPROM/ADC sizes). `PIC8_FAMILY_RAM_BYTES` is the neutral
alias consumers use.

## 13. The examples

`example_blink.c`: Timer0 overflow drives an ISR that toggles RB0; the
canonical dual-build smoke test, its header documents the expected
register image for the §4 gate. `example_gpio.c`: host-only GPIO + IOC
smoke test driving the sim directly.

## 14. Known gaps and gotchas

- The real-target build and the §4 `mdb` gate are pending the
  `Microchip.PIC12-16F1xxx_DFP`. No peripheral counts as done until that
  gate passes.
- Silicon errata DS80000479 (1934/1936/1937): ADC may not complete at
  FOSC > 8 MHz; ECCP 0%-duty direction-change and port-steering issues;
  Timer1 gate toggle issues; EUSART auto-baud SPBRG bug; MSSP SPI master
  BF/SSPIF set half SCK early (CKE=0). Honored in the relevant peripheral
  phases when built.
- The target platform's plain-C PIE RMW form is unverified on this core
  (the classic-PIC16 equivalent failed under XC8 v4.00); the §4 codegen
  probe clears or replaces it before the real-target build is trusted.

## 15. Appendix: datasheet section index

DS41364B §2.2 data memory, §3 resets, §4 interrupts, §6 I/O ports, §7
interrupt-on-change, §8 oscillator, §10 device config, §11 ADC, §12
comparator, §13 DAC, §14 FVR, §15 Timer0, §16 Timer1, §17 Timer2/4/6,
§18 capacitive sensing, §19 CCP/ECCP, §20 EUSART, §21 LCD, §22 MSSP,
§23 EEPROM/Flash, §24 Sleep, §26 instruction set.
