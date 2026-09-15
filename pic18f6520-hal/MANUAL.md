# PIC18F6520 family HAL, Manual

Family-agnostic conventions, the handle pattern, status codes, the harness,
and the host-sim/target build-time split: see `epic-common/MANUAL.md`. This
manual covers only what is genuinely specific to the **PIC18F6520**, verified
against the Microchip datasheet **DS39609B** (PIC18F6520/6620/8520/8620/
6720/8720).

**Phase 1-3 status (epic-hal#182, #183, #184):** platform layer, SFR map,
GPIO, Timer0-3, CCP1-5, MSSP, EUSART1/2, the interrupt core,
WDT/Sleep/BOR/POR, and the harness are implemented and verified under
host sim and real `mdb`. ADC, the comparator, the data EEPROM, PSP and
LVD land in phase 4 (#185) and are marked **phase N** below; Timer4 has
no driver yet (#183 covered Timer0-3 only). This family has no USB and
no SPP (DS39609B Table 1-1), so it never will; do not expect them
here.

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
9. [Timers: Timer0-4](#9-timers-timer0-4)
10. [CCP1-5](#10-ccp1-5)
11. [MSSP](#11-mssp)
12. [EUSART1/2](#12-eusart12)
13. [The SFR layer](#13-the-sfr-layer)
14. [Device selection](#14-device-selection)
15. [The examples](#15-the-examples)
16. [Known gaps and gotchas](#16-known-gaps-and-gotchas)
17. [Appendix: datasheet section index](#17-appendix-datasheet-section-index)
11. [Device selection](#11-device-selection)
12. [The examples](#12-the-examples)
13. [Known gaps and gotchas](#13-known-gaps-and-gotchas)
14. [Appendix: datasheet section index](#14-appendix-datasheet-section-index)

---

## 1. What this is

One part, DS39609B:

| Part      | Pins | Flash | RAM    | EEPROM | I/O | ADC ch | CCP |
|-----------|------|-------|--------|--------|-----|--------|-----|
| 18F6520   | 64   | 32 KB | 2032 B | 1 KB   | 52  | 12     | 5   |

64-pin TQFP: full PORTA-G I/O (7 ports, DS39609B Table 1-1), 12-channel
10-bit A/D, five CCP modules, MSSP (SPI + I²C), **two** EUSARTs, a Parallel
Slave Port, two comparators with a voltage reference, a Low-Voltage Detect,
and 1 KB data EEPROM (addressed by EEADR + EEADRH). It has the dual-priority
interrupt scheme (vectors at 0008h high, 0018h low; DS39609B §9.0). There
is **no USB** and **no SPP** on this part.

The 6520 is the largest of the three new PIC18 families (the 1320 and 2520
are the others), with the most I/O and the richest peripheral set: five
CCP instances (the 2520 has two), a fourth timer (TMR4), and a second
USART. Every peripheral count above was confirmed against the DFP proc
header `pic18f6520.h`, not assumed from pin count.

## 2. The big picture

```
pic18f6520-hal/
├── include/
│   ├── pic18f6520_hal.h          family header: device select, SFR mapping;
│   │                              pulls shared status codes from epic-common.
│   │                              Named _hal, not pic18f6520.h, to avoid
│   │                              shadowing the DFP device header (see §13).
│   ├── pic18f6520_sfr.h          SFR address map + bit names (1:1 DS39609B)
│   ├── pic18f6520_sim.h          simulation backend public API
│   ├── epic_hal.h                 family-neutral top-level include
│   ├── host/pic18_platform.h     SFRs -> memory array, weak attribute
│   ├── target/pic18_platform.h   SFRs -> volatile deref, no weak
│   ├── core/
│   │   ├── pic18_irq.h                 PIC18_IRQn enum + EPIC_IRQ_* backend
│   │   ├── hal_irq.h                   family-neutral pointer to pic18_irq.h
│   │   ├── pic18f6520_wdt_sleep.h       WDT/Sleep/BOR/POR helpers (RCON-based)
│   │   └── hal_wdt_sleep.h              family-neutral pointer
│   └── peripherals/              one .h per peripheral, Cube-style, plus a
│                                  hal_<ppp>.h family-neutral pointer
├── src/
│   ├── core/pic18_irq.c          interrupt backend implementation
│   ├── core/pic18_irq_dispatch.c shared IRQ fan-out (foundation subset)
│   ├── core/pic18f6520_wdt_sleep.c BOR/POR status helpers
│   ├── peripherals/             implementations of peripherals/ headers
│   ├── target/                   isr_vector + wdt_sleep_target (XC8 only)
│   ├── epiccc/                   vector/dispatch/WDT for the epiccc build
│   ├── sim/                      harness_sim, sim backend (host build only)
│   └── mdb/                      harness_mdb (MPLAB SIM gate, RA0 marker)
├── tests/                        example_blink, example_irq, example_smoke
├── mcu/pic18f6520-mplabx/        XC8 real-target build notes
└── CMakeLists.txt                host build (thin caller of epic-common/cmake)
```

## 3. Quick start

Host sim:

```sh
cmake -B build -S pic18f6520-hal && cmake --build build
./build/example_smoke          # "smoke: 10 ticks, device PIC18F6520"
./build/example_blink          # RB0 toggled 9 times
```

Real target (XC8, manifest-driven):

```sh
python3 scripts/epic_build.py build --module pic18f6520-hal --mcu 18F6520 --run
```

## 4. Build systems

The host/target split is done at build time, not with `#ifdef`: the CMake
host build puts `include/host` ahead of `include` on the include path, so
`pic18_platform.h` resolves to the memory-backed-SFR version and links the
host harness; the XC8 build puts `include/target` first and links the family-
blind target harness. The manifest (`epic-common/manifest/modules.toml`)
holds both the family table and the `pic18f6520-hal` pseudo-module that CI
builds. See `epic-common/MANUAL.md` and `epic-common/manifest/README.md`.

## 5. The host simulation backend

`pic18f6520_sim.h` exposes `pic18_sim_reset`, `pic18_sim_step`, the GPIO
drive/read hooks, and the IRQ callback registration. The register file is a
flat 4096-byte array indexed by the physical 12-bit SFR address; every SFR
the foundation touches is in the Access Bank (0xF60-0xFFF), so no BSR
translation is needed. Timer0 stepping (8/16-bit, prescaler, overflow ->
TMR0IF + IRQ callback) is modeled; other timers and peripherals join as
their drivers land.

## 6. Interrupts

Two-vector priority scheme, vectors at 0008h high and 0018h low
(DS39609B §9.0). `PIC18_IRQn` enumerates the sources; `EPIC_IRQ_Enable` /
`DisableSrc` / `ClearFlag` / `GetFlag` / `SetPriority` operate on
INTCON / INTCON2 / INTCON3 / PIE1-3 / PIR1-3 / IPR1-3. `EPIC_IRQ_Restore(1)`
is the drop-in for PIC16's `GIE = 1`; it also sets IPEN (RCON<7>) so the
priority scheme is active.

The 6520's source list is the richest of the PIC18 families this repo
carries: the 2455/2520 sources (INT0-2, RB, TMR0-3, CCP1-2, SSP, USART,
ADC, COMP, EEPROM) plus **INT3** (INTCON2/3), **TMR4**, **CCP3-5** and a
**second USART** (all in PIR3/PIE3/IPR3), **LVD** (PIR2) and **PSP**
(PIR1). The enum has 25 members; the 2520's had 16. As on the other
families, INT0 alone has no priority bit (always high).

The IRQ backend has its own dedicated smoke test (`tests/example_irq.c`),
the step docs/adding-a-device.md §5.5 mandates before any peripheral builds
on it: it enables the Timer0 interrupt, confirms the enable bit reads back
set, lets an overflow fire the ISR, and reports.

## 7. Core: WDT, Sleep, BOR/POR

`EPIC_WDT_Refresh` (`clrwdt` on target), `EPIC_Sleep_Enter` (`sleep` on
target), and RCON-based `EPIC_BOR_GetStatus` / `EPIC_BOR_ClearFlag` /
`EPIC_POR_GetStatus` / `EPIC_POR_ClearFlag` (DS39609B Register 4-4).
See `hal_wdt_sleep.h`. RCON carries IPEN and the RI/TO/PD/POR/BOR flags;
there is no SBOREN bit on this part (compare the 4550 family's RCON,
Register 9-11).

## 8. GPIO

PORTA-G, all seven ports with PORTx/LATx/TRISx registers (DS39609B
§10.0, Table 1-1). PORTA is 7 bits (RA0-RA6, no RA7) and PORTG is 5
bits (RG0-RG4, no RG5-RG7); the other five ports are full 8-bit. The
unimplemented TRISA<7> and TRISG<7:5> bits read 0 (Table 4-3). Writes
go through LATx, reads through PORTx, direction in TRISx. PORTB
pull-ups via INTCON2<RBPU>. Like every PIC18 port, an output does not
read back on PORTx (the pin input register), only on LATx. RB<7:4>
change interrupts are supported through
`EPIC_GPIO_RegisterChangeCallback` and the weak `RB_IRQHandler`.

## 9. Timers: Timer0-4

Four timer modules on this part (DS39609B Table 1-1; Timer4 is
registered in the SFR map but has no driver yet, out of this ticket's
scope).

**Timer0** (DS39609B §11.0): 8/16-bit timer/counter with its own
prescaler, controlled by T0CON; default 8-bit to stay a PIC16 drop-in.
Overflow -> TMR0IF + weak `TIMER0_IRQHandler`.

**Timer1** (DS39609B §12.0): 16-bit timer/counter. The driver sets
RD16 (T1CON<7>) so the atomic TMR1L-latches-TMR1H read/write idiom
works; bit 6 is unimplemented on this part (no T1RUN, Register 12-1),
unlike the 4550/2520 families. Overflow -> PIR1<TMR1IF> + weak
`TIMER1_IRQHandler`.

**Timer2** (DS39609B §13.0): 8-bit period-match timer with 2-bit
prescaler and 4-bit postscaler (T2CON, 1:1/4/16 and 1:(N+1)). Match
with PR2 resets the counter, advances the postscaler, and fires
PIR1<TMR2IF> when the postscaler wraps.

**Timer3** (DS39609B §14.0): 16-bit timer/counter, RD16-style atomic
access like Timer1; T3CCP1/T3CCP2 (T3CON<3>/<6>) select which CCP
module uses Timer3 as its time base (left at reset, Timer1). Overflow
-> PIR2<TMR3IF> + weak `TIMER3_IRQHandler`.

All four drivers follow the Cube-style handle pattern (per-timer
`*_HANDLE_DEFAULT`, `EPIC_TIMERx_Init` copies the handle and enables
the interrupt when a callback is given, `EPIC_TIMERx_Start` loads the
reload/period and starts counting, `_Stop`/`_DeInit` reverse it).
Per-timer host smokes live in `tests/example_timer1/2/3.c`; each was
gated under real mdb with register readbacks (T1CON=0x81 with
TMR1IF set on 0xFFF0-reload overflow, T2CON=0x04 with PR2=9 and
TMR2IF, T3CON=0x81 with PIR2<TMR3IF>).

## 10. CCP1-5

Five plain CCP modules (CP1CON/CCPR1 at 0xFBD-0xFBF, CCP2 at 0xFBA-0xFBC,
CCP3 at 0xFB7-0xFB9, CCP4 at 0xF73-0xF75, CCP5 at 0xF70-0xF72; DS39609B
§16.0, Register 16-1). All five share the identical plain-CCP layout
(mode bits 3:0, duty LSBs 5:4): this part has no Enhanced-CCP hardware
(no PSTRCON/ECCPAS/PWM1CON in the DFP), unlike the 4550's ECCP1 or the
2520's reduced ECCP1, so there is no auto-shutdown/restart API. One
driver with an instance selector (`CCP_INSTANCE_1..5`) programs
capture/compare/PWM; Capture/Compare use Timer1 or Timer3 (T3CON<
T3CCP2:T3CCP1>, reset default Timer1+Timer2), PWM uses Timer2. Each
branch of the instance selector touches only literal `PIC_REG_*`
tokens before any SFR access.

## 11. MSSP

Single MSSP module (SSPCON1/2, SSPSTAT, SSPADD, SSPBUF at 0xFC5-0xFC9;
DS39609B §17.0), SPI and I2C, same register shape as the 4550 family.
API follows the 2520 driver: `EPIC_SSP_Init` programs the mode/edges,
`EPIC_SSP_WriteByte`/`_ReadByte` transfer, the I2C condition helpers
(`_Start`/`_Stop`/`_RepeatedStart`/`_ReceiveEnable`/`_AcknowledgeEnable`)
drive SSPCON2, and the weak `SSP_IRQHandler` fires `TransferCallback`.

## 12. EUSART1/2

Two identical EUSART modules (EUSART1 registers at 0xFAB-0xFAF, EUSART2
at 0xF6B-0xF6F; DS39609B §18.0). One driver with an instance selector
(`USART_INSTANCE_1/2`). Async + sync master/slave, the 8-bit baud-rate
generator per Table 18-1 (no BRG16/SPBRGH/BAUDCON, see §13), 9-bit data
with address-detect. The weak handlers split per module: `USART_TX/
RX_IRQHandler` for EUSART1 (PIR1<TXIF/RCIF>), `USART2_TX/RX_IRQHandler`
for EUSART2 (PIR3<TX2IF/RC2IF>).

## 13. The SFR layer

Every SFR access names a compile-time-constant `PIC_REG_*` token through
`epic_sfr_read8` / `EPIC_REG8` (see `pic18_platform.h`). This is load-bearing:
on PIC18, a runtime SFR address compiles to the program-memory table
mechanism and silently writes nowhere (see §13).

## 14. Device selection

`PIC18F6520` is the only variant. The build driver emits `-DPIC18F6520`;
`pic18f6520_hal.h` defaults to it when nothing is defined.

## 15. The examples

- `example_smoke`: bare harness contract (family-blind).
- `example_blink`: Timer0 + GPIO + interrupt, RB0 toggle.
- `example_irq`: the IRQ-core smoke test described in §6.
- `example_timer1/2/3`: per-timer host overflow/match smokes (the §4
  gate's host-sim halves; the mdb halves run as register-readback gates).
- `example_ccp`: CCP1/CCP3 compare programming + time-base check.
- `example_ssp`: MSSP SPI-master programming check.
- `example_usart`: EUSART1/2 async TX programming (BRG math) check.

## 16. Known gaps and gotchas

- **Umbrella header name.** The family header is `pic18f6520_hal.h`, not
  `pic18f6520.h`: the DFP proc header for this part is literally
  `pic18f6520.h`, and an umbrella named the same would shadow it through
  `-I` at link time and strip the compiler's auto-included SFRs (the
  PIC16F628A lesson, epic-hal#137).
- **Never pass an SFR as a runtime value on PIC18.** A runtime-computed SFR
  address compiles to the program-memory table mechanism (`tblrd`/`tblwt`),
  silently writing nowhere. Every SFR access is a literal `PIC_REG_*` token;
  branch on the instance before touching SFRs, never dispatch on a runtime
  value and then access a register.
- **No USB / no SPP.** Do not expect SPP or USB config fields
  (usbdiv/cpudiv/plldiv/vregen) on this part.
- **Config words.** The 6520 spells the brown-out and stack-reset fields
  `BOR` / `STVR` (not `BOREN` / `STVREN`) and has no `MCLRE` / `IESO` /
  `FCMEN` / `PBADEN` / `XINST` keys; see
  `mcu/pic18f6520-mplabx/README.md` for the default set.
- **Dual EUSART, eight-bit BRG only.** Both EUSART1 and EUSART2 carry
  SPBRGx as an 8-bit baud generator (X = 0..255, DS39609B Table 18-1):
  no BAUDCON, no SPBRGH, no BRG16/auto-baud fields on this part. The
  `USART_ComputeSPBRG` signature therefore takes no `brg16` argument
  (compare the 4550/2520 families).
- Peripherals not yet ported (ADC, comparator, data EEPROM, PSP, LVD)
  are phase 4 (#185); Timer4 has no driver yet (#183 covered Timer0-3
  only), so its SFRs are in the map but there is no TMR4 driver or
  header yet. The MPLAB SIM gates still report PASS/FAIL on the RA0
  GPIO marker (`MODE=gpio`), the EUSART's mdb gate uses `uartio`
  capture directly (see PR #203's EUSART verification).

## 17. Appendix: datasheet section index

| Section | Topic        | Where covered here                |
|---------|--------------|-----------------------------------|
| §4.0    | SFR map      | `pic18f6520_sfr.h`                 |
| §9.0    | Interrupts   | `core/pic18_irq.h` / `.c`, §6      |
| §10.0   | GPIO ports   | `peripherals/pic18f6520_gpio.h`   |
| §11.0   | Timer0       | `peripherals/pic18f6520_timer0.h` |
| §12.0   | Timer1       | `peripherals/pic18f6520_timer1.h` |
| §13.0   | Timer2       | `peripherals/pic18f6520_timer2.h` |
| §14.0   | Timer3       | `peripherals/pic18f6520_timer3.h` |
| §16.0   | CCP1-5       | `peripherals/pic18f6520_ccp.h`     |
| §17.0   | MSSP         | `peripherals/pic18f6520_ssp.h`     |
| §18.0   | EUSART1/2    | `peripherals/pic18f6520_usart.h`   |
| §23.0   | Config words | `mcu/pic18f6520-mplabx/README.md` |
| Table 1-1 | Device features | §1                            |
