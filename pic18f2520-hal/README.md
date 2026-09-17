# PIC18F2520 family HAL

Hardware abstraction layer for the **PIC18F2520**, inspired by the STM32Cube
HAL API and structured as a sibling of the PIC18F2455 family HAL (the
architecturally closest 28/40-pin PIC18 family: same Access Bank addressing
shape at the family level, same two-vector interrupt architecture). Every
constant, register address and behaviour is taken 1-to-1 from the datasheet
[DS39631E](https://ww1.microchip.com/downloads/en/DeviceDoc/39631E.pdf).

This tree is the third family under the shared `epic-common/` layer
(see [epic-common/README.md](../epic-common/README.md) and
[epic-common/MANUAL.md](../epic-common/MANUAL.md)). The status codes, bit
helpers, the host/target harness contract, and the shared interrupt-dispatch
name (`epic_dispatch_all_irqs`) all come from `epic-common/` unchanged; only
the register-specific parts (SFR map, BSR/Access-Bank platform,
dual-priority interrupt backend, peripheral drivers) live here.

**[MANUAL.md](MANUAL.md)** is the human-readable manual.

## Status

**Complete family (phases 1-4, epic-hal#174-177):** platform, SFR map, GPIO,
Timer0-3, ECCP/CCP, MSSP, EUSART, ADC, comparator, data EEPROM, the
dual-priority interrupt core, WDT/Sleep, and the harness are implemented and
verified under host sim and real `mdb` (each peripheral through
docs/adding-a-device.md §4: host example + XC8 target build + `mdb`
register-readback gate). This part has **no USB and no SPP** (DS39631E
Table 1-1).

**18F2220 (epic-hal#155):** mid-generation sibling, DS39599. Same GPIO/
Timer0/IRQ/WDT foundation; no ECCP1 (standard CCP1, no ECCP1AS/PWM1CON,
`PIC18F2520_FAMILY_HAS_ECCP1=0`) and no 16-bit BRG/auto-baud (no BAUDCON
register at all, `PIC18F2520_FAMILY_HAS_BRG16=0`), recorded as
informational capability macros for now (`pic18f2520_ccp.c`/
`pic18f2520_usart.c` are `conditional_sources` scoped to 18F2520 only, so
no build links them for the 2220/2320 yet; wire the macros in when a
module needs CCP/USART on a variant without ECCP1/BRG16). Its CONFIG3H/4L
shape also predates the 2520's (`CCP2MX` is ON/OFF not PORTC/PORTBE,
`PBAD` not `PBADEN`, `STVR`/`FSCM` not `STVREN`/`FCMEN`, no
`XINST`/`LPT1OSC`).
**Flash-budget finding:** at 4 KB (2048 instructions, the smallest part
in this family), the 2220 cannot link the family's full peripheral set
just to build the foundation examples, so `hal_sources` was split:
GPIO+Timer0+IRQ core/WDT is the baseline every variant gets,
Timer1-3/CCP/SSP/USART/ADC/comparator/EEPROM are `conditional_sources`
scoped to `variants=["18F2520"]` (preserving the 2520's exact prior XC8
link order, verified with `epicmanifest.sources_for`). The shared
`pic18_irq_dispatch.c` has strong externs to every peripheral's
IRQHandler by design (see its own header comment), so it cannot link
against the trimmed baseline either; `pic18_irq_dispatch_min.c` (Timer0/
RB only) is the baseline-variant dispatch, and the full dispatch is now
itself a `conditional_sources` entry for 18F2520. Even with the trim,
blink fits (86.3% flash) but the mdb `irq-smoke` gate does not (the mdb
harness + `example_irq.c` overflow the remaining budget substantially,
not marginally); 18F2220 is the family's only 4 KB variant and is
intentionally absent from `family-check.yml`'s mdb job list for this
reason. A future small-flash part repeats this pattern (extend the
`conditional_sources`/dispatch split's `variants` lists), not a new one.

**18F2320 (epic-hal#155):** same DS39599 shape and capability macros as
the 2220 (no ECCP1, no 16-bit BRG, same CONFIG3H/4L field set) but 8 KB
flash, double the 2220's. Blink fits at 43.1% and, unlike the 2220, the
mdb `irq-smoke` gate fits too (verified via `mdb` register readback), so
it is in `family-check.yml`'s mdb job list. PIC18 does not mirror a
driven `LATx` latch back into `PORTx` under MPLAB SIM (the same finding
as the 2520/6520 gates), so its gate reads `LATA`
(`scripts/ci-target-sim.sh`'s device allowlist).

- ✅ Family header (`pic18f2520_hal.h`): device selection, capability
  macros, platform include. Named `_hal` to avoid shadowing the DFP
  `pic18f2520.h` (see "XC8 codegen gotchas" below).
- ✅ SFR map (`pic18f2520_sfr.h`): core status, GPIO (PORTA/B/C),
  INTCON/INTCON2/INTCON3, PIR1/PIE1/IPR1, PIR2/PIE2/IPR2, T0CON/TMR0L/H,
  addresses cross-checked against the PIC18Fxxxx DFP, bits/reset values
  cited to DS39631E.
- ✅ Platform layer (`include/host` + `include/target` `pic18_platform.h`):
  the same `epic_sfr_read8` / `EPIC_REG8` / `EPIC_WEAK` contract as PIC16;
  host sim is a flat 4096-byte array indexed by the physical 12-bit SFR
  address (all foundation SFRs are in the Access Bank 0xF60-0xFFF).
- ✅ GPIO driver (`peripherals/pic18f2520_gpio.h`): PORTA/B/C, writes through
  LATx (DS39631E §10.0), reads PORTx, PORTB pull-ups via INTCON2<RBPU>,
  RB<7:4> change interrupt hook.
- ✅ Timer0 driver (`peripherals/pic18f2520_timer0.h`): same API as PIC16
  plus a `Mode` field for the T0CON 8/16-bit select (default 8-bit).
- ✅ Interrupt core (`core/pic18_irq.h`): `PIC18_IRQn` enum (no SPP source),
  `EPIC_IRQ_*` against INTCON/INTCON2/INTCON3/PIE1/PIR1/IPR1/PIE2/PIR2/IPR2,
  priority mode (IPEN) enabled by `EPIC_IRQ_Restore`.
- ✅ ISR vectors (`src/target/pic18_isr_vector.c`, XC8 only):
  `__interrupt(high_priority)` at 0008h and `__interrupt(low_priority)` at
  0018h, both delegating to `epic_dispatch_all_irqs`.
- ✅ WDT / Sleep (`core/pic18f2520_wdt_sleep.h`): `EPIC_WDT_Refresh` /
  `EPIC_Sleep_Enter` (asm on target, no-op on host) + BOR/POR status from
  RCON.
- ✅ Host simulation backend (`src/sim/pic18_sim.c`): Timer0 stepping
  (8/16-bit, prescaler, overflow -> TMR0IF + IRQ callback) + GPIO drive/read.
- ✅ `example_blink` (Timer0 + GPIO + interrupt), `example_irq` (the
  dedicated IRQ-core smoke test), `example_smoke` (harness seam).
- ✅ MPLAB SIM gate, `MODE=gpio`: `src/mdb/pic18_harness_mdb.c`
  drives the PASS/FAIL marker on RA0 (the pic16f193x pattern), read by the
  CI wrapper via the latch `LATA` (`GPIO_REG=LATA`: PIC18's driven output
  latch does not read back on PORTx under MPSIM).

## XC8 codegen gotchas (settled)

- **Never pass an SFR address as a runtime value on PIC18.** XC8 compiles a
  runtime-computed SFR pointer to the program-memory table mechanism
  (`tblrd`/`tblwt`), silently writing nowhere. Every SFR access names a
  compile-time-constant `PIC_REG_*` token; branch on the instance before
  touching SFRs.
- **The family umbrella must not be `pic18f2520.h`.** That is the DFP
  device header's name; an umbrella of the same name shadows it through
  `-I` at link time so the compiler's auto-included SFRs are lost (the
  PIC16F628A lesson, epic-hal#137). The umbrella is `pic18f2520_hal.h`.
- **No compiler bugs found in phases 2-4.** Every XC8 surprise traced to
  a DFP misread or a SIM/mdb tool limit, not codegen. The XC8 User's Guide
  is not shipped in the toolchain image, so nothing here is claimed as a
  compiler bug; the split read+write RMW pattern and literal-SFR-token rule
  above are design constraints verified by green builds, not bug reports.
- **Reduced ECCP1 (driver, not compiler).** T3CON<T3CCP1> is BIT(3), not
  BIT(2); no P1M/PDC/PSSBD hardware on this 28-pin part. Caught by reading
  the DFP struct order, fixed before first build.
- **EUSART TXEN/CREN always on (driver).** Tying them to callbacks breaks
  polled TX/RX; TXIE/RCIE gate interrupts. TXSTA POR is 0x02 (TRMT).
- **mdb `set` does not write SFRs.** Preload-via-debugger gates fail
  silently; use ReloadValue/CompareValue driver paths for overflow/match
  gates instead.
- **SIM models registers, not physics.** SPI shifting, EEPROM cell writes,
  CCP output pins, and ADC conversions do not advance in MPLAB SIM; those
  gates prove programming + flags (the 4550's accepted level).
## Layout

```
pic18f2520-hal/
├── include/
│   ├── pic18f2520_hal.h        Family header, device selection, platform
│   ├── pic18f2520_sfr.h        SFR address map + bit names (1:1 DS39631E)
│   ├── pic18f2520_sim.h        Simulation backend public API
│   ├── epic_hal.h              Family-neutral top-level include
│   ├── host/pic18_platform.h   Host platform: memory-backed SFR + weak
│   ├── target/pic18_platform.h Target platform: volatile-deref SFR
│   ├── epiccc/pic18_platform.h epic-cc variant of the SFR layer
│   ├── core/                   pic18_irq.h, hal_irq.h, wdt_sleep, hal_wdt_sleep
│   └── peripherals/            pic18f2520_gpio.h, pic18f2520_timer0.h, hal_* pointers
├── src/
│   ├── core/                   pic18_irq.c, pic18_irq_dispatch.c, wdt_sleep
│   ├── peripherals/            implementations of peripherals/ headers
│   ├── target/                 isr_vector + wdt_sleep_target (XC8 only)
│   ├── epiccc/                 vector/dispatch/wdt_sleep for the epiccc build
│   ├── sim/                    harness_sim, sim backend (host build only)
│   └── mdb/                    harness_mdb (MPLAB SIM gate, RA0 marker)
├── tests/                      example_blink, example_irq, example_smoke
├── mcu/pic18f2520-mplabx/      XC8 real-target build notes
└── CMakeLists.txt              Host build (thin caller of epic-common/cmake)
```

## Build (host simulation)

```sh
cmake -B build -S pic18f2520-hal && cmake --build build
./build/example_blink          # Timer0 + GPIO + interrupt: RB0 toggled 9 times
./build/example_irq            # IRQ-core smoke: enable/fire/read-back
./build/example_smoke          # harness seam: smoke: 10 ticks, device PIC18F2520
```

## Build (real target)

Manifest-driven via `epic-common/manifest/modules.toml` (the family table and
the `pic18f2520-hal` pseudo-module); see `mcu/pic18f2520-mplabx/README.md` and
`epic-common/manifest/README.md`.

```sh
python3 scripts/epic_build.py build --module pic18f2520-hal --mcu 18F2520 --run
```

## Environment split

`src/` mirrors the three build environments: `src/core/` and
`src/peripherals/` are shared (host and target builds compile them),
`src/target/` is real-hardware-only, `src/sim/` is host-simulation-only,
`src/mdb/` is the MPLAB SIM gate variant, `src/epiccc/` serves the epic-cc
toolchain build. Never glob a `src/` directory into your build; select files
through the manifest.
