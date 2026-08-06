# PIC16F193X HAL Manual

Per-family register-level reference for the PIC16F193X HAL. Every
register fact here is cited to **DS41364B** (the PIC16F193X/LF193X data
sheet). The family-agnostic conventions, the status codes, the
handle pattern, the harness, and the interrupt model's shared half live
in [`../epic-common/MANUAL.md`](../epic-common/MANUAL.md); this file
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
`epic-common` contract with bodies for the Enhanced Mid-range core. It
is a new family, not a variant of `pic16f87xa-hal`, because the
addressing model (BSR), interrupt entry (auto context save), and I/O
model (LAT/ANSEL) differ (DS41364B §2.2, §4.0, §6.0).

## 2. The big picture

Single interrupt vector at 0x0004, no priority. Hardware saves
W/STATUS/BSR/FSR0/FSR1/PCLATH to shadow registers on entry and restores
them on RETFIE (DS41364B §4.1), so ISRs need no manual push/pop. The
`epic_dispatch_all_irqs` fan-out (one call per peripheral handler) is
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
python3 scripts/epic_build.py build --module epic-pic16f193x-firmware --mcu 16F1937 --run
```

## 4. Build systems

Host build: `CMakeLists.txt`, a thin caller of
`epic-common/cmake/epic_family.cmake`. Include path puts `include/host`
first (memory-backed SFR), links the `_sim.c` halves. Target build:
manifest-driven (`epic-common/manifest/modules.toml`'s
`families.PIC16F193X`), puts `include/target` first (volatile-deref
SFR), links the `_target.c` halves. The split is by include path +
linked file, never `#ifdef`.

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
holds TMR4, TMR6, CCP3, CCP4, CCP5. `EPIC_IRQ_SetPriority` is a no-op
(single vector, no priority), the no-op half of the shared contract;
PIC18 implements it for real. See `../epic-common/MANUAL.md` §6 for the
shared model; the family-specific half is the `PIC16F193X_IRQn` enum and
the 3-PIE-bank table in `src/core/pic16f193x_irq.c`.

## 8. Core: WDT, Sleep, BOR/POR

`EPIC_WDT_Refresh` = `clrwdt` (target) / no-op (host). `EPIC_Sleep_Enter` =
`sleep` (target) / no-op (host). BOR/POR status reads PCON<0>/<1>
(DS41364B §3.0, Register 3-3). The watchdog is enabled by the WDTEN
config bits or by SWDTEN in WDTCON (DS41364B §24.1).

## 9. GPIO

`EPIC_GPIO_Init` programs TRISx (direction) + ANSELx (analog/digital) +
LATx (output start low). Writes go to LATx (`EPIC_GPIO_WritePin` /
`TogglePin` / `WritePort`), reads come from PORTx (`ReadPin` / `ReadPort`)
(DS41364B §6.0). PORTB weak pull-ups are per-pin via WPUB, gated by the
global WPUEN (OPTION_REG<7>, active-low). The PORTB interrupt-on-change
is per-pin via IOCBP (positive edge) / IOCBN (negative edge), flag in
IOCBF, summarized by IOCIF (DS41364B §7.0). PORTD/PORTE are 40/44-pin
only (1934/1937/1939); PORTE is 4-bit.

## 10. Timer0

`EPIC_TIMER0_*` configures OPTION_REG (T0CS/T0SE/PSA/PS<2:0>) and TMR0
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
values across the two reads. Use `EPIC_TIMER1_ReadCounter()` rather
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
the timer. `EPIC_TIMER1_Init`/`EPIC_TIMER1_Start` return `EPIC_INVALID`
for any `ClockSource` other than `TIMER1_CLOCK_INTERNAL`.

### Driver API

`EPIC_TIMER1_Init`, `EPIC_TIMER1_DeInit`, `EPIC_TIMER1_Start`,
`EPIC_TIMER1_Stop`, `EPIC_TIMER1_ReadCounter`, `EPIC_TIMER1_WriteCounter`,
`EPIC_TIMER1_PrescalerToRatio`. Weak `TIMER1_IRQHandler`.

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
setting the flag. One driver, `EPIC_TIMER246_*`, covers all three via a
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

`EPIC_TIMER246_Init`, `EPIC_TIMER246_DeInit`, `EPIC_TIMER246_Start`,
`EPIC_TIMER246_Stop`, `EPIC_TIMER246_ReadCounter`,
`EPIC_TIMER246_WriteCounter`, `EPIC_TIMER246_ReadPeriod`,
`EPIC_TIMER246_WritePeriod`, `EPIC_TIMER246_PrescalerToRatio`,
`EPIC_TIMER246_PostscalerToRatio`. Each takes a
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
only CCP1 is). One driver, `EPIC_CCP_*`, covers both via a
`CCP_InstanceTypeDef` selector (mirrors `pic18fxx5x_ccp.h`'s
convention). This phase covers capture and compare modes only; PWM
(enhanced output steering, dead-band, auto-shutdown) is deferred and
rejected by `EPIC_CCP_Init` with `EPIC_INVALID` for `CCP_MODE_PWM`,
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

`EPIC_CCP_Init`, `EPIC_CCP_DeInit`, `EPIC_CCP_SetCompare`,
`EPIC_CCP_GetCapture`. Weak `CCP1_IRQHandler`/`CCP2_IRQHandler`, one per
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

## 14. EUSART (DS41364B §23.0)

Enhanced USART, async 8-bit mode only this phase. 9-bit addressed
mode, auto-baud detection, and synchronous mode are deferred. Auto-baud
is deferred specifically due to DS80000479's "EUSART auto-baud SPBRG
bug" on this silicon, not just convenience.

### Register layout

| Register | Address | Key bits |
|---|---|---|
| TXSTA | 0x19E | TXEN(5), BRGH(2), TRMT(1, read-only) |
| RCSTA | 0x19D | SPEN(7), CREN(4), OERR(1, read-only) |
| SPBRGL | 0x19B | 8-bit baud divisor (BRG16=0 this phase) |
| SPBRGH | 0x19C | unused (BRG16=0 this phase) |
| BAUDCON | 0x19F | BRG16(3), RCIDL(6, read-only) |

POR values: TXSTA=0x02 (TRMT=1), RCSTA=0x00, BAUDCON=0x40 (RCIDL=1).

Baud rate formula (BRG16=0): `rate = FOSC / (divisor * (SPBRG+1))` where
divisor=64 (BRGH=0) or 16 (BRGH=1). `USART_ComputeSPBRG` computes this.

### Driver API

`EPIC_USART_Init`, `EPIC_USART_DeInit`, `EPIC_USART_Transmit`,
`EPIC_USART_IsTxShiftRegisterEmpty`, `EPIC_USART_Receive`,
`EPIC_USART_HasOverrunError`, `USART_ComputeSPBRG`. Weak
`USART_TX_IRQHandler`/`USART_RX_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_eusart.c`: init at 9600 baud (32MHz
Fosc, BRGH=1), transmit one byte, register-state verification.

## 15. MSSP (SPI Master) (DS41364B §22.0)

MSSP in SPI master mode only this phase. I2C (master/slave) and SPI
slave are deferred (substantially different, larger scope). The default
handle uses CKE=1 (errata-safe); CKE=0 carries a real DS80000479 errata
(BF/SSPIF set half SCK early) but is not forbidden.

### Register layout

| Register | Address | Key bits |
|---|---|---|
| SSPSTAT | 0x214 | BF(0, read-only), CKE(6), SMP(7) |
| SSPCON1 | 0x215 | SSPM(3:0), CKP(4), SSPEN(5), SSPOV(6), WCOL(7) |
| SSPBUF | 0x211 | Read/write data buffer |

SSPM mode select (bits 3:0): `0000`=Fosc/4, `0001`=Fosc/16,
`0010`=Fosc/64, `0011`=Timer2 output (SPI master only this phase).

### Driver API

`EPIC_SSP_Init`, `EPIC_SSP_DeInit`, `EPIC_SSP_WriteByte`, `EPIC_SSP_ReadByte`,
`EPIC_SSP_IsBufferFull`, `EPIC_SSP_HasWriteCollision`,
`EPIC_SSP_ClearWriteCollision`. Weak `SSP_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_mssp.c`: init at Fosc/4, CKE=1,
register-state verification.

## 16. ADC (DS41364B ADC chapter)

10-bit ADC. DS80000479 errata: ADC may not complete at FOSC > 8 MHz.
`ADC_HANDLE_DEFAULT` uses `ADC_CLOCK_FRC` (Fosc-independent) to sidestep
this by default. Callers who pick an Fosc-derived clock are responsible
for keeping the ADC clock period within the datasheet's safe threshold.

### Register layout

| Register | Address | Key bits |
|---|---|---|
| ADCON0 | 0x9D | ADON(0), GO/nDONE(1), CHS(6:2, 5-bit) |
| ADCON1 | 0x9E | ADPREF(1:0), ADNREF(2), ADCS(6:4), ADFM(7) |
| ADRESL/H | 0x9B/0x9C | 10-bit result, justified per ADFM |

ADCS clock select (bits 6:4): `000`=Fosc/2, `001`=Fosc/8, `010`=Fosc/32,
`011`=FRC (errata-safe), `100`=Fosc/4, `101`=Fosc/16, `110`=Fosc/64.

### Driver API

`EPIC_ADC_Init`, `EPIC_ADC_DeInit`, `EPIC_ADC_SelectChannel`,
`EPIC_ADC_Start`, `EPIC_ADC_IsConversionDone`, `EPIC_ADC_Read`. Weak
`ADC_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_adc.c`: init with FRC clock,
register-state verification (ADCON0=0x01, ADCON1=0xB0).

## 17. Comparator (DS41364B §9.0)

Dual comparator (C1/C2). Each instance has its own CMxCON0
(enable/config/output) and CMxCON1 (channel select/edge interrupt),
plus shared read-only CMOUT mirror. C1OUT/C2OUT (CMxCON0 bit 6) are
read-only hardware status bits.

### Register layout

| Register | Address | Key bits |
|---|---|---|
| CM1CON0 | 0x111 | C1ON(7), C1OUT(6, RO), C1OE(5), C1POL(4), C1HYS(1) |
| CM1CON1 | 0x112 | C1PCH(5:4), C1NCH(1:0), C1INTP(7), C1INTN(6) |
| CM2CON0 | 0x113 | C2ON(7), C2OUT(6, RO), C2OE(5), C2POL(4), C2HYS(1) |
| CM2CON1 | 0x114 | C2PCH(5:4), C2NCH(1:0), C2INTP(7), C2INTN(6) |
| CMOUT | 0x115 | MC1OUT(0, RO), MC2OUT(1, RO) |

### Driver API

`EPIC_COMP_Init`, `EPIC_COMP_DeInit`, `EPIC_COMP_ReadOutput`. Weak
`CMP1_IRQHandler`/`CMP2_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_comparator.c`: init both instances,
register-state verification (CM1CON0=0x80, CM2CON0=0x80, with
C1OUT/C2OUT read-only bit 6 masked).

## 18. EEPROM (DS41364B §23.0)

Data EEPROM, 256 bytes on every variant. Data space only this phase
(program-memory self-write deferred). The unlock sequence (0x55/0xAA
to EECON2) is required before WR. The §4 gate confirmed the banked RMW
of EECON1 (bank 3) works correctly with plain-C EPIC_BIT_SET/CLR,
unlike PIE1/2/3 which needed the inline-asm fix (Finding 2).

### Register layout

| Register | Address | Key bits |
|---|---|---|
| EECON1 | 0x195 | RD(0), WR(1), WREN(2), WRERR(3), CFGS(6), EEPGD(7) |
| EECON2 | 0x196 | Write-only unlock (0x55 then 0xAA) |
| EEADRL/H | 0x191/0x192 | Address (H mask 0x7F) |
| EEDATL/H | 0x193/0x194 | Data (H mask 0x3F) |

### Driver API

`EPIC_EEPROM_Init`, `EPIC_EEPROM_DeInit`, `EPIC_EEPROM_ReadByte`,
`EPIC_EEPROM_WriteByte` (blocking, spins on WR),
`EPIC_EEPROM_IsWriteComplete`, `EPIC_EEPROM_HasWriteError`. Weak
`EEPROM_IRQHandler`.

### Example

See `pic16f193x-hal/tests/example_eeprom.c`: sets WREN on EECON1
(bank 3) and verifies the banked RMW landed (the codegen risk the
brief flagged). Full write/read cycle not exercised in the gate
example (the while(WR) spin deadlocks the host sim's polled step
model); the WREN test is the actual codegen verification.

## 19. DAC (DS41364B §13.0)

5-bit DAC. DACCON0 (0x118): DACEN(7), DACLPS(6), DACOE(5), DACPSS(3:2),
DACNSS(0). DACCON1 (0x119): DACR(4:0, 5-bit output value).

`EPIC_DAC_Init`, `EPIC_DAC_DeInit`. See `tests/example_dac.c`.

## 20. FVR (DS41364B §12.0)

Fixed Voltage Reference. FVRCON (0x117): FVREN(7), FVRRDY(6, RO),
TSEN(5), TSRNG(4), CDAFVR(3:2), ADFVR(1:0). FVRRDY is read-only
(hardware sets it when FVR is stable).

`EPIC_FVR_Init`, `EPIC_FVR_DeInit`, `EPIC_FVR_IsReady`. See
`tests/example_fvr.c`.

## 21. SR Latch (DS41364B §11.0)

SR latch. SRCON0 (0x11A): SRLEN(7), SRCLK(6:4), SRQEN(3), SRNQEN(2),
SRPS(1, self-clearing), SRPR(0, self-clearing). SRCON1 (0x11B).

`EPIC_SRLATCH_Enable`, `EPIC_SRLATCH_Disable`. See
`tests/example_srlatch.c`.

## 22. CPS (DS41364B §18.0)

Capacitive Sensing. CPSCON0 (0x1E): CPSON(7), CPSRNG(3:2), CPSOUT(1, RO),
T0XCS(0). CPSCON1 (0x1F): CPSCH(3:0, channel select). CPSOUT is read-only.

`EPIC_CPS_Init`, `EPIC_CPS_DeInit`. See `tests/example_cps.c`.

## 23. The SFR layer

`include/pic16f193x_sfr.h` defines `PIC_REG_*` addresses, `PIC_*_BIT`
masks, and `PIC_*_POR_VALUE` reset values, all DS41364B-cited. The
platform pair (`include/host` + `include/target`, same name, include-path
selected) defines `EPIC_REG8` / `epic_sfr_read8` / `epic_sfr_write8` /
`EPIC_SFR_PTR` and the per-PIE-bank `EPIC_PIE_ENABLE_BIT` /
`EPIC_PIE_DISABLE_BIT` macros (`pir_index` 0/1/2 for PIE1/2/3).

## 18. Device selection

`include/pic16f193x.h` selects exactly one of 1933/1934/1936/1937/1938/
1939 via a `-D` define, default 1937, and sets per-device capability
macros (`PIC16F193X_FAMILY_HAS_PORTD`/`_PORTE` on 40/44-pin parts, plus
flash/RAM/EEPROM/ADC sizes). `EPIC_FAMILY_RAM_BYTES` is the neutral
alias consumers use.

## 19. The examples

`example_blink.c`: Timer0 overflow drives an ISR that toggles RB0; the
canonical dual-build smoke test, its header documents the expected
register image for the §4 gate. `example_gpio.c`: host-only GPIO + IOC
smoke test driving the sim directly. `example_timer1.c`: Timer1
overflow drives an ISR that toggles RB0, the §4 gate's `HARNESS=sim
MODE=gpio` payload (its header documents the expected register image).

## 20. Known gaps and gotchas

- The `Microchip.PIC12-16F1xxx_DFP` is installed and the real-target
  XC8 build passes for all six parts. Timer1 has cleared the §4 gate
  (Task 11 fix-round-1): `make mdb-test ... MODE=gpio WAIT_MS=60000`
  produces `EPIC_HARNESS_RESULT: PASS` and the §4 control-register
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

## 21. Appendix: datasheet section index

DS41364B §2.2 data memory, §3 resets, §4 interrupts, §6 I/O ports, §7
interrupt-on-change, §8 oscillator, §10 device config, §11 ADC, §12
comparator, §13 DAC, §14 FVR, §15 Timer0, §16 Timer1, §17 Timer2/4/6,
§18 capacitive sensing, §19 CCP/ECCP, §20 EUSART, §21 LCD, §22 MSSP,
§23 EEPROM/Flash, §24 Sleep, §26 instruction set.
