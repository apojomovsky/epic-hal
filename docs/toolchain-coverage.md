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
| epic-math | 8 tests | 87XA (4) + PIC18 4455/4550 | none | PIC18 golden-vector selftest exists, never run under mdb; PIC16 smoke runs 1 call per group |
| epic-serial | yes | 87XA 876A/877A, PIC18 4455/4550 | none | Disable/Restore race class + PIC18 TX path |
| epic-taskmgr | **no** (examples only) | 87XA 876A/877A only | none | PIC18 excluded (stale rc1) |
| epic-modbus | yes | PIC18 4455/4550 only | none | PIC16 excluded for RAM (rc2/rc3) |
| epic-bus | yes | 87XA (4), PIC18 4455/4550 | none | PIC18 GPIO runtime-addr path exercised here |
| epic-console | yes | **none (supported=empty)** | none | stale rc1 exclusion, re-enableable |
| epic-settings | yes | **none (supported=empty)** | none | stale rc1 exclusion, re-enableable |
| epic-adcfilter | yes | 87XA (4), PIC18 (4) | none | |
| epic-debounce | yes | 87XA 876A/877A, PIC18 4455/4550 | none | |
| epic-pid | yes | 87XA (4), PIC18 (4) | none | |
| epic-encoder | yes | 87XA (4), PIC18 4455/4550 | none | 32-bit read under Disable/Restore |
| epic-fsm | yes | all 3 families (14) | none | |
| epic-lcd | yes | **not in manifest** | none | never real-target builds |
| epic-usb | yes | **not in manifest** | none | PIC18-only; USB not simulatable |
| epic-sdcard | yes | **not in manifest** | none | PIC18-only; needs a block device |

## Audit findings (the bug inventory)

### A. Finding-9 class, 87XA: ungated `pic_select_bank` + plain `EPIC_REG8` (5 sites)

The Finding 9 follow-up fixed 8 sites but missed the DeInit/read
paths. Same mechanism (plain-C banked write after an unrecognized bank
switch is misdirected by XC8's stale bank tracking). None of these run
in any current gate. Probe: call DeInit/Read with known values, read
the SFR back under mdb.

- `pic16f87xa_adc.c:44-47` `EPIC_ADC_DeInit` writes 0x9F (ADCON1)
- `pic16f87xa_comp.c:46-50` `EPIC_COMP_DeInit` writes CMCON
- `pic16f87xa_usart.c:103-107` `EPIC_USART_DeInit` writes SPBRG
- `pic16f87xa_timer2.c:33-36` `EPIC_TIMER2_ReadPeriod` reads PR2
- `pic16f87xa_vref.c:34-38` `EPIC_VREF_DeInit` writes CVRCON

### B. SSP_ReadByte class, 87XA: plain Bank-1 SFR access with no bank select (~17 sites)

`EPIC_SSP_ReadByte` (`pic16f87xa_ssp.c:134` SSPSTAT RMW, documented
open item) plus the same shape elsewhere. Some of these provably work
(Init-time TXSTA/TRISx writes are exercised by the passing tick/swuart
gates, which fail if TX never enables), so this class needs per-site
probe confirmation rather than blanket fixing.

- `pic16f87xa_ssp.c:134,140` SSPSTAT (documented unfixed)
- `pic16f87xa_usart.c:70,102,124,129-130,135` TXSTA writes/RMW
- `pic16f87xa_gpio.c:68,84,91,140,146` TRISx (Bank 1) runtime-addr RMW + OPTION_REG
- `pic16f87xa_timer0.c:27-29,37,56` OPTION_REG RMW
- `pic16f87xa_wdt_sleep.c:22,27,32,37` PCON reads/RMW

### C. PIC18 Finding-3 class: runtime-computed SFR addresses (gpio.c)

`pic18fxx5x_gpio.c` dereferences runtime `tris_addr`/`lat_addr`/
`port_addr` values (Init 72,84,99; DeInit 106-107; WritePin 116,119;
TogglePin 126; ReadPin 135; WritePort 141; ReadPort 146). This is the
exact shape `pic18_irq.c` and `pic18fxx5x_ccp.c` were fixed to avoid
(runtime SFR address compiles to program-memory table access). The
PIC18 tick gate never touches GPIO, so this path has never run under
mdb. HIGH suspicion.

### D. 193X Finding-2 class: runtime-address writes to Bank-1/2/3 SFRs (gpio.c)

`pic16f193x_gpio.c` routes runtime-address RMW writes to TRISx/LATx/
ANSELx (banks 1/2/3) through FSR1:INDF1, the route Finding 2 proved
silently addresses the wrong byte for PIE1/2/3. The file header claims
a branch-before-touch pattern the code does not implement. The 193X
gpio-mode gate passes, so either the harness drives RA0 through a
literal-token path or the route works for these SFRs; needs a direct
probe. HIGH suspicion.

### E. TXIF un-gated dispatch branches (Finding 10(b) class)

- `pic18fxx5x-hal/src/core/pic18_irq_dispatch.c:75` LIVE: TXIF is
  always set when TXREG is empty, so every ISR from any source fires
  `USART_TX_IRQHandler`, whose GetFlag check passes, so the callback
  (`epic_serial_on_tx` etc.) runs every ISR. This is Finding 10(b)
  unfixed on PIC18. Fix: gate on PIE1 TXIE like the 87XA reference.
- `pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c:88` LATENT: same
  branch, handler is an empty stub today, but the every-ISR call is
  already wasted and any real handler inherits the defect.

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
