# pic16f88x-hal

HAL for the PIC16F88X family (PIC16F882/883/884/886/887), classic
mid-range 8-bit parts. Full peripheral coverage: GPIO (per-pin
pull-ups, interrupt-on-change, analog select), Timer0/1/2 (Timer1 with
gate), EUSART (BRG16, auto-baud, wake-up), MSSP (SPI + I2C with address
mask), 10-bit ADC (11/14 channels + CVREF + VP6), two comparators with
SR latch, CVREF, ECCP1 (dead-time, steering, auto-shutdown) + CCP2,
data EEPROM, WDT/BOR/Sleep, internal oscillator with fail-safe monitor,
and ULPWU. Datasheet: DS40001291H; silicon errata: DS80000302K; Timer1
errata: DS80329B.

## Build

Host simulation (no hardware):

```sh
cmake -B build && cmake --build build
./build/example_blink
```

Real target (XC8, via the manifest):

```sh
python3 scripts/epic_build.py build --module pic16f88x-hal --mcu 16F887 \
    --variant target --build-dir build-target --dfp-dir <dfp>/xc8
```

MPLAB SIM gate (HARNESS=sim, the banked-SFR probe):

```sh
python3 scripts/epic_build.py build --module pic16f88x-hal --mcu 16F887 \
    --variant sim --build-dir build-sim/pic16f88x-hal --dfp-dir <dfp>/xc8
bash scripts/sim-mdb-run.sh pic16f88x 16F887 PIC16F887 pic16f88x-hal 5000 uart
```

## Layout

- `include/pic16f88x.h`: device selection, family capability macros.
- `include/pic16f88x_sfr.h`: SFR address map + bit definitions, 1:1
  with DS40001291H Tables 2-1..2-4 and the register blocks.
- `include/host/` vs `include/target/`: the SFR mapping layer split at
  build time (memory-backed sim register file vs direct volatile
  deref), no `#ifdef` in application code.
- `src/core/`: IRQ controller + dispatch, WDT/Sleep/BOR/POR helpers.
- `src/peripherals/`: one driver per peripheral.
- `src/sim/`: the host simulation backend.
- `src/target/`: real-target-only glue (ISR vector, WDT/Sleep asm).
- `src/mdb/`: the MPLAB SIM harness.
- `tests/`: host examples + the HARNESS=sim banked-SFR probe.

## XC8 codegen gotchas (verified, not assumed)

- **Banked SFR RMW**: a plain C read-modify-write on a Bank 1/2/3 SFR
  (OPTION_REG, TRISx, TXSTA, ADCON1, VRCON, WDTCON, CMxCON0, SRCON,
  BAUDCTL, ANSEL...) silently misdirects to the Bank 0 alias under XC8
  v4.00. Every such access goes through the `EPIC_BANK1/2/3_READ8/
  WRITE8` inline-asm macros in `include/target/pic16f88x_platform.h`
  (load into W through a common-RAM scratch byte, bank in, one
  movwf/iorwf/andwf, bank out). The HARNESS=sim probe
  (`tests/sim_bank_probe.c`) exercises every one of these sites under
  MPLAB SIM with known values, so a regression fails the gate instead
  of corrupting silently.
- **The 88X spreads SFRs across all four banks** (Bank 2: WDTCON,
  CM1CON0/CM2CON0/CM2CON1, EEPROM data/address; Bank 3: SRCON,
  BAUDCTL, ANSEL/ANSELH, EECON1/EECON2). The 87XA's macros only
  covered Banks 1/3; the 88X adds the Bank 2 path.
- **Handle storage**: every driver copies the caller's handle into
  driver-owned storage (the 87XA's pointer-holding version was a
  confirmed dangling-pointer bug, see epic-common/MANUAL.md §3.3). The
  storage is unpinned: the linker's best-fit scatter packs it, and the
  smallest part (882, 128 B RAM) has no Bank 2/3 GPR to pin into.
- **Flash page pinning**: `epic_dispatch_all_irqs` is pinned to 0x900
  on parts with >= 4K flash (XC8 emits no PCLATH setup for the handler
  calls, so the dispatch and handlers must share a page). The 882 (2K
  flash, one page) needs no pin.
- **Inline asm is XC8-only**: the banked macros live in the target
  platform header, never in shared sources; the host build uses the
  plain-array path.

## Silicon errata that shape the drivers

- Timer0 prescaler-assignment switch can spuriously reset (item 10);
  `EPIC_TIMER0_Start` follows the datasheet sequence.
- Timer1 external-crystal reload can miss the first count (item 8);
  the reload sequence is documented in MANUAL.md §Timer1.
- ADC VP6 channel selection can disturb the HFINTOSC (item 3);
  documented in the ADC driver.
- MSSP SPI host: TMR2/2 short first pulse (2), fast-reload write
  collision (4), disable glitch (11); workarounds in the driver.
- ECCP dead-time above the duty cycle gives unpredictable waveforms
  (12); keep PDC below the duty.

## Devices

| Device | Flash | RAM | EEPROM | ADC | Ports |
|---|---|---|---|---|---|
| PIC16F882 | 2K | 128 B | 128 B | 11 | A-C |
| PIC16F883 | 4K | 256 B | 256 B | 11 | A-C |
| PIC16F884 | 4K | 256 B | 256 B | 14 | A-E |
| PIC16F886 | 8K | 368 B | 256 B | 11 | A-C |
| PIC16F887 | 8K | 368 B | 256 B | 14 | A-E |

The 882's 128 B RAM is the tightest budget in the family; some
epic-* modules may not fit and are excluded in the manifest with real
reasons.

## Docs

- `MANUAL.md`: datasheet-cited register/peripheral reference.
- `epic-common/MANUAL.md`: the shared contract (naming, handle
  pattern, harness, interrupt model).
- `docs/adding-a-device.md`: the verification-gated playbook this
  family was added through.
