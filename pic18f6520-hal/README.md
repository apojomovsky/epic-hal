# PIC18F6520 family HAL

Hardware abstraction layer for the **PIC18F6520**, inspired by the STM32Cube
HAL API and structured as a sibling of the PIC18F2455 family HAL (the
architecturally closest 64-pin PIC18 family: same Access Bank shape,
0x0000-0x005F, same two-vector interrupt architecture, confirmed against
`p18f6520.toml` in epic-cc). Every constant, register address and behaviour
is taken 1-to-1 from the datasheet
[DS39609B](https://ww1.microchip.com/downloads/en/DeviceDoc/39609b.pdf)
(PIC18F6520/6620/8520/8620/6720/8720).

This tree is another family under the shared `epic-common/` layer
(see [epic-common/README.md](../epic-common/README.md) and
[epic-common/MANUAL.md](../epic-common/MANUAL.md)). The status codes, bit
helpers, the host/target harness contract, and the shared interrupt-dispatch
name (`epic_dispatch_all_irqs`) all come from `epic-common/` unchanged; only
the register-specific parts (SFR map, Access-Bank platform, dual-priority
interrupt backend, peripheral drivers) live here.

**[MANUAL.md](MANUAL.md)** is the human-readable manual.

## Status

**Complete (epic-hal#182, #183, #184, #185, umbrella #150; ECAN-quad and
80-pin coverage #246):** platform, SFR map, GPIO (PORTA-G, plus PORTH/J
on the 80-pin parts), Timer0-3, CCP (1-5, 1-2 on the ECAN quads), MSSP,
EUSART (1/2, single on the ECAN quads), ADC, comparator, data EEPROM,
the dual-priority interrupt core, WDT/Sleep/BOR/POR, and the harness are
implemented and verified under host sim and real `mdb` (each peripheral
through docs/adding-a-device.md §4: host example + XC8 target build +
`mdb` register-readback gate). TMR4, PSP, LVD and the ECAN module have
no driver yet (SFRs are in the map). This part has **no USB and no SPP**
(DS39609B Table 1-1).

- ✅ Family header (`pic18f6520_hal.h`): device selection, capability
  macros, platform include. Named `_hal` to avoid shadowing the DFP
  `pic18f6520.h` (see "XC8 codegen gotchas" below).
- ✅ SFR map (`pic18f6520_sfr.h`): core status, GPIO (PORTA-G, plus
  PORTH/J on the 80-pin parts), INTCON/INTCON2/INTCON3, PIR1-3/PIE1-3/
  IPR1-3, T0CON/TMR0L/H, T1CON/TMR1L/H, T2CON/PR2/TMR2, T3CON/TMR3L/H,
  T4CON/PR4/TMR4, CCP1-5CON/CCPRxL/H, MSSP, EUSART1/2, PSP, ADC,
  comparator, CVR, LVD, EEPROM; addresses cross-checked against the
  PIC18Fxxxx DFP and the EDC, bits/reset values cited to DS39609B. The
  PIC_REG_* address set is generator-projected
  (`scripts/gen-sfr.py --family PIC18F6520 --check` is a CI gate).
- ✅ Platform layer (`include/host` + `include/target` `pic18_platform.h`):
  the same `epic_sfr_read8` / `EPIC_REG8` / `EPIC_WEAK` contract as the
  other PIC18 families; host sim is a flat 4096-byte array indexed by the
  physical 12-bit SFR address (all foundation SFRs are in the Access Bank
  0xF60-0xFFF).
- ✅ GPIO driver (`peripherals/pic18f6520_gpio.h`): PORTA-G (7 ports),
  plus PORTH/J on the 80-pin parts (GPIOH/J, full 8-bit, DS39661 §10.0);
  writes through LATx (DS39609B §10.0), reads PORTx, PORTB pull-ups via
  INTCON2<RBPU>, RB<7:4> change interrupt hook.
- ✅ Timer0 driver (`peripherals/pic18f6520_timer0.h`): same API as PIC16
  plus a `Mode` field for the T0CON 8/16-bit select (default 8-bit).
- ✅ Timer1 driver (`peripherals/pic18f6520_timer1.h`): 16-bit with RD16
  atomic read/write; DS39609B Register 12-1 (bit 6 unimplemented, no
  T1RUN on this part).
- ✅ Timer2 driver (`peripherals/pic18f6520_timer2.h`): period match with
  4-bit postscaler, DS39609B Register 13-1.
- ✅ Timer3 driver (`peripherals/pic18f6520_timer3.h`): 16-bit with RD16,
  DS39609B Register 14-1. Timer4 (T4CON/PR4/TMR4 at 0xF76-0xF78) has no
  driver yet (#183 covered Timer0-3 only).
- ✅ CCP driver (`peripherals/pic18f6520_ccp.h`): one driver with an
  instance selector over five plain CCP modules (CCP1CON/CCPR1 at
  0xFBD-0xFBF through CCP5 at 0xF70-0xF72); CCP1-2 only on the ECAN
  quads (the rest rejected, same addresses). All plain: no
  auto-shutdown/PWM-bridge hardware on this part (no PSTRCON/ECCPAS/
  PWM1CON in the DFP), so no ECCP-specific API.
- ✅ MSSP driver (`peripherals/pic18f6520_ssp.h`): SPI + I2C, same
  register shape as the 4550 family.
- ✅ EUSART driver (`peripherals/pic18f6520_usart.h`): one driver with
  an instance selector over the two identical EUSART modules (single
  EUSART on the ECAN quads, same addresses as EUSART1); 8-bit
  BRG only (no BAUDCON/SPBRGH on this part).
- ✅ ADC driver (`peripherals/pic18f6520_adc.h`): 12-channel 10-bit SAR,
  CHS/VCFG/PCFG/ADCS; no ACQT field on this part (ADCON2 carries only
  ADCS + ADFM, DFP/EDC confirmed), so the Acquisition handle field is
  kept for contract compatibility and ignored.
- ✅ Comparator driver (`peripherals/pic18f6520_comp.h`): two
  comparators, 8 CMCON modes, C1OUT/C2OUT readouts, CMIF change
  interrupt. CVRCON (the comparator voltage reference) is a separate
  module in the SFR map; no driver yet.
- ✅ Data EEPROM driver (`peripherals/pic18f6520_eeprom.h`): 1 KB,
  10-bit EEADRH:EEADR addressing (the 4550/2520 families use 8-bit
  EEADR only), unlock 0x55/0xAA, EEIF write-complete (interrupt or
  poll).
- ✅ Interrupt core (`core/pic18_irq.h`): `PIC18_IRQn` enum (25 sources,
  the richest set in this repo: INT0-3, RB, TMR0-4, CCP1-5, SSP, USART1/2
  TX+RX, ADC, CMP, EEPROM, LVD, PSP), `EPIC_IRQ_*` against
  INTCON/INTCON2/INTCON3/PIE1-3/PIR1-3/IPR1-3, priority mode (IPEN) enabled
  by `EPIC_IRQ_Restore`.
- ✅ ISR vectors (`src/target/pic18_isr_vector.c`, XC8 only):
  `__interrupt(high_priority)` at 0008h and `__interrupt(low_priority)` at
  0018h, both delegating to `epic_dispatch_all_irqs`.
- ✅ WDT / Sleep (`core/pic18f6520_wdt_sleep.h`): `EPIC_WDT_Refresh` /
  `EPIC_Sleep_Enter` (asm on target, no-op on host) + BOR/POR status from
  RCON.
- ✅ Host simulation backend (`src/sim/pic18_sim.c`): Timer0-3 stepping
  (8/16-bit, prescaler, overflow -> flag + IRQ callback) + GPIO drive/read
  over all nine ports.
- ✅ `example_blink` (Timer0 + GPIO + interrupt), `example_irq` (the
  dedicated IRQ-core smoke test), `example_smoke` (harness seam),
  `example_timer1/2/3` (per-timer host overflow/match smokes),
  `example_ccp` (CCP1/CCP3 compare), `example_ssp` (SPI master),
  `example_usart` (dual-EUSART async TX), `example_adc` (AN0
  conversion), `example_comp` (two-independent mode), `example_eeprom`
  (10-bit write/read round-trip).
- ✅ MPLAB SIM gate, `MODE=gpio`: `src/mdb/pic18_harness_mdb.c`
  drives the PASS/FAIL marker on RA0 (the pic16f193x pattern), read by the
  CI wrapper via the latch `LATA` (`GPIO_REG=LATA`: PIC18's driven output
  latch does not read back on PORTx under MPSIM, verified 2026-09-15).
- ✅ CI wiring at foundation time: manifest family + pseudo-module,
  `scripts/ci-target-sim.sh` sim gate, `sfr-map-audit.py` / `config-key-audit.py`
  / `hex-identity-audit.py` / `gen-sfr.py` / `pre-commit-checks.sh` family
  registration, and the `epic_build.py` PIC18 classifier (the canonical
  part comes from the manifest's `variants[-1]`, shared with gen-sfr.py).
  The `family-check.yml` job is deferred until a consumer module exists (the
  1320/2520 pattern); the manifest family/module, device-data audits, and
  the sim-gate definition are wired and pass on their own.

## Device facts (confirmed against the DFP, not assumed)

The 6520 is a 64-pin part and the largest of the three new PIC18 families:

- **7 I/O ports on the 64-pin parts** (PORTA-G), **9 on the 80-pin ones**
  (PORTH/J), every one with PORTx/LATx/TRISx (the 2520/1320 forks have 3
  and 2); TRISG implements RG0-RG4 only.
- **5 CCP modules** (CCP1-5, all plain CCP: no ECCP auto-shutdown/PWM
  registers exist on this part), PIR3/PIE3/IPR3 carry their flags.
- **4 timers** (TMR0-3 plus TMR4, T4CON/PR4/TMR4 at 0xF76-0xF78).
- **2 EUSARTs** (EUSART1 registers at 0xFAB-0xFAF, EUSART2 at 0xF6B-0xF6F;
  one driver with an instance selector; no BAUDCON and no SPBRGH on
  this part, the baud generator is 8-bit only).
- **1 MSSP**, **PSP** (parallel slave port, PSPCON at 0xFB0; no driver
  yet), **dual comparator + CVR**, **LVD** (no driver yet), **1 KB data
  EEPROM** (EEADR + EEADRH, 10-bit), **12-channel 10-bit A/D**
  (AN0-AN11). The ADC's ADCON2 has no ACQT field (only ADCS + ADFM),
  unlike the 4550/2520/1320.
- No USB, no SPP. Config words drop usbdiv/cpudiv/plldiv/vregen; the
  brown-out and stack-reset fields are spelled `BOR` / `STVR` (not
  `BOREN` / `STVREN`), and there is no `MCLRE` / `IESO` / `FCMEN` /
  `PBADEN` / `XINST` key.

## XC8 codegen gotchas (settled)

- **Never pass an SFR address as a runtime value on PIC18.** XC8 compiles a
  runtime-computed SFR pointer to the program-memory table mechanism
  (`tblrd`/`tblwt`), silently writing nowhere. Every SFR access names a
  compile-time-constant `PIC_REG_*` token; branch on the instance before
  touching SFRs.
- **The family umbrella must not be `pic18f6520.h`.** That is the DFP
  device header's name; an umbrella of the same name shadows it through
  `-I` at link time so the compiler's auto-included SFRs are lost (the
  PIC16F628A lesson, epic-hal#137). The umbrella is `pic18f6520_hal.h`.
- **No compiler bugs found in any phase.** Every XC8 surprise traced to a
  DFP misread or a SIM/mdb tool limit, not codegen. The literal-SFR-token
  rule above is a design constraint verified by green builds, not a bug
  report. The XC8 User's Guide is not shipped in the toolchain image, so
  nothing here is claimed as a compiler bug without that source (the
  adding-a-device.md rule).
- **mdb SIM does not model EEPROM cell writes or SPI shifting.** The
  EEPROM gate proves the address load + unlock + WR strobe (EEADRH/EEADR/
  EECON1 readbacks); the cell contents are a host-sim-only model
  (`pic18_sim_*eeprom*`). Same limit as the 4550/2520 families.

## Layout

```
pic18f6520-hal/
├── include/
│   ├── pic18f6520_hal.h        Family header, device selection, platform
│   ├── pic18f6520_sfr.h        SFR address map + bit names (1:1 DS39609B)
│   ├── pic18f6520_sim.h        Simulation backend public API
│   ├── epic_hal.h              Family-neutral top-level include
│   ├── host/pic18_platform.h   Host platform: memory-backed SFR + weak
│   ├── target/pic18_platform.h Target platform: volatile-deref SFR
│   ├── epiccc/pic18_platform.h epic-cc variant of the SFR layer
│   ├── core/                   pic18_irq.h, hal_irq.h, wdt_sleep, hal_wdt_sleep
│   └── peripherals/            pic18f6520_gpio.h, pic18f6520_timer0.h, hal_* pointers
├── src/
│   ├── core/                   pic18_irq.c, pic18_irq_dispatch.c, wdt_sleep
│   ├── peripherals/            implementations of peripherals/ headers
│   ├── target/                 isr_vector + wdt_sleep_target (XC8 only)
│   ├── epiccc/                 vector/dispatch/wdt_sleep for the epiccc build
│   ├── sim/                    harness_sim, sim backend (host build only)
│   └── mdb/                    harness_mdb (MPLAB SIM gate, RA0 marker)
├── tests/                      example_blink, example_irq, example_smoke
├── mcu/pic18f6520-mplabx/      XC8 real-target build notes
└── CMakeLists.txt              Host build (thin caller of epic-common/cmake)
```

## Build (host simulation)

```sh
cmake -B build -S pic18f6520-hal && cmake --build build
./build/example_blink          # Timer0 + GPIO + interrupt: RB0 toggled 9 times
./build/example_irq            # IRQ-core smoke: enable/fire/read-back
./build/example_smoke          # harness seam: smoke: 10 ticks, device PIC18F6520
```

## Build (real target)

Manifest-driven via `epic-common/manifest/modules.toml` (the family table and
the `pic18f6520-hal` pseudo-module); see `mcu/pic18f6520-mplabx/README.md` and
`epic-common/manifest/README.md`.

```sh
python3 scripts/epic_build.py build --module pic18f6520-hal --mcu 18F6520 --run
```

## Environment split

`src/` mirrors the three build environments: `src/core/` and
`src/peripherals/` are shared (host and target builds compile them),
`src/target/` is real-hardware-only, `src/sim/` is host-simulation-only,
`src/mdb/` is the MPLAB SIM gate variant, `src/epiccc/` serves the epic-cc
toolchain build. Never glob a `src/` directory into your build; select files
through the manifest.
