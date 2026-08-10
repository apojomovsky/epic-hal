# Toolchain coverage and pre-toolchain bug inventory

Status: **audit complete 2026-08-09** (feeds
`docs/superpowers/plans/2026-08-09-toolchain-validation-hunt.md`). This
is the audit report (Phase 0 of the hunt) and the probe checklist for
Phases 1-2. Every SUSPECT site below is a candidate real bug in code
that was written without the real toolchain at hand; each is either
verified by a probe (SAFE) or fixed (FIXED). Nothing is assumed safe
from source reading alone.

## Coverage matrix

Gate key: `mdb` = runs under XC8 + MPLAB SIM via
`scripts/sim-mdb-run.sh` and greps the `EPIC_HARNESS_RESULT` marker.
Compile = real-target build in the CI matrix (manifest-driven).

| Module | Host ctest | Real-target compile | mdb gate | Notes |
|---|---|---|---|---|
| epic-tick | yes | 87XA (4) + PIC18 4455/4550 | 16F877A + 18F4550 | reference gate pair |
| epic-swuart | 6 tests | 87XA 876A/877A, PIC18 (4), 193X (6) | 16F877A | RX path not simulatable (no pin injection) |
| epic-pic16f193x-firmware | no | 193X (6) | 16F1937 (gpio) | bare-HAL smoke |
| epic-math | 8 tests | 87XA (4) + PIC18 4455/4550 | **16F877A smoke + 18F4550 full replay** | PIC18 PASS=62 FAIL=0 golden vectors; PIC16 smoke (layout-limited) |
| epic-serial | 1 test | 87XA 876A/877A, PIC18 4455/4550 | **16F877A PASS** (4870w) | TX path + class-G paths under live ISR; RX not simulatable |
| epic-taskmgr | 2 tests (added) | 87XA 876A/877A, PIC18 4455/4550 (re-enabled) | **18F4550 PASS** (7378B) | PIC16 Timer0-tick ISR storm + GIE persistence findings |
| epic-modbus | 3 tests | PIC18 4455/4550 only | **18F4550 PASS** (16392B) | TX framing + CRC verified in uart1io capture |
| epic-bus | 1 test | 87XA (4), PIC18 4455/4550 | **16F877A PASS** (7848w) | MSSP data path not modeled by MPLAB SIM |
| epic-console | 2 tests | 87XA 876A/877A, PIC18 4455/4550 (re-enabled) | **18F4550 PASS** (12100B) | RX injection impossible (SCL stim crashes mdb) |
| epic-settings | 1 test | PIC18 4550 (re-enabled) | **18F4550 PASS** (7321B) | needs eeprom_writes runner arg (sim never completes EEPROM writes) |
| epic-adcfilter | 1 test | 87XA (4), PIC18 (4) | **16F877A PASS** (4215w) | |
| epic-debounce | 2 tests | 87XA 876A/877A, PIC18 4455/4550 | **16F877A PASS** (4076w) | |
| epic-pid | 9 tests | 87XA (4), PIC18 (4) | **18F4550 PASS** (8055B) | PIC16 gate moved (layout budget) |
| epic-encoder | 11 tests | 87XA (4), PIC18 4455/4550 | **16F877A PASS** (5555w) | class-G race did not manifest |
| epic-fsm | 1 test | all 3 families (14) | **16F877A PASS** (3116w) | |
| epic-lcd | 1 test | 87XA 16F877A (manifest added) | **16F877A PASS** (5170w) | 3 compile bugs fixed; fptable + stack findings |
| epic-usb | 2 tests | **not in manifest** | none | PIC18-only; USB not simulatable; host tests only |
| epic-sdcard | 1 test | **not in manifest** | none | PIC18-only; needs a block device; host tests only |
| pic16f87xa-hal | no | (pseudo-module) | **bank-probe PASS** (3082w) | permanent banked-SFR regression gate |
| pic18fxx5x-hal | no | (pseudo-module) | **gpio-probe PASS** (6623B) | class-C runtime-address verification |
| pic16f193x-hal | no | (firmware pseudo-module) | **16F1937 PASS** (gpio) | class-D route verified by existing gate |

## Audit findings (the bug inventory)

### A. Finding-9 class, 87XA: ungated `pic_select_bank` + plain `EPIC_REG8` (5 sites)

The Finding 9 follow-up fixed 8 sites but left the DeInit/read paths.
The banked-SFR probe (`pic16f87xa-hal/tests/sim_bank_probe.c`, permanent
gate) settled each one under mdb, 2026-08-09:

- The four DeInit write sites (ADC/COMP/USART/VREF) are **SAFE**:
  `pic_select_bank` is now an asm macro, so the tracking reset makes
  the following plain write's banksel correct.
- `EPIC_TIMER2_ReadPeriod` (PR2 read) was **corrupted** (returned the
  bank-0 alias SSPBUF value); **fixed** with `EPIC_BANK1_READ8`.

### B. SSP_ReadByte class, 87XA: plain Bank-1 SFR access with no bank select (~17 sites)

**Confirmed corrupted and fixed**, all sites, 2026-08-09 (the probe's
register evidence: PR2 read hit SSPBUF, OPTION_REG RMW read TMR0 and
wrote `count & ~0x20` back, TXSTA read hit RCSTA, SSP_ReadByte's
BF-clear hit SSPCON and cleared CKP, PCON reads hit PIR1). Plain Bank-1
*reads* and *RMWs* misdirect to their bank-0 aliases; plain *writes*
land. Fixed sites: `EPIC_SSP_ReadByte` (BF-clear), `EPIC_SSP_
IsBufferFull`, `EPIC_TIMER0_Init`/`DeInit` (OPTION_REG), `EPIC_USART_
Init` (TXSTA write), `GetTX9D`/`SetTX9D`/`IsTxShiftRegisterEmpty`,
`EPIC_GPIO_SetPullups` (OPTION_REG), the `wdt_sleep` PCON reads and
clears. The GPIO TRISx runtime-address path is **SAFE** (FSR-indirect,
probe-confirmed).

### B2. Flaws found inside the Finding-9 fix macros themselves (fixed)

- `EPIC_BANK1_*` and `EPIC_PIE_*` only managed RP0; an incoming
  RP1=1 state (observed right after the sim harness init, STATUS=0x41)
  routed their accesses to bank-3 GPR. Now bank-absolute (both RP
  bits), matching the `EPIC_BANK2/3_*` discipline.
- `EPIC_PIE_ENABLE/DISABLE_BIT`'s PIE2 branch never selected bank 2
  (it RMW'd the bank-1 alias of PIE2, PCON). Now selects bank 2.

### C. PIC18 Finding-3 class: runtime-computed SFR addresses (gpio.c)

**Verified SAFE** by the permanent `pic18fxx5x-hal` GPIO probe gate
(18F4550, 11 checks): the runtime-address TRIS/LAT/PORT path does not
exhibit the program-memory-table misdirection in this driver.

### D. 193X Finding-2 class: runtime-address writes to Bank-1/2/3 SFRs (gpio.c)

**Verified by the existing firmware-sim gate**: the 193X harness drives
RA0 through `EPIC_GPIO_WritePin` (runtime `lat_addr` route) and the
gpio-mode gate reads it back, so the LATA/TRISA class-D route works
under XC8. TogglePin/WritePort/DeInit remain covered by usage.

### E. TXIF un-gated dispatch branches (Finding 10(b) class)

**Fixed** (PIC18 `pic18_irq_dispatch.c:75` live defect, 193X
`pic16f193x_irq_dispatch.c:88` latent): both branches now gate on PIE1
TXIE like the 87XA reference (`EPIC_PIE1_READ_TXIE` added to the 193X
platform headers).

### F. Table-driven GetFlag/ClearFlag in ISR context (stringdir class)

`EPIC_IRQ_GetFlag/ClearFlag` dereference a ROM descriptor table
through XC8's `stringdir`/retlw route, which clobbers PCLATH when the
table sits on another flash page. Reachable today:

- 87XA: timer0, timer1, timer2, adc, comp, eeprom, gpio (RB), psp, ssp,
  usart (TX/RX) handlers (`EPIC_IRQ_GetFlag`/`ClearFlag` at the
  file:line pairs listed by the audit). CCP1/2 use the safe direct
  `EPIC_BIT_CLR` pattern.
- 193X: gpio (IOC), timer0, timer1, timer246 handlers. CCP1-5 safe.
- PIC18: none (switch-based compile-time-token API, Finding 3 fix).

### G. Disable/Restore GIE-race class (Finding 10(a))

MPLAB SIM (and per the tick investigation, hardware semantics) can
deliver a latched interrupt inside a `GIE=0` critical section, tearing
the protected read and leaving GIE cleared after ISR return.
`epic_tick_get` is the only site converted to read-twice-retry.
Remaining sites:

- `epic_serial.c:48-52` Disable/Restore INSIDE the TX ISR callback
  (plus a DisableSrc PIE1 RMW from ISR context)
- `epic_serial.c:97-106` blocking push with explicit dispatcher calls
- `epic_serial.c:115-121` read
- `epic_swuart.c:496-500,534-538,547-549` write/read/error-count
- `task_manager.c` 10 sites (spawn/start/stop/reset/set_period/ticks/
  run_once/one-shot/count)
- `encoder.c:96-98,105-107,113-115` 32-bit and 16-bit reads
- `pic16f193x_eeprom.c:39-43` EECON2 unlock sequence (the only
  HAL-side runtime disable)

### H. Unpinned bank-sensitive statics (risk set)

All IRQ-shared statics live in banked GPR (linker best-fit); none is
pinned except the two scratch bytes. Pure-C accesses get auto-banksel
(low residual risk), but the epic-math incident proved the class is
real when asm or multi-byte interleaving is involved. The multi-byte
struct copies (`g_t0_storage`, `g_ccp_callbacks[3]`, `g_handle[5]`,
`g_t1_handle`, `g_usart`) are the worst scatter cases. 87XA: 14
symbols. 193X: 6 symbols. PIC18: bank-agnostic, exempt.

### I. Coverage gaps (manifest)

- `epic-console`, `epic-settings`: `supported` is empty. The rc1
  exclusion reason (dispatch strong-references every peripheral
  handler, partial source lists) is structurally impossible now that
  the manifest uses full `hal_sources`; re-enable and verify.
- `epic-taskmgr`: PIC18 excluded (same stale rc1) and no host ctest.
- `epic-lcd`, `epic-usb`, `epic-sdcard`: not in the manifest at all;
  host ctests run, but no real-target build ever happens.

## Simulator and toolchain findings (2026-08-09, all evidence-backed)

The gate campaign also mapped the limits of the verification vehicle itself.
These are environment facts, not module bugs; gates are designed around them
and each is documented at its source (the sim files' headers):

- **No RX injection, period.** Firmware RCREG writes, RCIF writes, and mdb
  `set RCREG`/`set PIR1` are all masked by MPLAB SIM 6.35; the documented
  SCL `stim` + `packetin()` path fails to parse even the guide's minimal
  testbench and crashes the simulator; bit-banging a 9600 waveform onto the
  RX pin raises FERR without setting RCIF (the model's receive data path
  consumes a stimulus buffer, not the pin). All gates are TX-side or
  internal-state.
- **The MSSP data path is not modeled.** SSPIF/BF never set; SEN/PEN latch
  forever. SSPBUF captures written bytes (no WCOL). Bounded waits only.
- **CPU-executed data-EEPROM writes never complete** on either family (WR
  holds, EEIF never raises; the EECON2 write-only SFR never reaches the
  sim's lock observer). mdb `write` commands do complete them: the
  sim-mdb-run.sh `eeprom_writes` opt-in arg (default 0) emits
  halt+complete cycles for gates whose firmware blocks on EEIF.
- **TXIF never clears on TXREG write** under the sim, so TXREG readback is
  unreliable; verify TX from the uart1io capture instead.
- **Short tick periods starve the main loop** under the sim (ISR cost
  exceeds the period; TMR0IF pending at every retfie, ISR storm). Tick
  periods of ~1 ms and up behave.
- **GIE can be left cleared** after ISR return under the sim (the
  epic-tick Finding 10(a) class), observed on PIC16 Timer0-tick builds;
  PIC18 gates are unaffected.
- **PIC16F87XA sim builds have a layout budget**: the `__interrupt` body
  must stay in flash page 0 (the vector's PCLATH-less goto) and the
  hand-asm routines' internal gotos likewise; past ~4.2 K words the linker
  scatters them and aborts with fixup-overflow 1356. Family-agnostic pure-
  logic gates run on PIC18 instead (absolute calls, no constraint); the
  epic-math PIC16 gate uses the minimal smoke (see the manifest comment).
- **The lcd gpio4 transport's ops-function-pointer send** cannot be invoked
  under XC8 v4.00/PIC16 (fptable ABI + bank-1 frame collision parks the
  machine in bank 3); pin data paths need real hardware. The 8-level
  hardware stack also constrains the driver send path with a live tick ISR.

## Remaining follow-up (recorded, not done)

- The stringdir `GetFlag`/`ClearFlag` PCLATH-clobber class (class F):
  87XA (10 handlers) and 193X (4 handlers) still route ISR-path flag
  handling through the table; converting to the direct-flag pattern is the
  PR #9 follow-up hardening. No gate failure has exposed it, but it remains
  the documented latent hazard.
- `__at()` pinning of the PIC16 math asm routines below 0x800 would enable
  a full golden-vector PIC16 replay (currently layout-limited to the smoke).
- epic-usb and epic-sdcard real-target manifest entries (compile-only) are
  the remaining coverage gap; their host tests run in CI.

## Probe checklist (Phase 2), with resolutions

The probes became permanent gates; each is resolved:

P1. DeInit/read probe for the 5 class-A sites: known value through the
     API, SFR readback under mdb. **Done** (sim_bank_probe.c): DeInit
     writes SAFE, ReadPeriod fixed.
P2. Per-site probe for the class-B sites, plus `EPIC_SSP_ReadByte`.
     **Done** (sim_bank_probe.c): all class-B sites fixed.
P3. PIC18 GPIO probe: `EPIC_GPIO_WritePin`/`ReadPin` on both ports,
     register readback (the class-C route). **Done** (sim_gpio_probe.c):
     SAFE.
P4. 193X GPIO probe: same for TRISx/LATx writes (class-D route).
     **Covered by the existing firmware-sim gate** (the harness drives
     RA0 through the class-D route); no dedicated module added.
P5. PIC18 TX-path gate (class E). **Done**: the TXIE-gate fix landed and
     every PIC18 gate exercises it.
P6. ISR-load probes per family (the tick methodology): INTCON/PIR/
     PCLATH at halts under a timer-driven load, exercising the class-F
     handlers; then convert handlers to the direct-flag pattern.
     **Partial**: the serial/encoder/taskmgr gates run live-ISR loads
     (tick survived, no tear). The class-F stringdir conversion remains
     follow-up (see above).
P7. GIE-race probes: for each class-G site, hammer the protected read
     under a live tick ISR and check for torn values and GIE state.
     **Done** for epic-encoder (5000 hammer reads under a 1 ms tick:
     no tear, tick survived); the other class-G sites are exercised by
     their gates. The PIC16 GIE-persistence sim limitation is recorded
     above.
P8. epic-math: full golden-vector replay under mdb on PIC18; curated
     subset on PIC16. **Done**: PIC18 PASS=62 FAIL=0; PIC16 limited to
     the smoke by the layout budget (follow-up: __at pinning).

## Combination matrix (2026-08-09, all 12 gates PASS)

The combination campaign (docs/superpowers/plans/2026-08-09-
combination-matrix.md) added 12 permanent interleave gates
(epic-combo-*, 112/112 matrix builds, 32/32 sim gates, 6/6 bundles):

| Gate | Combo | Family | Notes |
|---|---|---|---|
| C1 | USART + SSP + EEPROM + TMR2 ISR | 87XA | EEPROM reads removed (inert 16F877A model); manual RP window exercises the preempted-bank ISR path |
| C2 | TMR0 + TMR1 + TMR2 + USART | 87XA | TMR0IF does not latch under the sim with PSA=0 (verified via the counter instead) |
| C3 | ADC + TMR1 + USART | 87XA | sim models the ADC (conversion completes, GO clears, ADIF sets) |
| C4 | RB change + TMR2 + USART | 87XA | polled-TX TSR-empty wait is an ISR-wedge landing zone (run loop TX-free) |
| C5 | EEPROM write + TMR2 ISR | PIC18 | 16F877A EEPROM model fully inert (writes never complete, RD never refreshes EEDATA); needs eeprom_writes |
| C6 | epic-tick + epic-serial | 87XA | byte-exact 50-line capture through the ring + dispatch |
| C7 | epic-tick + epic-settings | PIC18 | found the un-gated EEIF dispatch branch (see below) |
| C8 | epic-taskmgr + epic-serial | PIC18 | period ratios + ring push/pop exactness + byte-exact capture |
| C9 | epic-encoder + epic-tick | 87XA | unbounded tick delays freeze under the sim wedge (bounded probes) |
| C10 | epic-lcd + epic-tick | PIC18 | moved off 87XA: RAM edge + 8-level stack + wedge; PIC18 spins delays on the live tick |
| C11 | epic-swuart + epic-tick | 87XA | found the PIE2 bank regression (see Finding 11 correction); swuart/CCP 8-bit handle-pointer fragility fixed at source (CCP driver now stores driver-owned callbacks), gate runs unpinned |
| C12 | epic-modbus + epic-serial + epic-tick | PIC18 | frame byte-exact under a live tick; PIC18 has no TX-storm GIE wedge |

Bugs found and fixed by the matrix:
- The 87XA PIE2 enable/disable selected Bank 2; PIE2 is at 0x8D =
  Bank 1, so every PIR2-source Enable/DisableSrc RMW'd EEADR instead
  (a hunt-era regression, exposed by C11). Reverted to Bank 1.
- The dispatches' EEPROM branches dispatched EEPROM_IRQHandler on
  PIR2<EEIF> without gating on EEIE, and the handler clears EEIF: a
  polling consumer (epic-settings) can lose the completion signal to a
  live ISR. All three dispatches now gate EEIF on EEIE and leave the
  flag untouched when disabled (the poller owns it).

Simulator limits added by the combos: 16F877A data-EEPROM model inert;
TMR0IF not a reliable event source (PSA=0); the GIE-on-with-pending-
interrupt wedge (re-confirmed by C4/C9); the polled-TX wait as a wedge
landing zone; unbounded tick delays hang under a wedge (bounded probes
are the discipline); the PIC18 sim arms extra PIE bits on its own.
