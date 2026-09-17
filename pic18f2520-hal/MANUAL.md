# PIC18F2520 family HAL, Manual

Family-agnostic conventions, the handle pattern, status codes, the harness,
and the host-sim/target build-time split: see `epic-common/MANUAL.md`. This
manual covers only what is genuinely specific to the **PIC18F2520**, verified
against the Microchip datasheet **DS39631E** (PIC18F2420/2520/4420/4520).

**Complete family:** platform, SFR map, GPIO, Timer0-3, ECCP/CCP, MSSP,
EUSART, ADC, comparator, data EEPROM, interrupt core, WDT/Sleep, and the
harness are implemented and verified under host sim and real `mdb`
(epic-hal#174-177). This family has no USB and no SPP (DS39631E Table 1-1),
so it never will; do not expect them here.

---

## Contents

1. [What this is](#1-what-this-is)
2. [The big picture](#2-the-big-picture)
3. [Quick start](#3-quick-start)
4. [Build systems](#4-build-systems)
5. [The host simulation backend](#5-the-host-simulation-backend)
6. [Interrupts](#6-interrupts)
7. [Core: WDT, Sleep, BOR/POR](#7-core-wdt-sleep-borpor)
8. [GPIO](#8-gpio)
9. [Timer0](#9-timer0)
10. [Timer1](#10-timer1)
11. [Timer2](#11-timer2)
12. [Timer3](#12-timer3)
13. [ECCP1 / CCP2, Capture, Compare, PWM](#13-eccp1--ccp2-capture-compare-pwm)
14. [MSSP, SPI and I2C](#14-mssp-spi-and-i2c)
15. [EUSART](#15-eusart)
16. [ADC](#16-adc)
17. [Comparator](#17-comparator)
18. [Data EEPROM](#18-data-eeprom)
19. [The SFR layer](#19-the-sfr-layer)
20. [Device selection](#20-device-selection)
21. [The examples](#21-the-examples)
22. [Known gaps and gotchas](#22-known-gaps-and-gotchas)
23. [Appendix: datasheet section index](#23-appendix-datasheet-section-index)

---

## 1. What this is

One part, DS39631E:

| Part      | Pins | Flash | RAM    | EEPROM | I/O | ADC ch | CCP/ECCP |
|-----------|------|-------|--------|--------|-----|--------|----------|
| 18F2520   | 28   | 32 KB | 1520 B | 256 B  | 24* | 10     | 2/0      |

\* 24 digital I/O (PORTA/B/C); Table 1-1 counts 25 by including the input-only RE3/MCLR pin, which this HAL excludes (no LAT/TRIS for it).

28-pin (PDIP/SOIC/SSOP): PORTA/B/C plus a single RE3/MCLR input (no PORTD,
no LATE/TRISE registers), 10-channel A/D, ECCP1 + CCP2, MSSP (SPI + I²C),
EUSART, two comparators, and 256 B data EEPROM. It has the dual-priority
interrupt scheme (vectors at 0008h high, 0018h low; DS39631E §9.0). There is
**no USB** and **no SPP** on this part.

## 2. The big picture

```
pic18f2520-hal/
├── include/
│   ├── pic18f2520_hal.h          family header: device select, SFR mapping;
│   │                              pulls shared status codes from epic-common.
│   │                              Named _hal, not pic18f2520.h, to avoid
│   │                              shadowing the DFP device header (see §22).
│   ├── pic18f2520_sfr.h          SFR address map + bit names (1:1 DS39631E)
│   ├── pic18f2520_sim.h          simulation backend public API
│   ├── epic_hal.h                 family-neutral top-level include
│   ├── host/pic18_platform.h     SFRs -> memory array, weak attribute
│   ├── target/pic18_platform.h   SFRs -> volatile deref, no weak
│   ├── core/
│   │   ├── pic18_irq.h                 PIC18_IRQn enum + EPIC_IRQ_* backend
│   │   ├── hal_irq.h                   family-neutral pointer to pic18_irq.h
│   │   ├── pic18f2520_wdt_sleep.h       WDT/Sleep/BOR/POR helpers (RCON-based)
│   │   └── hal_wdt_sleep.h              family-neutral pointer
│   └── peripherals/              one .h per peripheral, Cube-style, plus a
│                                  hal_<ppp>.h family-neutral pointer
├── src/
│   ├── core/pic18_irq.c          interrupt backend implementation
│   ├── core/pic18_irq_dispatch.c shared IRQ fan-out (foundation subset)
│   ├── core/pic18f2520_wdt_sleep.c BOR/POR status helpers
│   ├── peripherals/             implementations of peripherals/ headers
│   ├── target/                   isr_vector + wdt_sleep_target (XC8 only)
│   ├── epiccc/                   vector/dispatch/WDT for the epiccc build
│   ├── sim/                      harness_sim, sim backend (host build only)
│   └── mdb/                      harness_mdb (MPLAB SIM gate, RA0 marker)
├── tests/                        example_blink, example_irq, example_smoke
├── mcu/pic18f2520-mplabx/        XC8 real-target build notes
└── CMakeLists.txt                host build (thin caller of epic-common/cmake)
```

## 3. Quick start

Host sim:

```sh
cmake -B build -S pic18f2520-hal && cmake --build build
./build/example_smoke          # "smoke: 10 ticks, device PIC18F2520"
./build/example_blink          # RB0 toggled 9 times
```

Real target (XC8, manifest-driven):

```sh
python3 scripts/epic_build.py build --module pic18f2520-hal --mcu 18F2520 --run
```

## 4. Build systems

The host/target split is done at build time, not with `#ifdef`: the CMake
host build puts `include/host` ahead of `include` on the include path, so
`pic18_platform.h` resolves to the memory-backed-SFR version and links the
host harness; the XC8 build puts `include/target` first and links the family-
blind target harness. The manifest (`epic-common/manifest/modules.toml`)
holds both the family table and the `pic18f2520-hal` pseudo-module that CI
builds. See `epic-common/MANUAL.md` and `epic-common/manifest/README.md`.

## 5. The host simulation backend

`pic18f2520_sim.h` exposes `pic18_sim_reset`, `pic18_sim_step`, the GPIO
drive/read hooks, and the IRQ callback registration. The register file is a
flat 4096-byte array indexed by the physical 12-bit SFR address; every SFR
the foundation touches is in the Access Bank (0xF60-0xFFF), so no BSR
translation is needed. Timer0 stepping (8/16-bit, prescaler, overflow ->
TMR0IF + IRQ callback) is modeled; other timers and peripherals join as
their drivers land.

## 6. Interrupts

Two-vector priority scheme, vectors at 0008h high and 0018h low
(DS39631E §9.0). `PIC18_IRQn` enumerates the sources; `EPIC_IRQ_Enable` /
`DisableSrc` / `ClearFlag` / `GetFlag` / `SetPriority` operate on
INTCON / INTCON2 / INTCON3 / PIE1 / PIR1 / IPR1 / PIE2 / PIR2 / IPR2.
`EPIC_IRQ_Restore(1)` is the drop-in for PIC16's `GIE = 1`; it also sets
IPEN (RCON<7>) so the priority scheme is active. This part has no SPP
source, so that IRQ enum entry is absent (compare the 18F2455 family,
whose enum carries PIC18_IRQ_SPP for its 40/44-pin parts).

The IRQ backend has its own dedicated smoke test (`tests/example_irq.c`),
the step docs/adding-a-device.md §5.5 mandates before any peripheral builds
on it: it enables the Timer0 interrupt, confirms the enable bit reads back
set, lets an overflow fire the ISR, and reports.

## 7. Core: WDT, Sleep, BOR/POR

`EPIC_WDT_Refresh` (`clrwdt` on target), `EPIC_Sleep_Enter` (`sleep` on
target), and RCON-based `EPIC_BOR_GetStatus` / `EPIC_BOR_ClearFlag` /
`EPIC_POR_GetStatus` / `EPIC_POR_ClearFlag` (DS39631E Register 4-1).
See `hal_wdt_sleep.h`.

## 8. GPIO

PORTA/B/C only (DS39631E §10.0). Writes go through LATx, reads through
PORTx, direction in TRISx. PORTB pull-ups via INTCON2<RBPU>. There is no
PORTD and no GPIOE on this 28-pin part (the lone RE3/MCLR input has no
LAT/TRIS registers). RB<7:4> change interrupts are supported through
`EPIC_GPIO_RegisterChangeCallback` and the weak `RB_IRQHandler`.

## 9. Timer0

8/16-bit timer/counter with its own prescaler (DS39631E §11.0), controlled
by T0CON; default 8-bit to stay a PIC16 drop-in. Overflow -> TMR0IF +
weak `TIMER0_IRQHandler`.

## 10. Timer1

*DS39631E §12.0, Register 12-1 (T1CON 0xFCD, identical to 4550).*

16-bit timer/counter. T1CON adds RD16 (bit 7, set by this driver for atomic
16-bit access) and read-only T1RUN (ignored). Prescaler 1:1/2/4/8.
Overflow sets PIR1<TMR1IF>. `example_timer1`: internal/1:1, 2 overflows
per 150k cycles on host; mdb proves TMR1 counts + TMR1IF on hardware.

## 11. Timer2

*DS39631E §13.0, Register 13-1 (T2CON 0xFCA, identical to 4550).*

8-bit timer with PR2 period + postscaler (1:1..1:16). Match (TMR2==PR2)
resets TMR2 and sets PIR1<TMR2IF> after the postscaler. Drives CCP PWM.
`example_timer2`: PR2=9, 10 matches per 100 cycles on host; mdb proves
TMR2/PR2/TMR2IF on hardware.

## 12. Timer3

*DS39631E §12.0, Register 12-1 (T3CON 0xFB1, identical to 4550).*

Second 16-bit timer alongside Timer1. Shares T1OSC (no oscillator field).
T3CCP2:T3CCP1 select Timer1 vs Timer3 for CCP (reset default Timer1; the
CCP driver leaves them). Overflow sets PIR2<TMR3IF>. `example_timer3`:
internal/1:1, 2 overflows per 150k cycles; mdb proves TMR3IF.

## 13. ECCP1 / CCP2, Capture, Compare, PWM

*DS39631E §15.0. ECCP1 + plain CCP2, same instance split as 4550.*

**Reduced ECCP1:** the 2520 has no P1M bridge modes, no PDC dead-band, no
PSSBD (P1B/P1D do not exist on this 28-pin part; confirmed vs DFP, zero
PSSBD symbols vs 42 on 4550). The driver omits bridge/dead-band and keeps
capture/compare/PWM + auto-shutdown (ECCPAS/PSSAC/PRSEN only). Capture and
compare use Timer1/3; PWM uses Timer2. `example_ccp`: compare-toggle mode
programming + time base; mdb proves CCP1IF on Timer1==CCPR1 hardware match.

## 14. MSSP, SPI and I2C

*DS39631E §17.0 (SSPCON1 0xFC6, identical to 4550).*

Register-level SPI master/slave + I2C master/slave. I2C Start/Stop/ACK
state machine left to caller; SPI completes on SSPBUF write, poll BF.
`example_ssp`: SPI-master programming; mdb proves SSPCON1/SSPSTAT/SSPBUF
on hardware (SIM does not model SPI shifting, same as 4550).

## 15. EUSART

*DS39631E §18.0 (same shape as 4550).*

Async + sync with 16-bit BRG (BRG16), auto-baud, 9-bit address-detect.
TXEN/CREN always enabled (polled TX/RX work with no callback; TXIE/RCIE
still gate interrupts). TXSTA POR is 0x02 (TRMT). `example_usart`: 9600
baud async TX; mdb UART captures 0x55 bytes out on SIM (actual TX on wire).

## 16. ADC

*DS39631E §19.0 (10-bit, AN0-4 + AN8-12 on 28-pin, same as 4550).*

AN5-7 exist only on 40/44-pin parts. GO/DONE starts conversion; ADIF sets
on completion. `example_adc`: AN0 programming; mdb proves GO clears +
ADIF sets on hardware.

## 17. Comparator

*DS39631E §20.0 (two comparators, same as 4550).*

8 modes via CMCON; C1OUT/C2OUT read-only outputs. `example_comp`: mode
programming; mdb proves CMCON on hardware.

## 18. Data EEPROM

*DS39631E §7.0 (256 bytes, EEADR alone, same as 4550).*

Unlock 0x55/0xAA to EECON2, strobe WR, poll EEIF (or interrupt via EEIE;
dispatch leaves EEIF untouched when EEIE is off so pollers own it).
`example_eeprom`: write/read round-trip; mdb proves EEADR/EECON1 programming
(SIM does not model cell writes).

## 19. The SFR layer

Every SFR access names a compile-time-constant `PIC_REG_*` token through
`epic_sfr_read8` / `EPIC_REG8` (see `pic18_platform.h`). This is load-bearing:
on PIC18, a runtime SFR address compiles to the program-memory table
mechanism and silently writes nowhere (see §22).

## 20. Device selection

`PIC18F2520` (DS39631E) and `PIC18F2220` (DS39599) are the family's
variants. The build driver emits `-DPIC18F2520` or `-DPIC18F2220`;
`pic18f2520_hal.h` defaults to `PIC18F2520` when nothing is defined. The
2220 has no ECCP1 (standard CCP1CON only, no ECCP1AS/PWM1CON, DS39599
§16.0) and no 16-bit BRG (no BAUDCON register at all, 8-bit SPBRG only,
DS39599 §17.0); both are gated by the `PIC18F2520_FAMILY_HAS_ECCP1` /
`PIC18F2520_FAMILY_HAS_BRG16` capability macros in §13/§15's drivers.

## 21. The examples

- `example_smoke`: bare harness contract (family-blind).
- `example_blink`: Timer0 + GPIO + interrupt, RB0 toggle.
- `example_irq`: the IRQ-core smoke test described in §6.
- `example_timer1/2/3`: per-timer overflow/match counting (§10-12).
- `example_ccp`: CCP1 compare-mode programming + time base (§13).
- `example_ssp`: MSSP SPI-master programming (§14).
- `example_usart`: EUSART async TX programming + byte out (§15).
- `example_adc/comp/eeprom`: analog programming + round-trips (§16-18).

## 22. Known gaps and gotchas

- **Umbrella header name.** The family header is `pic18f2520_hal.h`, not
  `pic18f2520.h`: the DFP proc header for this part is literally
  `pic18f2520.h`, and an umbrella named the same would shadow it through
  `-I` at link time and strip the compiler's auto-included SFRs (the
  PIC16F628A lesson, epic-hal#137).
- **Never pass an SFR as a runtime value on PIC18.** A runtime-computed SFR
  address compiles to the program-memory table mechanism (`tblrd`/`tblwt`),
  silently writing nowhere. Every SFR access is a literal `PIC_REG_*` token;
  branch on the instance before touching SFRs, never dispatch on a runtime
  value and then access a register.
- **No USB / no SPP.** Do not expect SPP or USB config fields
  (usbdiv/cpudiv/plldiv/vregen) on this part.
- **Reduced ECCP1.** No P1M/PDC/PSSBD hardware (28-pin); the driver omits
  bridge/dead-band. Do not port 4550 full-bridge code unchanged.
- **EUSART TXEN/CREN always on.** Polled TX/RX work with no callback;
  TXIE/RCIE gate interrupts. TXSTA POR is 0x02 (TRMT), not 0x00.
- **SIM limits.** MPLAB SIM does not model SPI shifting, EEPROM cell
  writes, or CCP output pins; those gates prove register programming +
  flags, matching the 4550's accepted level.
- **Config words.** The 2520 drops the 2455 family's USB config fields;
  see `mcu/pic18f2520-mplabx/README.md` for the default set.
- **Gates stay `MODE=gpio`.** The EUSART exists, but the CI blink gate
  reports on the RA0 latch by design (no UART in the blink path).

## 23. Appendix: datasheet section index

| Section | Topic        | Where covered here                |
|---------|--------------|-----------------------------------|
| §5.0    | SFR map      | `pic18f2520_sfr.h`                 |
| §7.0    | Data EEPROM  | `peripherals/pic18f2520_eeprom.h` |
| §9.0    | Interrupts   | `core/pic18_irq.h` / `.c`, §6      |
| §10.0   | GPIO ports   | `peripherals/pic18f2520_gpio.h`   |
| §11.0   | Timer0       | `peripherals/pic18f2520_timer0.h` |
| §12.0   | Timer1/3     | `peripherals/pic18f2520_timer1/3.h` |
| §13.0   | Timer2       | `peripherals/pic18f2520_timer2.h` |
| §15.0   | ECCP/CCP     | `peripherals/pic18f2520_ccp.h`    |
| §17.0   | MSSP         | `peripherals/pic18f2520_ssp.h`    |
| §18.0   | EUSART       | `peripherals/pic18f2520_usart.h`  |
| §19.0   | ADC          | `peripherals/pic18f2520_adc.h`    |
| §20.0   | Comparator   | `peripherals/pic18f2520_comp.h`   |
| §22.1   | Config words | `mcu/pic18f2520-mplabx/README.md` |
| Table 1-1 | Device features | §1                            |
