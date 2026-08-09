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
| epic-math | 8 tests | 87XA (4) + PIC18 4455/4550 | 16F877A smoke-sim + 18F4550 full replay | PIC18: PASS=62 FAIL=0 golden vectors; PIC16: smoke (layout-limited, see note) |
| epic-serial | yes | 87XA 876A/877A, PIC18 4455/4550 | gate in progress | Disable/Restore race class + PIC18 TX path |
| epic-taskmgr | **no** (examples only) | 87XA 876A/877A only | gate in progress | PIC18 excluded (stale rc1) |
| epic-modbus | yes | PIC18 4455/4550 only | gate in progress | PIC16 excluded for RAM (rc2/rc3) |
| epic-bus | yes | 87XA (4), PIC18 4455/4550 | gate in progress | PIC18 GPIO runtime-addr path exercised here |
| epic-console | yes | **none (supported=empty)** | gate in progress (PIC18) | stale rc1 exclusion, re-enableable |
| epic-settings | yes | **none (supported=empty)** | gate in progress (PIC18) | stale rc1 exclusion, re-enableable |
| epic-adcfilter | yes | 87XA (4), PIC18 (4) | gate in progress | |
| epic-debounce | yes | 87XA 876A/877A, PIC18 4455/4550 | gate in progress | |
| epic-pid | yes | 87XA (4), PIC18 (4) | **16F877A PASS** (5693w) | pure logic, Q8.8 |
| epic-encoder | yes | 87XA (4), PIC18 4455/4550 | gate in progress | 32-bit read under Disable/Restore |
| epic-fsm | yes | all 3 families (14) | **16F877A PASS** (3116w) | |
| epic-lcd | yes | **not in manifest** | gate in progress (manifest added) | never real-target built before |
| epic-usb | yes | **not in manifest** | none | PIC18-only; USB not simulatable |
| epic-sdcard | yes | **not in manifest** | none | PIC18-only; needs a block device |
| pic16f87xa-hal | no | (pseudo-module) | **bank-probe PASS** (3082w) | permanent banked-SFR regression gate |
| pic18fxx5x-hal | no | (pseudo-module) | **gpio-probe PASS** (6623B) | class-C runtime-address verification |

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
struct copies (`g_t0_storage`, `g_ccp_handles[3]`, `g_handle[5]`,
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

## Probe checklist (Phase 2)

P1. DeInit/read probe for the 5 class-A sites: known value through the
     API, SFR readback under mdb.
P2. Per-site probe for the class-B sites: same shape, plus
     `EPIC_SSP_ReadByte` specifically (write known byte to SSPBUF,
     read back, verify SSPSTAT/SSPBUF).
P3. PIC18 GPIO probe: `EPIC_GPIO_WritePin`/`ReadPin` on both ports,
     register readback (the class-C route).
P4. 193X GPIO probe: same for TRISx/LATx writes (class-D route).
P5. PIC18 TX-path gate: the tick-sim PIC18 gate plus an
     epic-serial-driven example; verify the every-ISR callback firing
     (class E) and fix with the TXIE gate.
P6. ISR-load probes per family (the tick methodology): INTCON/PIR/
     PCLATH at halts under a timer-driven load, exercising the class-F
     handlers; then convert handlers to the direct-flag pattern.
P7. GIE-race probes: for each class-G site, hammer the protected read
     under a live tick ISR and check for torn values and GIE state.
P8. epic-math: full golden-vector replay under mdb on PIC18; curated
     subset on PIC16.
