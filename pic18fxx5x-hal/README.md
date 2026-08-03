# PIC18F2455 family HAL

Hardware abstraction layer for the **PIC18F2455 / 2550 / 4455 / 4550** family,
inspired by the STM32Cube HAL API and structured as a sibling of the
PIC16F87XA HAL. Every constant, register address and behaviour is taken
1-to-1 from the datasheet [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf).

This tree is the second family under the shared `pic8-common/` layer
(see [docs/multi-family-plan.md](../docs/multi-family-plan.md)). The status
codes, bit helpers, the host/target harness contract, and the shared
interrupt-dispatch name (`pic8_dispatch_all_irqs`) all come from
`pic8-common/` unchanged; only the register-specific parts (SFR map,
BSR/Access-Bank platform, dual-priority interrupt backend, peripheral
drivers) live here.

➜ **[MANUAL.md](MANUAL.md)** is the full, human-readable manual:
architecture, both builds, the simulator, the harness, and a per-peripheral
reference. Start there if you are new to this HAL.

➜ **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** covers XC8 codegen
gotchas found empirically on this family (runtime-SFR-address pitfalls and
their fixes), not datasheet material, so it lives apart from `MANUAL.md`.

## Status

**Phase 4 complete — every peripheral on the part is ported.** GPIO,
Timer0-3, the dual-priority interrupt core, WDT/Sleep, the Enhanced CCP
(ECCP1) + plain CCP2, MSSP (SSP), EUSART, Comparator, Data EEPROM, the
10-bit A/D converter, and the Streaming Parallel Port (SPP) are all
implemented and cited against DS39632E. `example_blink` /
`example_timer1`/`2`/`3` / `example_ccp_pwm` / `example_ssp` /
`example_usart` / `example_comp` / `example_eeprom` / `example_adc` /
`example_spp` run on the host sim, and the HAL builds to a `.hex`
(vectors at 0008h/0018h) for all four devices on XC8.

- ✅ Family header (`pic18fxx5x.h`): device select for all four parts,
  family capability macros (flash / RAM / EEPROM / I/O / ADC channels /
  PORTD / PORTE / SPP / USB), pulls in the shared status codes and the
  platform layer.
- ✅ SFR map (`pic18fxx5x_sfr.h`): the MVP subset (STATUS, BSR, RCON,
  PORTA-E / LATA-E / TRISA-E, INTCON / INTCON2 / INTCON3, PIR1 / PIE1 /
  IPR1, TMR0L / TMR0H / T0CON), addresses cross-checked against the
  PIC18Fxxxx DFP, every bit and reset value cited to DS39632E.
- ✅ Platform layer (`include/host` + `include/target` `pic18_platform.h`):
  the same `pic8_sfr_read8` / `PIC8_REG8` / `PIC8_WEAK` contract as PIC16.
  Per the plan's Phase 2 decision, the host sim is a flat 4096-byte array
  indexed by the physical 12-bit address (no BSR translation; every MVP
  SFR is in the Access Bank 0xF60-0xFFF).
- ✅ GPIO driver (`peripherals/pic18fxx5x_gpio.h`): same API as PIC16,
  writes through LATx (DS39632E §10.0), reads PORTx, PORTB pull-ups via
  INTCON2<RBPU>.
- ✅ Timer0 driver (`peripherals/pic18fxx5x_timer0.h`): same API as PIC16
  plus a `Mode` field for the T0CON 8/16-bit select (default 8-bit, the
  PIC16-compatible mode the task manager uses).
- ✅ Timer1 driver (`peripherals/pic18fxx5x_timer1.h`): 16-bit, same API as
  PIC16; PIC18 T1CON adds `RD16` (16-bit read/write mode, set by the driver)
  and the read-only `T1RUN` status bit.
- ✅ Timer2 driver (`peripherals/pic18fxx5x_timer2.h`): 8-bit with PR2 +
  postscaler, same API as PIC16; simpler than PIC16 because PIC18's PR2 is
  in the Access Bank (no bank switching).
- ✅ Timer3 driver (`peripherals/pic18fxx5x_timer3.h`): 16-bit, mirrors
  Timer1's API; shares Timer1's T1OSC (no T3OSCEN), `RD16` set, leaves
  `T3CCP2:T3CCP1` at reset (CCP timer-select, managed by the CCP/ECCP
  driver). Overflow → PIR2<TMR3IF>.
- ✅ ECCP1 + CCP2 driver (`peripherals/pic18fxx5x_ccp.h`): mirrors PIC16's
  `HAL_CCP_*` API (capture/compare/PWM, weak ISRs) and adds the PIC18
  Enhanced CCP features PIC16 lacks — multi-output PWM (single /
  half-bridge / full-bridge forward/reverse via `P1M`), programmable
  dead-band + auto-restart (`ECCP1DEL`), and auto-shutdown with source
  select + pin states (`ECCP1AS`). PIC18 also adds a Compare "toggle on
  match" mode. CCP2 is the plain CCP (no enhanced features). The
  `T3CCP2:T3CCP1` Timer1/Timer3 capture/compare time-base select is left
  at reset (Timer1) for the driver; configurable via Timer3.
- ✅ MSSP / SSP driver (`peripherals/pic18fxx5x_ssp.h`): SPI master/slave +
  I²C master/slave, mirrors PIC16's `HAL_SSP_*` API; PIC18 MSSP registers
  are all in the Access Bank (no bank switching) and the control register
  is SSPCON1 (PIC16's is SSPCON). Register-level only — the I²C state
  machine (Start/Stop/ACK, address matching) is left to the user.
- ✅ EUSART driver (`peripherals/pic18fxx5x_usart.h`): Enhanced USART,
  mirrors PIC16's `HAL_USART_*` API + the PIC18 additions via `BAUDCON` +
  `SPBRGH` — 16-bit baud generator (`BRG16`), auto-baud detect (`ABDEN`),
  9-bit address-detect (`ADDEN`). `USART_ComputeSPBRG` encodes all four
  rows of Table 20-1.
- ✅ Comparator driver (`peripherals/pic18fxx5x_comp.h`): two on-chip
  comparators, mirrors PIC16's `HAL_COMP_*` API — the PIC18 `CMCON` has
  the same bit layout and eight modes, just in the Access Bank. Added
  `PIC18_IRQ_CMP` (PIR2<CMIF>). CVRCON (comparator Vref) is separate.
- ✅ Data EEPROM driver (`peripherals/pic18fxx5x_eeprom.h`): 256-byte
  data EEPROM, mirrors PIC16's `HAL_EEPROM_*` API; PIC18 moves the
  registers into the Access Bank and adds `EEPGD`/`CFGS` (kept 0 for
  data EEPROM). Write does the mandatory 0x55→0xAA unlock; the sim models
  the cell array.
- ✅ A/D Converter driver (`peripherals/pic18fxx5x_adc.h`): 10-bit ADC,
  10/13 channels; a fresh design (not a mirror) for the PIC18's three
  control registers — `ADCON0` (channel/GO-DONE/ADON), `ADCON1`
  (PCFG + VCFG Vref), `ADCON2` (ADCS clock / ACQT acquisition / ADFM).
  Tables cross-checked against DS39632E §21; PCFG is a raw Table 21-3 code.
- ✅ SPP driver (`peripherals/pic18fxx5x_spp.h`): Streaming Parallel Port,
  the USB-era parallel port (the PIC18 analog of PIC16's PSP), 40/44-pin
  only — fully gated through `PIC18FXX5X_FAMILY_HAS_SPP`. Register-level:
  programs SPPCON/SPPCFG/SPPEPS, byte-level SPPDATA access, busy/WR/RD
  status, SPPIF IRQ; the USB streaming protocol is left to the user.
- ✅ Interrupt core (`core/pic18_irq.h`): `PIC18_IRQn` enum, `HAL_IRQ_*`
  against INTCON / INTCON2 / INTCON3 / PIE1 / PIR1 / IPR1, priority mode
  (IPEN) enabled by `HAL_IRQ_Restore`. `HAL_IRQ_SetPriority` is the
  shared-contract extension (no-op on PIC16).
- ✅ ISR vectors (`src/core/pic18_isr_vector.c`, XC8 only):
  `__interrupt(high_priority)` at 0008h and `__interrupt(low_priority)` at
  0018h, both delegating to `pic8_dispatch_all_irqs`.
- ✅ WDT / Sleep (`core/pic18fxx5x_wdt_sleep.h`): `HAL_WDT_Refresh` /
  `HAL_Sleep_Enter` (asm on target, no-op on host) + BOR/POR status from
  RCON (PIC18 folds TO/PD/POR/BOR into RCON, not a separate PCON).
- ✅ Host simulation backend (`src/sim/pic18_sim.c`): Timer0/1/2/3 stepping
  (8/16-bit, prescalers, overflow → TMR0IF/PIR1/PIR2 flags) + GPIO
  drive/read, mirroring `pic16f87xa_sim.c`'s API shape.
- ✅ `example_blink` (Timer0 + GPIO + interrupt), `example_timer1`,
  `example_timer2`, `example_timer3`, `example_ccp_pwm` (ECCP1 half-bridge
  PWM + dead-band), `example_ssp`, `example_usart`, `example_comp`,
  `example_eeprom`, `example_adc`, `example_spp`, + `example_smoke`
  (harness seam) — all buildable on host sim; the XC8 target build uses
  `example_blink` as its APP_SOURCES and links the full HAL library.

**Deferred:** real-silicon blink confirmation (no PIC18 board on hand;
flagged in the plan, not silently skipped).

## Layout

```
pic18fxx5x-hal/
├── include/
│   ├── pic18fxx5x.h              Family header, device selection, platform
│   │                             include; pulls in shared status codes /
│   │                             bit helpers from pic8-common/hal_status.h
│   ├── pic18fxx5x_sim.h          Simulation backend public API (Phase 1 minimal)
│   ├── host/pic18_platform.h     Host platform: memory-backed SFR + weak attr
│   ├── target/pic18_platform.h   Target platform: volatile-deref SFR
│   ├── core/                     (Phase 2: pic18_irq.h)
│   └── peripherals/              (Phase 2: HAL_GPIO_*, HAL_TIMER0_*, ...)
├── src/
│   ├── core/                     harness_sim, irq_dispatch (Phase 2: isr_vector)
│   ├── peripherals/              (Phase 2)
│   └── sim/                      Host simulation backend
├── tests/                        example_smoke (Phase 2: example_blink, ...)
├── mcu/pic18fxx5x-mplabx/        XC8 Makefile (thin caller of pic8-common/mk)
└── CMakeLists.txt                Host build (thin caller of pic8-common/cmake)
```

## Build (host simulation)

```sh
cmake -B build -S .
cmake --build build

./build/example_blink          # Timer0 + GPIO + interrupt: RB0 toggled 9 times
./build/example_smoke          # harness seam: smoke: 10 ticks, device <PART>
./build/example_blink_PIC18F2455
./build/example_blink_PIC18F2550
./build/example_blink_PIC18F4455
./build/example_blink_PIC18F4550
```

`example_blink` exits 0 with `RB0 toggled 9 times` (600k sim cycles /
256×256 ≈ 9 Timer0 overflows). `example_smoke` exits 0 with
`smoke: 10 ticks, device <PART>`. Both prove the shared `pic8_harness_*`
contract is family-blind: PIC18 links against the exact same header and
contract PIC16 uses.

## Build (real target)

The HAL compiles against XC8 via the same sources, with the host/target
split done at **build time, not with `#ifdef`**: the XC8 Makefile puts
`include/target` ahead of `include` on the include path, so
`pic18_platform.h` resolves to the real-target version (SFR macros =
direct volatile dereference) and the family-blind target harness is
linked. The MPLAB X project lives under `mcu/pic18fxx5x-mplabx/`; its
Makefile produces `<MCU>-firmware.hex` via `xc8-cc`.

### PIC18F DFP requirement

XC8 v3.x moved device support into Device Family Packs. The **PIC18Fxxxx
DFP is not bundled with XC8** (unlike the PIC16Fxxx DFP); install it once:

- Via MPLAB X: *Tools → Packs → Pack Manager*, search `PIC18Fxxxx_DFP`.
- Or manually: download `Microchip.PIC18Fxxxx_DFP.<ver>.atpack` from
  [packs.download.microchip.com](https://packs.download.microchip.com/)
  and unzip it into
  `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`
  (the Makefile's `DFP_DIR` defaults to the `xc8/` subdir there).

Then:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make MCU=18F4550    # default; also 18F2455 / 18F2550 / 18F4455
```

The result is `build/18F4550-firmware.hex`. The Phase 1 config-word set
is a conservative default (HS oscillator, WDT on, LVP off, XINST off,
code/write/read protection off); adjust per your hardware. The block-3
code-protection settings (`CP3` / `WRT3` / `EBTR3`) are emitted only for
the 32 KB parts (18F2550 / 18F4550), since the 24 KB parts stop at block 2
(DS39632E §23.1).

## XC8 PIC18 interrupt syntax (resolved, Phase 1 open question)

PIC18F2455/2550/4455/4550 are classic dual-priority-vector PIC18 devices
(vectors at 0008h high, 0018h low; DS39632E §9.0), not the vectored-
interrupt-controller variants. Per the XC8 C Compiler User Guide for PIC
(DS50002737K) §5.9.1.2 "Writing Dual-priority or Legacy Mode ISRs", the
syntax Phase 2 uses for `pic18_isr_vector.c` is:

```c
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void) { ... }
void __interrupt(low_priority)  PIC18_IRQ_HandlerLow(void)  { ... }
```

If no priority argument is present the ISR defaults to high priority;
Microchip recommends always specifying it. This is recorded in
[docs/multi-family-plan.md](../docs/multi-family-plan.md).

## API conventions

Same as the PIC16F87XA HAL (mirror STM32Cube): `HAL_PPP_Init/DeInit`,
`HAL_PPP_MspInit` weak override, `GPIOA..` / `GPIO_PIN_*`, `HAL_OK/ERROR/
BUSY/TIMEOUT/INVALID`, `PIC8_BIT*`. The IRQ enum will be `PIC18_IRQ_*`
(Phase 2), taking the per-family `PIC18_IRQn` type, with the priority
contract extension decided and recorded in the plan before the
interrupt core is written.
