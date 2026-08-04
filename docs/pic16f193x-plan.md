# PIC16F193X family addition

Status: **foundation, Timer1, Timer2/4/6, and the §4 codegen probe all
clear. Timer1 cleared the §4 register-readback half of the gate (Task
11 fix-round-1): the PIE1/2/3 inline-asm fix makes `mdb` report
`PIE1=0x01`. Timer2/4/6 has now also cleared the §4 gate: the
`HARNESS=sim` build reports PASS via `MODE=gpio` on `PORTA` bit 0, the
6-part real-target XC8 build all PASS, and `mdb` register readback
confirms T2CON=0x04, T4CON=0x05, T6CON=0x1E, PR2=0xC7, PR4=0x7C,
PR6=0x31, PIE1=0x02, PIE3=0x0A (TMR4IE | TMR6IE), PIR1=0x02, PIR3=0x0A
(TMR4IF | TMR6IF), TRISC=0xF8, LATA=0x01. PIE3=0x0A is the first PIE3
source verified by the §4 gate, confirming the Finding 2 inline-asm fix
works for the third `pir_index` branch (`iorwf PIE3,f`), not just PIE1.
§7 now has one detailed implementation plan doc per remaining
peripheral, all 13 written
(`docs/superpowers/plans/2026-08-04-pic16f193x-*.md`): Timer2/4/6
(done), CCP1/2, EUSART, MSSP, ADC, Comparator, EEPROM, DAC, FVR, SR
latch, CPS, LCD, CCP3/4/5. Next: execute the remaining 12 one at a time
through the §4 gate, in the order §7's table lists, confirming each
with the user before starting. The LCD plan (peripheral #12) has one
explicitly flagged open risk (the seg-to-register bit mapping) that
must be resolved from the DFP header before that plan's driver code
ships, see its own file.**
Device identity confirmed (§1), Path B confirmed (§2), scope set (§3).
The foundation (platform headers, SFR map, IRQ backend, dispatch, ISR
vector, harness, WDT/Sleep, GPIO, Timer0, host sim) builds clean with
`-Wall -Wextra -Werror` and passes its host examples across all six
parts (commit `786e9db`). The `Microchip.PIC12-16F1xxx_DFP` is now
installed and the real-target XC8 build passes for all six parts (§4).
The XC8 codegen probe of the two known-risky SFR-access patterns came
back clean (`pic16f193x-hal/docs/ARCHITECTURE.md` Finding 1): runtime
SFR-address dispatch routes through FSR1:INDF1 (BSR-independent by
construction), literal tokens in non-mirrored banks get a correct
`movlb`.

The toolchain gap is closed: MPLAB X / `mdb` is installed
(`docker/ci-toolchain/Dockerfile`, pushed to the private
`ghcr.io/apojomovsky/pic8-hal-ci` GHCR image) and confirmed working via
the root `Makefile`'s `make mdb-test`, real `PIC8_HARNESS_RESULT: PASS`
against `pic8-tick`'s pilot module on both existing families. Timer1
has since cleared the §4 gate for this family too (Task 11
fix-round-1): `make mdb-test ... MODE=gpio WAIT_MS=60000` produces a
real `PIC8_HARNESS_RESULT: PASS`, with `mdb` register readback
confirming `PIE1=0x01`. What is **not** yet done: the other two
`pir_index` branches (PIE2/PIE3, covering TMR2/4/6, CCP1-5, SSP,
USART TX/RX, ADC, TMR1G, LCD, BCL, EEPROM, CMP1/2, OSF) still need the
same register-readback verification before any peripheral routed
through them is marked done; each new peripheral needs its own
`example_<periph>.c` and the same PORTA-bit-0-or-PASS-marker protocol
(`MODE=gpio`, since this family doesn't have a `HARNESS=sim`
EUSART-reporting build). That is real, undone work for whoever picks
up the next peripheral (§7), not a solved detail.

This plan follows `docs/adding-a-device.md` (the operational procedure,
which supersedes `docs/multi-family-plan.md`'s "add family #3" checklist).
Solved vs. pending framing is at the end of each section.

## §1. Device identity (solved)

Datasheet: **DS41364B**, "PIC16F193X/LF193X Data Sheet, 28/40/44-Pin
Flash-Based, 8-Bit CMOS MCU with LCD Driver and nanoWatt XLP Technology"
(Microchip). Local copy: `/home/alexis/Downloads/41364B.pdf` (not
committed; `*.pdf` is gitignored). Official link:
<https://ww1.microchip.com/downloads/en/DeviceDoc/41364B.pdf>.

Note on revisions: the local copy is the preliminary **rev B** that
documented all six 193X parts in one file. Current Microchip revisions
split the family across three datasheets: DS41364C (1934/1936/1937),
DS40001574D (1938/1939), DS41575 (1933). The SFR/peripheral register
layout is identical across all six (SFRs live in banks 0-7, common to
all), so one HAL family citing the local DS41364B for register facts
covers every variant. Where a register fact differs per flash-size
group, the plan cites the specific table.

Errata: DS80000479 (1934/36/37) flags real silicon issues to honor when
those peripherals are built: ADC may not complete at FOSC > 8 MHz; ECCP
0%-duty direction-change and port-steering issues; Timer1 gate toggle
issues; EUSART auto-baud SPBRG bug; MSSP SPI master BF/SSPIF set half SCK
early (CKE=0). Recorded here so the peripheral phases don't miss them.

**PIC16F1937** (the part the user named; 40/44-pin, 36 I/O):

| Field | Value |
|---|---|
| Flash | 8192 words, last addr 1FFFh |
| RAM | 512 bytes |
| EEPROM | 256 bytes |
| 10-bit ADC | 14 channels |
| LCD | 24 segments / 4 commons |
| Packages | PDIP-40 (P), TQFP-44 (PT), QFN-44 (ML) |
| Voltage | F = 1.8-5.5V, LF = 1.8-3.6V |
| Temp | I = -40..+85C, E = -40..+125C |

No SSOP/SOIC/Skinny-DIP on the 40/44-pin part; those are 28-pin
(1933/1936/1938) only.

Whole family (DS41364B, all six), per-device sizes:

| Device | Flash (words) | RAM (B) | EEPROM (B) | I/O | Pin | LCD seg |
|---|---|---|---|---|---|---|
| 1933 | 4096 | 256 | 256 | 25 | 28 | 16 |
| 1934 | 4096 | 256 | 256 | 36 | 40/44 | 24 |
| 1936 | 8192 | 512 | 256 | 25 | 28 | 16 |
| 1937 | 8192 | 512 | 256 | 36 | 40/44 | 24 |
| 1938 | 16384 | 1024 | 256 | 25 | 28 | 16 |
| 1939 | 16384 | 1024 | 256 | 36 | 40/44 | 24 |

All have 256 bytes EEPROM. Solved.

## §2. Path decision (solved): Path B, new family

The 193X is the Enhanced Mid-range core, architecturally distinct from
both existing families, so `adding-a-device.md` §2 puts it on Path B
(new family from scratch), not a variant of `pic16f87xa-hal`. Confirmed
with the user (it changes the work by an order of magnitude vs. Path A).

Architecture, the parts that drive the HAL bodies:

- **Addressing**: BSR (Bank Select Register) selects one of up to 32
  banks of 128 bytes. Each bank = 12 core registers (0x00-0x0B) + 20 SFR
  (0x0C-0x1F) + 16 common registers mirrored in every bank (0x20-0x2F) +
  up to 80 GPR (0x30-0x7F). FSR0/FSR1 give 16-bit indirect addressing;
  linear data memory (FSRxH<7:5>='001') chains the 80-byte GPR regions
  into one contiguous block. Not classic PIC16's RP0/RP1 4-bank scheme,
  not PIC18's 96-byte Access Bank (the 193X has only the 16-byte common
  mirror). `MOVLB` loads BSR directly.
- **Interrupts**: single vector at 0x0004, **no priority** (GIE/PEIE
  only), 23 sources. INTCON holds TMR0/INT/IOC; PIR1/PIE1 = TMR1G, AD,
  RC, TX, SSP, CCP1, TMR2, TMR1; PIR2/PIE2 = OSF, C2, C1, EE, BCL, LCD,
  CCP2; PIR3/PIE3 = CCP5, CCP4, CCP3, TMR6, TMR4. **Automatic context
  save** to shadow registers in bank 31 on entry: W, STATUS (except
  TO/PD), BSR, FSR0, FSR1, PCLATH; `RETFIE` restores them and sets GIE.
  No manual push/pop. `HAL_IRQ_SetPriority` is a no-op, like classic
  PIC16.
- **I/O model**: PORTx/TRISx/LATx/ANSELx/WPUB/WPUE/IOCBP/IOCBN/IOCBF.
  The LAT latch (classic PIC16 lacks it) avoids read-modify-write
  hazards; ANSEL gives per-pin analog select.
- **Instruction set**: 49 instructions, 16-level stack (classic PIC16:
  35 instructions, 8-level). Adds MOVLB, MOVLP, ADDFSR, ADDWFC, SUBWFB,
  ASRF, LSLF, LSRF, BRA, BRW, CALLW, MOVIW, MOVWI, RESET. **MOVFF is
  PIC18-only**, not present here; register-to-register moves go through W
  or FSR/INDF.

SFR map (DS41364B Table 2-4, banks 0-7; banks 8-15 are GPR/linear only,
Table 2-5):

- Core (0x00-0x0B, every bank): INDF0, INDF1, PCL, STATUS, FSR0L/H,
  FSR1L/H, BSR, WREG, PCLATH, INTCON.
- Bank 0 (0x0C-0x1F): PORTA-E, PIR1/PIR2/PIR3, TMR0, TMR1L/H, T1CON,
  T1GCON, TMR2, PR2, T2CON, CPSCON0/1.
- Bank 1 (0x8C-0x9F): TRISA-E, PIE1/PIE2/PIE3, OPTION, PCON, WDTCON,
  OSCTUNE, OSCCON, OSCSTAT, ADRESL/H, ADCON0/1.
- Bank 2 (0x10C-0x11F): LATA-E, CM1CON0/1, CM2CON0/1, CMOUT, BORCON,
  FVRCON, DACCON0/1, SRCON0/1, APFCON.
- Bank 3 (0x18C-0x19F): ANSELA/B/D/E, EEADRL/H, EEDATL/H, EECON1/2,
  RCREG, TXREG, SPBRGL/H, RCSTA, TXSTA, BAUDCON.
- Bank 4 (0x20C-0x21F): WPUB, WPUE, SSPBUF, SSPADD, SSPMSK, SSPSTAT,
  SSPCON1/2/3.
- Bank 5 (0x28C-0x29F): CCPR1L/H, CCP1CON, PWM1CON, CCP1AS, PSTR1CON,
  CCPR2L/H, CCP2CON, PWM2CON, CCP2AS, PSTR2CON, CCPTMRS0/1.
- Bank 6 (0x30C-0x31F): CCPR3L/H, CCP3CON, PWM3CON, CCP3AS, PSTR3CON,
  CCPR4L/H, CCP4CON, CCPR5L/H, CCP5CON.
- Bank 7 (0x38C-0x39F): IOCBP, IOCBN, IOCBF.
- Bank 8 (0x40C-0x41F, partial): NOT purely GPR/linear despite the
  Table 2-5 assumption above; TMR4/PR4/T4CON (0x415-0x417) and
  TMR6/PR6/T6CON (0x41C-0x41E) live here, confirmed via the installed
  DFP header when Timer2/4/6 was implemented
  (`docs/superpowers/plans/2026-08-04-pic16f193x-timer246.md`).

Peripheral modules (DS41364B): I/O Ports + Interrupt-on-Change, Oscillator
(INTOSC + Fail-Safe Clock Monitor), SR Latch, ADC (10-bit), Comparator
(C1/C2), DAC, FVR, Timer0, Timer1 (+ gate), Timer2/4/6, Capacitive
Sensing, 5x CCP (ECCP1/2/3 enhanced, CCP4/5 standard), EUSART, LCD
segment driver, MSSP (SPI/I2C + SMBus), Data EEPROM + Flash self-write,
Sleep, ICSP. Solved.

## §3. Scope (solved, user-approved 2026-08-03)

- **Variants**: the whole 193X family (1933/1934/1936/1937/1938/1939, F
  + LF) via per-device capability macros, matching the repo's multi
  variant convention.
- **Peripherals**: foundation first (platform headers, SFR map, harness,
  IRQ backend + dispatch + vector, WDT/Sleep, GPIO + IOC, Timer0), host
  verified; then peripherals one at a time, each through the §4 gate.
- **Toolchain**: the user handles the `Microchip.PIC12-16F1xxx_DFP` +
  `mdb` install. I build and verify via the host simulation backend now;
  the real-target XC8 build and the §4 `mdb` register-readback gate are
  deferred until the toolchain lands. Per the playbook, nothing counts as
  done until the `mdb` gate passes, so each foundation piece and
  peripheral is tracked as `host-verified, mdb-pending`.

## §4. Blocker: DFP / mdb (resolved: both installed and confirmed working)

`adding-a-device.md` §1.3 requires the part to be in a pinned DFP; the
1937 was not. It lives in `Microchip.PIC12-16F1xxx_DFP`, which the user
downloaded (`Microchip.PIC12-16F1xxx_DFP.1.9.258.atpack`) and which is
now installed locally at
`/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC12-16F1xxx_DFP/` (both
the flat layout and the versioned `Microchip/PIC12-16F1xxx_DFP/1.9.258/`
layout, matching the existing PIC16Fxxx_DFP/PIC18Fxxxx_DFP convention).
Confirmed present: `edc/PIC16F1937.PIC`, `xc8/pic/include/proc/pic16f1937.h`,
`xc8/pic/dat/cfgdata/16f1937.cfgdata`, and the matching files for all
six parts (1933/34/36/37/38/39, F+LF).

**Real-target XC8 build: now passing for all six parts** (`make
MCU=16F1933..1939`), each producing a valid Intel-HEX firmware image.
One datasheet/DFP disagreement found and fixed here, exactly the kind
`adding-a-device.md`'s "flag it, don't guess" rule anticipates: the
`DEBUG` config-word field the initial Makefile draft emitted
(`#pragma config DEBUG = OFF`) is marked `islanghidden="true"` in the
DFP's `PIC16F1937.PIC` (reserved for debugger tooling, not a
user-settable `#pragma config`); XC8 rejects it with error 1363. Removed
from the Makefile's generated config-word recipe; the other 12
directives (FOSC/WDTE/PWRTE/MCLRE/CP/CPD/BOREN/CLKOUTEN/IESO/FCMEN/LVP/
STVREN/PLLEN/WRT) are all confirmed non-hidden in the DFP and compile
clean.

**`mdb` (MPLAB SIM, headless): now installed and confirmed working.**
User supplied the MPLAB X IDE installer; `docker/ci-toolchain/Dockerfile`
now builds the full image (XC8 + all three DFPs + MPLAB X), pushed to
the private `ghcr.io/apojomovsky/pic8-hal-ci` GHCR package that
`xc8-build.yml`/`sim-tests.yml` pull. The root `Makefile`'s `make
mdb-test` was run for real against `pic8-tick`'s pilot module (both
PIC16F87XA and PIC18F4550), both reaching a genuine
`PIC8_HARNESS_RESULT: PASS`. See `docs/docker-dev-plan.md` for the full
account. The real-target build passing was necessary but not sufficient
per the playbook; that half is now also closed.

**Not yet done (parked concerns)**: Timer1 cleared the §4 gate for the
PIE1 branch only. The other two `pir_index` branches (PIE2 and PIE3,
covering TMR2/4/6, CCP1-5, SSP, USART TX/RX, ADC, TMR1G, LCD, BCL,
EEPROM, CMP1/2, OSF) still need the same §4 register-readback
verification before their peripherals are marked done. `make mdb-test`
MODE=gpio works for Timer1, but each new peripheral needs its own
example_<periph>.c + the same PORTA-bit-0-or-PASS-marker protocol.
Also: the brief's default `WAIT_MS=2000` is too short on the Docker
MPLAB SIM (~1/2000th real-time); `WAIT_MS=60000` is the working value
documented in the MANUAL + adding-a-device.md.

## §5. Foundation design (solved, user-approved)

New tree `pic16f193x-hal/` cloned from `pic16f87xa-hal/` (closest by
interrupt architecture: single vector, no priority, table-driven flag/
enable). The BSR banking is written fresh in the SFR/platform layer. The
fixed contract (shared names/signatures, neutral-shim headers, the
`PIC8_*` CMake/Makefile variable contract) is implemented unchanged.

Foundation deliverables (everything needed for a minimal blink + host sim
+ `mdb`-smoke-ready skeleton):

1. `include/pic16f193x.h` device selection + per-device capability macros
   (`PIC16F193X_FAMILY_HAS_PORTD`/`_PORTE` on 40/44-pin 1934/1937/1939
   only; `FLASH_KW`, `RAM_BYTES`, `EEPROM_B`, `ADC_CH`, `DEVICE_NAME`,
   `PIC8_FAMILY_RAM_BYTES` alias).
2. `include/pic16f193x_sfr.h` SFR addresses/bit masks/POR values from
   DS41364B Table 2-4/2-5, cited. Bank-select helper via BSR (`MOVLB` on
   target, no-op flat-array on host).
3. `include/host/pic16f193x_platform.h` +
   `include/target/pic16f193x_platform.h` access-macro pair (host flat
   array + `PIC8_WEAK=weak`; target volatile deref + `PIC8_WEAK` empty),
   define `PIC8_REG8`, `pic8_sfr_read8/write8`, `PIC8_SFR_PTR`, target
   bank-switch inline-asm. Every SFR access stays a compile-time-constant
   `PIC_REG_*` token (the proven pattern from `pic18_irq.c`/
   `pic18fxx5x_ccp.c`); runtime dispatch branches before touching any
   SFR.
4. `include/core/pic16f193x_irq.h` + `src/core/pic16f193x_irq.c` IRQ
   backend: `PIC16F193X_IRQn` (23 sources) +
   `HAL_IRQ_Disable/Restore/Enable/DisableSrc/ClearFlag/GetFlag/SetPriority`,
   table-driven descriptor extended to 3 PIR/PIE banks (`pir_index`
   0/1/2 for PIR1/2/3, INTCON-level for TMR0/INT/IOC). `SetPriority` =
   no-op. PIE1/2/3 in bank 1 via the PIE-bit macros.
5. `src/core/pic16f193x_irq_dispatch.c` (`pic8_dispatch_all_irqs` strong
   extern fan-out) + `src/core/pic16f193x_isr_vector.c` (single
   `__interrupt()` at 0x0004, hardware auto-saves context).
6. `src/core/pic16f193x_harness_sim.c` (host harness pumps sim, registers
   `pic8_dispatch_all_irqs` as the IRQ callback) + shared
   `pic8-common/.../pic8_harness_target.c` target no-op.
7. `include/core/pic16f193x_wdt_sleep.h` +
   `src/core/pic16f193x_wdt_sleep.c`/`_sim.c`/`_target.c`.
8. `include/peripherals/pic16f193x_gpio.h` + `src/peripherals/pic16f193x_gpio.c`
   (PORTA-E, LAT, TRIS, ANSEL, WPUB/WPUE, IOC handle pattern) +
   `pic16f193x_timer0` (for blink).
9. Neutral shims `pic8_hal.h`, `core/hal_irq.h`, `core/hal_wdt_sleep.h`,
   `peripherals/hal_gpio.h`, `peripherals/hal_timer0.h`.
10. `include/pic16f193x_sim.h` + `src/sim/pic16f193x_sim.c` host sim: flat
    array register file, `sim_reset/step/drive_input/read_output/
    set_irq_callback`; models Timer0 overflow + GPIO + IOC enough for
    blink and host tests.
11. `tests/example_blink.c` (Timer0 + GPIO + IRQ, dual-build, header
    documents the expected register image for the §4 gate) +
    `tests/example_smoke.c` (host seam test).
12. `CMakeLists.txt` (host build) + `mcu/pic16f193x-mplabx/Makefile`
    (device-selection chain for all six parts, `DFP_DIR` for
    `Microchip.PIC12-16F1xxx_DFP`, target-first include path, the §10
    `#pragma config` recipe for CONFIG1/CONFIG2).
13. `README.md`, `MANUAL.md` (per-family register ref, points to
    `pic8-common/MANUAL.md` for shared conventions), `docs/ARCHITECTURE.md`
    (codegen findings, filled as the §4 gate surfaces them).

Solved. Foundation implemented, host-verified and committed (`786e9db`);
real-target build verified (§4); `mdb` gate not yet run for this family
(§4, §6).

## §6. Verification (partly solved)

- **Host sim** (solved): `cmake -B build && cmake --build build`, then
  running each example directly (the shared `pic8_family.cmake` doesn't
  register `ctest` targets; examples self-report pass/fail via
  `pic8_harness_report`'s exit code). Clean with `-Wall -Wextra -Werror`
  for every foundation piece. Catches logic bugs; does not catch the
  codegen bugs the gate exists for.
- **Real-target XC8 build** (solved): `make MCU=16F193{3,4,6,7,8,9}` all
  build clean and produce a valid Intel-HEX image with the
  `Microchip.PIC12-16F1xxx_DFP` installed (§4). One datasheet/DFP
  disagreement found and fixed (the hidden `DEBUG` config field, §4).
- **`mdb` register readback** (Timer1 PASS, other peripherals pending):
  Timer1 cleared the §4 gate via `make mdb-test ... MODE=gpio
  WAIT_MS=60000`, with `mdb` reporting `PIE1=0x01` (TMR1IE bit 0 set)
  and `PORTA` bit 0 transitioning high so the harness reports
  `PIC8_HARNESS_RESULT: PASS` (Task 11 fix-round-1). The other
  `pir_index` branches (PIE2/PIE3) still need the same register-readback
  verification before any peripheral routed through them is marked done.
  Each new peripheral gets its own `example_<periph>.c` and the same
  PORTA-bit-0-or-PASS-marker protocol. The underlying §4 mechanism
  (`stepi <N>` + `print <REGISTER>` via `mdb.sh`,
  `docs/adding-a-device.md` §4.6) is what `make mdb-test` wraps.
- **XC8 codegen probe** (solved, with one PIE1/2/3 RMW bug found and
  fixed): disassembled the linked `example_blink` firmware and confirmed
  the read-only SFR-dispatch patterns are safe on this core
  (`pic16f193x-hal/docs/ARCHITECTURE.md` Finding 1). The RMW half
  (Finding 2) silently failed under XC8 v3.10: `FSR1=0x0091` writes to
  a GPR byte, not the banked mirror of PIE1; fixed by switching to
  `__at(0x70)` scratch + inline-asm `movlb 1` / `iorwf PIE1,f` /
  `movlb 0`, mirroring `pic16f87xa-hal`'s proven shape. `mdb`
  register-readback confirmed the fix (`PIE1=0x01` after the fix,
  `PIE1=0x00` before).

## §7. Peripheral roadmap (pending, one detailed plan doc per peripheral)

After the foundation, GPIO, Timer0, and Timer1 (all cleared, §4-§6), 13
peripheral groups remain, covering every module DS41364B documents for
this family (§2's peripheral list). Each gets its own plan doc under
`docs/superpowers/plans/`, written to the same rigor as Timer1's (design
+ full `Task N` code-level breakdown, `docs/superpowers/plans/2026-08-03-pic16f193x-phase0-timer1.md`
is the template every new plan mirrors) so an implementer with no prior
context on this codebase can execute it. Each plan gets its own
`example_<periph>.c` with a hand-computed expected register image (the
`example_timer1.c` header-comment pattern), host sim step function,
real-target XC8 build, and the `mdb` `MODE=gpio` register-readback half
of the §4 gate (mandatory for every 193X peripheral until an EUSART
`HARNESS=sim` build exists, since there is no UART-reporting path yet).

**Standing rule for every plan below, non-negotiable**: every register
bit position must be re-derived from DS41364B's own register table for
that specific SFR and cross-checked against the installed DFP header
(`/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC12-16F1xxx_DFP/xc8/pic/include/proc/pic16f1937.h`,
`_<REG>_<FIELD>_POSN`/`_MASK` macros), never transcribed from another
family's driver or from memory. This is not theoretical: Timer1 shipped
with `PIC_T1CON_TMR1CS` at bit 1 (correct for `pic16f87xa`'s classic
PIC16 T1CON, wrong here; the real field is bits 7:6) because it was
copied without re-verifying, caught only by the §4 gate. Every plan doc
below embeds the exact verified addresses/bit positions already
extracted from the DFP header so the implementer never has to guess.

**MANUAL.md section numbers** are assigned in landing order starting at
§12 (Timer1 is §11); `pic16f193x-hal/docs/ARCHITECTURE.md` §5 notes the
renumbering cascade risk if peripherals land out of this order, i.e. if
peripheral N+2 lands before N+1, renumber on merge, don't leave a gap or
a collision. `src/core/pic16f193x_irq_dispatch.c` needs exactly one
`extern`+call addition per peripheral (`include/core/pic16f193x_irq.h`'s
23-source table and `src/core/pic16f193x_irq.c`'s descriptor array are
already fully populated for all pending peripherals, zero changes
needed there). Each `pic16f193x-hal/CMakeLists.txt` addition is one
`HAL_SOURCES` line + one `pic8_add_example[_per_device]` call, same
shape as Timer1's.

| # | Peripheral group | Plan doc | Registers (bank) | MANUAL § | Reference driver to mirror |
|---|---|---|---|---|---|
| 1 | Timer2/4/6 | `docs/superpowers/plans/2026-08-04-pic16f193x-timer246.md` | TMR2/PR2/T2CON (bank 0); TMR4/PR4/T4CON, TMR6/PR6/T6CON (bank 8, **not yet in `pic16f193x_sfr.h`**, addresses newly confirmed via DFP: 0x415-0x417, 0x41C-0x41E) | §12 | `pic16f87xa_timer2.{h,c}` |
| 2 | CCP1/CCP2 (ECCP1/ECCP2) | `docs/superpowers/plans/2026-08-04-pic16f193x-ccp12.md` | CCPR1-2L/H, CCP1-2CON, PWM1-2CON, CCP1-2AS, PSTR1-2CON, CCPTMRS0/1 (bank 5) | §13 | `pic18fxx5x_ccp.{h,c}` (PIC18's ECCP1/plain-CCP2 split matches this family's shape; `pic16f87xa_ccp.{h,c}` is plain-CCP-only, not a good template here) |
| 3 | EUSART | `docs/superpowers/plans/2026-08-04-pic16f193x-eusart.md` | RCREG/TXREG/SPBRGL/H/RCSTA/TXSTA/BAUDCON (bank 3) | §14 | `pic16f87xa_usart.{h,c}` |
| 4 | MSSP (SPI + I2C) | `docs/superpowers/plans/2026-08-04-pic16f193x-mssp.md` | SSPBUF/SSPADD/SSPMSK/SSPSTAT/SSPCON1-3 (bank 4) | §15 | `pic16f87xa_ssp.{h,c}` |
| 5 | ADC | `docs/superpowers/plans/2026-08-04-pic16f193x-adc.md` | ADRESL/H/ADCON0/1 (bank 1) | §16 | `pic16f87xa_adc.{h,c}` |
| 6 | Comparator (C1/C2) | `docs/superpowers/plans/2026-08-04-pic16f193x-comparator.md` | CM1CON0/1, CM2CON0/1, CMOUT (bank 2) | §17 | `pic16f87xa_comp.{h,c}` (single-`CMCON` shape; this family splits into per-comparator CM1CON0/1 + CM2CON0/1 + a shared CMOUT, adapt the shape, don't copy the register count) |
| 7 | EEPROM | `docs/superpowers/plans/2026-08-04-pic16f193x-eeprom.md` | EEADRL/H/EEDATL/H/EECON1/2 (bank 3) | §18 | `pic16f87xa_eeprom.{h,c}` |
| 8 | DAC | `docs/superpowers/plans/2026-08-04-pic16f193x-dac.md` | DACCON0/1 (bank 2) | §19 | none in either reference family, new shape (small: 2 registers) |
| 9 | FVR | `docs/superpowers/plans/2026-08-04-pic16f193x-fvr.md` | FVRCON (bank 2) | §20 | none in either reference family, new shape (single register) |
| 10 | SR latch | `docs/superpowers/plans/2026-08-04-pic16f193x-srlatch.md` | SRCON0/1 (bank 2) | §21 | none in either reference family, new shape |
| 11 | Capacitive sensing (CPS) | `docs/superpowers/plans/2026-08-04-pic16f193x-cps.md` | CPSCON0/1 (bank 0) | §22 | none in either reference family, new shape |
| 12 | LCD segment driver | `docs/superpowers/plans/2026-08-04-pic16f193x-lcd.md` | LCDCON/LCDPS/LCDREF/LCDCST/LCDRL/LCDSE0-2/LCDDATA0-11 (bank 15, 0x790-0x7AB, **not yet in `pic16f193x_sfr.h`**, newly confirmed via DFP) | §23 | none in either reference family, largest and highest-risk of the 13 (24 segments / 4 commons on the 1937/1939, fewer on 28-pin parts, needs its own per-device segment-count capability macro) |
| 13 | CCP3/CCP4/CCP5 | `docs/superpowers/plans/2026-08-04-pic16f193x-ccp345.md` | CCPR3-5L/H, CCP3CON, PWM3CON, CCP3AS, PSTR3CON (bank 6, ECCP3), CCP4CON, CCP5CON (bank 6, **plain, no PWM/AS/PSTR registers**, do not assume ECCP shape for 4/5) | §24 | `pic18fxx5x_ccp.{h,c}` for CCP3 (ECCP), `pic16f87xa_ccp.{h,c}` for CCP4/5 (plain); shares the peripheral #2 driver's handle-shape decisions, so land after it |

Errata to carry into the relevant plan (DS80000479, full detail already
in §1): ADC plan must gate/warn on FOSC > 8 MHz; CCP1/2 and CCP3/4/5
plans must flag ECCP1-3's 0%-duty direction-change and port-steering
issues; EUSART plan must avoid/flag auto-baud-detect mode (SPBRG bug);
MSSP plan must note the SPI-master BF/SSPIF-half-SCK-early behavior
when CKE=0.

Status per plan doc: **Timer2/4/6 done (cleared the §4 gate); the other
12 written, not yet implemented** until each is executed and its
peripheral clears the §4 gate; this table's own Status line and each
peripheral's own plan-doc Status line are the source of truth as they
land. The landing order above is a recommendation (dependency-free
peripherals can go in any order); confirm with the user before starting
each one, same as Timer1.

## §8. Risks (from the playbook appendix)

- SFR access while a BSR bank switch is in effect, via a plain C
  local/parameter: confirmed on classic PIC16, unverified on enhanced
  mid-range. The §4 gate + the codegen probe cover this.
- SFR address that is a runtime variable/struct-field/parameter at the
  point of access: confirmed on PIC18. Mitigation unchanged: branch
  before touching any SFR, literal `PIC_REG_*` token per branch.
- Baud-rate/timing divisor math silently overflowing the register width:
  confirmed on PIC18. Sanity-check computed divisors against the
  register width for EUSART/Timer peripherals.
- Missing `HARNESS=sim` -> watchdog-off override: confirmed on both
  families. Carry the same Makefile override.
- Dangling pointer in a HAL `_Init` storing the caller's pointer instead
  of copying the handle: confirmed on PIC16. The handle-copy pattern is
  already the repo's convention; follow it.

## §9. Sign-off checklist (from `adding-a-device.md` §6)

- [ ] Every peripheral passed §4's full gate (host + real `mdb`).
- [ ] `pic16f193x-hal/MANUAL.md` covers every peripheral touched,
      datasheet-cited.
- [ ] `pic16f193x-hal/docs/ARCHITECTURE.md` records any codegen finding
      with XC8 User's Guide citations.
- [ ] `scripts/ci-discover-xc8-matrix.py` reflects the new family; any
      `KNOWN_BROKEN` entries documented in `docs/mplabx-link-gaps-plan.md`.
- [ ] Full regression: every module, every 193X variant, host and real
      target, immediately before the final commit.
- [ ] User signs off before anything is pushed.

## References

- `docs/adding-a-device.md` (operational procedure, supersedes
  `multi-family-plan.md`'s add-family checklist).
- `docs/multi-family-plan.md` (the fixed contract; open questions are a
  historical PIC18 record).
- `pic8-common/README.md` + `pic8-common/MANUAL.md` (shared conventions).
- DS41364B §2.2 (data memory), §4 (interrupts), §6 (I/O ports), §8
  (oscillator), §10 (device config), §11-§22 (peripherals), §26
  (instruction set).
