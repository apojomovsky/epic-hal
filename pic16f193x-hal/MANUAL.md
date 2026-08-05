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

## 11. Timer1 (DS41364B §16.0)

The 16-bit timer/counter. Clock source, prescaler, and on bit are in
T1CON (DS41364B Register 16-1). The flag is `PIR1<TMR1IF>`,
the enable is `PIE1<TMR1IE>`. The 16-bit counter lives in
TMR1H:TMR1L.

### Atomic read

DS41364B §16.4.1: the 16-bit counter can return inconsistent
values across the two reads. Use `HAL_TIMER1_ReadCounter()` rather
than reading TMR1H and TMR1L directly. The driver uses the standard
high-low-high retry idiom (DS41364B §16.4.1).

### Atomic write

DS41364B §16.8: writing TMR1H clears the prescaler. Write the high
byte first.

### Register layout

| Register | Address | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|---|---|---|---|---|---|---|---|---|---|
| T1CON | 0x18 | TMR1CS1 | TMR1CS0 | T1CKPS1 | T1CKPS0 | T1OSCEN | T1SYNC | (unused, 0) | TMR1ON |

T1CON POR value: `0x00` (DS41364B Register 16-1 POR column).

TMR1CS<1:0> (bits 7:6) selects the clock source: `00` = FOSC/4
(internal, the only encoding this phase uses), `01` = FOSC, `10` =
T1CKI pin or T1OSC (per T1OSCEN), `11` = CAPOSC. T1CKPS<1:0> selects
the prescaler ratio (1:1, 1:2, 1:4, 1:8). T1OSCEN enables the
dedicated Timer1 oscillator circuit; T1SYNC controls external-clock
synchronization when TMR1CS<1:0> = 1X, ignored otherwise. Both are
left at 0 this phase (see "Not in this phase" below). TMR1ON enables
the timer. `HAL_TIMER1_Init`/`HAL_TIMER1_Start` return `HAL_INVALID`
for any `ClockSource` other than `TIMER1_CLOCK_INTERNAL`.

### Driver API

`HAL_TIMER1_Init`, `HAL_TIMER1_DeInit`, `HAL_TIMER1_Start`,
`HAL_TIMER1_Stop`, `HAL_TIMER1_ReadCounter`, `HAL_TIMER1_WriteCounter`,
`HAL_TIMER1_PrescalerToRatio`. Weak `TIMER1_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_timer1.c` for the canonical
Timer1 ISR + main loop + bounded sim run.

### Not in this phase

- T1GCON (gate control, DS41364B §16.6): Timer1.1 spec.
- External clock + T1OSC (DS41364B §16.5): Timer1.1 spec.

## 12. Timer2 / Timer4 / Timer6 (DS41364B §17.0)

Three instances of the same 8-bit timer: TMRx counts 0..PRx and resets
to 0 on the cycle it would exceed PRx (never reaches PRx+1), unlike
Timer0/Timer1's raw free-running overflow. TMRxIF fires once every
prescaler x (PRx+1) x postscaler cycles: the PR match happens every
prescaler x (PRx+1) cycles, the postscaler divides that further before
setting the flag. One driver, `HAL_TIMER246_*`, covers all three via a
`TIMER246_InstanceTypeDef` selector (mirrors `pic18fxx5x_ccp.h`'s
`CCP_InstanceTypeDef` convention).

### Register layout

Identical for T2CON/T4CON/T6CON (DS41364B §17.0):

| Register | Address | Bit 7 | Bits 6:3 | Bit 2 | Bits 1:0 |
|---|---|---|---|---|---|
| T2CON | 0x1C | unimplemented | T2OUTPS<3:0> | TMR2ON | T2CKPS<1:0> |
| T4CON | 0x417 | unimplemented | T4OUTPS<3:0> | TMR4ON | T4CKPS<1:0> |
| T6CON | 0x41E | unimplemented | T6OUTPS<3:0> | TMR6ON | T6CKPS<1:0> |

POR value: `0x00` for all three. PRx POR value: `0xFF` for all three
(TMR2/TMR4/TMR6 POR: `0x00`).

Prescaler `T*CKPS<1:0>`: `00`=1:1, `01`=1:4, `1x`=1:16 (both `10` and
`11` give 1:16). Postscaler `T*OUTPS<3:0>`: value N gives 1:(N+1),
linear, all 16 encodings distinct.

TMR2/PR2/T2CON live in bank 0 (0x1A-0x1C). TMR4/PR4/T4CON and
TMR6/PR6/T6CON live in bank 8 (0x415-0x417, 0x41C-0x41E), a bank not
documented in this repo's `docs/pic16f193x-plan.md` §2 bank-map table
before this peripheral landed (that table only covered banks 0-7; see
`docs/superpowers/plans/2026-08-04-pic16f193x-timer246.md`'s "Known
documentation gap" section for the full account).

### Driver API

`HAL_TIMER246_Init`, `HAL_TIMER246_DeInit`, `HAL_TIMER246_Start`,
`HAL_TIMER246_Stop`, `HAL_TIMER246_ReadCounter`,
`HAL_TIMER246_WriteCounter`, `HAL_TIMER246_ReadPeriod`,
`HAL_TIMER246_WritePeriod`, `HAL_TIMER246_PrescalerToRatio`,
`HAL_TIMER246_PostscalerToRatio`. Each takes a
`TIMER246_InstanceTypeDef` (or the handle carries it). Weak
`TIMER2_IRQHandler`/`TIMER4_IRQHandler`/`TIMER6_IRQHandler`, one per
instance. Every SFR access inside the driver branches on the instance
before touching any register, so each branch's own access is a literal
`PIC_REG_*` token (mirrors `pic18fxx5x_ccp.c`'s `CCP_WRITE_*`/
`CCP_READ_*` shape, `docs/adding-a-device.md` §4.8's proven pattern).

### Errata

DS80000479 does not list any Timer2/4/6-specific silicon issue (it
covers ADC, ECCP, Timer1 gate, EUSART, MSSP). No workaround needed.

### Example

See `pic16f193x-hal/tests/example_timer246.c`: all three instances run
at once with different prescaler/postscaler/period values, each
overflow ISR toggles a distinct RC pin (RC0/RC1/RC2).

## 13. CCP1 / CCP2 (DS41364B §15.0)

Both instances are Enhanced CCP on this device (unlike PIC18 where
only CCP1 is). One driver, `HAL_CCP_*`, covers both via a
`CCP_InstanceTypeDef` selector (mirrors `pic18fxx5x_ccp.h`'s
convention). This phase covers capture and compare modes only; PWM
(enhanced output steering, dead-band, auto-shutdown) is deferred and
rejected by `HAL_CCP_Init` with `HAL_INVALID` for `CCP_MODE_PWM`,
mirroring Timer1's `TIMER1_CLOCK_EXTERNAL` rejection precedent.

### Register layout

CCPxCON (DS41364B Register 15-1/15-2, bank 5):

| Register | Address | Bits 7:6 | Bits 5:4 | Bits 3:0 |
|---|---|---|---|---|
| CCP1CON | 0x293 | P1M (PWM-only) | DC1B (PWM duty LSBs) | CCP1M mode |
| CCP2CON | 0x29A | P2M (PWM-only) | DC2B (PWM duty LSBs) | CCP2M mode |

CCPxM mode select (bits 3:0): `0000`=off, `0100`-`0111`=capture
(falling/rising/4th/16th edge), `1000`-`1011`=compare
(set/clear/software-int/special-event), `11xx`=PWM (rejected this phase).

CCPRxL/H (16-bit compare/capture register): CCPR1L=0x291, CCPR1H=0x292,
CCPR2L=0x298, CCPR2H=0x299.

### Driver API

`HAL_CCP_Init`, `HAL_CCP_DeInit`, `HAL_CCP_SetCompare`,
`HAL_CCP_GetCapture`. Weak `CCP1_IRQHandler`/`CCP2_IRQHandler`, one per
instance. Every SFR access branches on the instance before touching any
register (literal `PIC_REG_*` token per branch).

### Errata

DS80000479 flags "ECCP 0%-duty direction-change and port-steering
issues" for the deferred PWM modes, not for this phase's
capture/compare scope. The errata applies to whoever picks up PWM.

### Example

See `pic16f193x-hal/tests/example_ccp.c`: both instances in
compare-set mode with distinct compare values, register-state
verification (no ISR, no callback, pure register readback).

## 14. The SFR layer

`include/pic16f193x_sfr.h` defines `PIC_REG_*` addresses, `PIC_*_BIT`
masks, and `PIC_*_POR_VALUE` reset values, all DS41364B-cited. The
platform pair (`include/host` + `include/target`, same name, include-path
selected) defines `PIC8_REG8` / `pic8_sfr_read8` / `pic8_sfr_write8` /
`PIC8_SFR_PTR` and the per-PIE-bank `PIC8_PIE_ENABLE_BIT` /
`PIC8_PIE_DISABLE_BIT` macros (`pir_index` 0/1/2 for PIE1/2/3).

## 15. Device selection

`include/pic16f193x.h` selects exactly one of 1933/1934/1936/1937/1938/
1939 via a `-D` define, default 1937, and sets per-device capability
macros (`PIC16F193X_FAMILY_HAS_PORTD`/`_PORTE` on 40/44-pin parts, plus
flash/RAM/EEPROM/ADC sizes). `PIC8_FAMILY_RAM_BYTES` is the neutral
alias consumers use.

## 16. The examples

`example_blink.c`: Timer0 overflow drives an ISR that toggles RB0; the
canonical dual-build smoke test, its header documents the expected
register image for the §4 gate. `example_gpio.c`: host-only GPIO + IOC
smoke test driving the sim directly. `example_timer1.c`: Timer1
overflow drives an ISR that toggles RB0, the §4 gate's `HARNESS=sim
MODE=gpio` payload (its header documents the expected register image).

## 17. Known gaps and gotchas

- The `Microchip.PIC12-16F1xxx_DFP` is installed and the real-target
  XC8 build passes for all six parts. Timer1 has cleared the §4 gate
  (Task 11 fix-round-1): `make mdb-test ... MODE=gpio WAIT_MS=60000`
  produces `PIC8_HARNESS_RESULT: PASS` and the §4 control-register
  readback confirms `PIE1=0x01`. The remaining peripherals (Timer2/4/6,
  CCP/ECCP, EUSART, MSSP, ADC, LCD, comparators, DAC, FVR, EEPROM, etc.)
  still need to clear the §4 gate individually; none of them count as
  done until they do.
- Silicon errata DS80000479 (1934/1936/1937): ADC may not complete at
  FOSC > 8 MHz; ECCP 0%-duty direction-change and port-steering issues;
  Timer1 gate toggle issues; EUSART auto-baud SPBRG bug; MSSP SPI master
  BF/SSPIF set half SCK early (CKE=0). Honored in the relevant peripheral
  phases when built.
- The target platform's PIE1/2/3 RMW uses an inline-asm `movlb 1` +
  `iorwf PIE1,f` / `andwf PIE1,f` shape (in `pic16f193x_platform.h`,
  with a `__at(0x70)` scratch byte in `pic16f193x_isr_vector.c`),
  mirroring `pic16f87xa-hal`'s proven pattern. The plain-C RMW
  that the foundation originally shipped silently failed under XC8
  v3.10 (FSR1H=0 read of address 0x91, which is the linear/GPR
  byte, not PIE1 in bank 1). See `docs/ARCHITECTURE.md` Finding 2
  for the codegen evidence and the fix.

## 18. Appendix: datasheet section index

DS41364B §2.2 data memory, §3 resets, §4 interrupts, §6 I/O ports, §7
interrupt-on-change, §8 oscillator, §10 device config, §11 ADC, §12
comparator, §13 DAC, §14 FVR, §15 Timer0, §16 Timer1, §17 Timer2/4/6,
§18 capacitive sensing, §19 CCP/ECCP, §20 EUSART, §21 LCD, §22 MSSP,
§23 EEPROM/Flash, §24 Sleep, §26 instruction set.
