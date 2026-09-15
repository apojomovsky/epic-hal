# PIC18F6520 family HAL, Manual

Family-agnostic conventions, the handle pattern, status codes, the harness,
and the host-sim/target build-time split: see `epic-common/MANUAL.md`. This
manual covers only what is genuinely specific to the **PIC18F6520**, verified
against the Microchip datasheet **DS39609B** (PIC18F6520/6620/8520/8620/
6720/8720).

**Foundation status (phase 1):** platform layer, SFR map, GPIO, Timer0, the
interrupt core, WDT/Sleep/BOR/POR, and the harness are implemented and
verified under host sim and real `mdb`. Timer1-4, CCP1-5, MSSP, EUSART1/2,
ADC, the comparator, the data EEPROM, PSP and LVD land in later phases and
are marked **phase N** below. This family has no USB and no SPP (DS39609B
Table 1-1), so it never will; do not expect them here.

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
10. [The SFR layer](#10-the-sfr-layer)
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

## 9. Timer0

8/16-bit timer/counter with its own prescaler (DS39609B §11.0), controlled
by T0CON; default 8-bit to stay a PIC16 drop-in. Overflow -> TMR0IF +
weak `TIMER0_IRQHandler`.

## 10. The SFR layer

Every SFR access names a compile-time-constant `PIC_REG_*` token through
`epic_sfr_read8` / `EPIC_REG8` (see `pic18_platform.h`). This is load-bearing:
on PIC18, a runtime SFR address compiles to the program-memory table
mechanism and silently writes nowhere (see §13).

## 11. Device selection

`PIC18F6520` is the only variant. The build driver emits `-DPIC18F6520`;
`pic18f6520_hal.h` defaults to it when nothing is defined.

## 12. The examples

- `example_smoke`: bare harness contract (family-blind).
- `example_blink`: Timer0 + GPIO + interrupt, RB0 toggle.
- `example_irq`: the IRQ-core smoke test described in §6.

## 13. Known gaps and gotchas

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
- **Foundation has no EUSART yet**, so the MPLAB SIM gates report PASS/FAIL
  on the RA0 GPIO marker (`MODE=gpio`), not over UART. Phase 3 adds the
  EUSARTs and can switch the gates to `MODE=uart`.
- Peripherals not yet ported (Timer1-4, CCP1-5, MSSP, EUSART1/2, ADC,
  comparator, data EEPROM, PSP, LVD) are phase 2-4; their headers do not
  exist yet.

## 14. Appendix: datasheet section index

| Section | Topic        | Where covered here                |
|---------|--------------|-----------------------------------|
| §4.0    | SFR map      | `pic18f6520_sfr.h`                 |
| §9.0    | Interrupts   | `core/pic18_irq.h` / `.c`, §6      |
| §10.0   | GPIO ports   | `peripherals/pic18f6520_gpio.h`   |
| §11.0   | Timer0       | `peripherals/pic18f6520_timer0.h` |
| §23.0   | Config words | `mcu/pic18f6520-mplabx/README.md` |
| Table 1-1 | Device features | §1                            |
