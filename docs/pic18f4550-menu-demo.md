# PIC18F4550 flagship demo: dual-toolchain comparison (epic-hal#250)

Status: 2026-09-23 (close-out rerun, epic-hal#250). `epic-menu-demo` is a realistic, product-shaped
PIC18F4550 firmware (LCD menu + buttons + ADC + EEPROM + CCP/PWM +
epic-taskmgr + epic-serial together) built under both XC8 and epic-cc,
and compared for flash/RAM size and (where both sides produce a hex)
behavioral UART trace. Unlike the `epic-combo-*` firmwares in
`docs/combo-epiccc-conformance.md` (narrow, white-box compiler-
conformance probes with recorder-mocked transports), this exercises the
real, unmocked driver code paths for every peripheral it touches.

## Method

- `scripts/compare-toolchains.sh <module> <mcu> <device> [wait_ms]
  [eeprom_writes]` builds the module's sim variant under both
  toolchains, prints a flash/RAM size table (both figures normalized to
  flash *words*; `scripts/epic_build.py`'s `parse_memory_summary` does
  the XC8 byte-to-word conversion PIC18's byte-addressed program space
  needs); if both builds succeed, it also diffs the two builds' real
  MPLAB SIM (`mdb.sh`) UART traces byte-for-byte.
- e.g. `scripts/compare-toolchains.sh epic-menu-demo 18F4550
  PIC18F4550 60000 4`.

## Result

| Toolchain | Build | Real MPLAB SIM gate | Flash (words) | RAM (bytes) |
|---|---|---|---|---|
| XC8 | PASS | PASS (`EPIC_HARNESS_RESULT: PASS` at a 1500 ms wait) | 9061/16384 (55.3%) | 639/2048 (31.2%) |
| epic-cc | PASS | FAIL (no UART capture, epic-cc#632) | 11888/16384 (72.6%) | 743/2048 (36.3%) |

The flash overflow from the September baseline is resolved: epic-cc now
fits the whole module in 11888 words against XC8's 9061 (1.31x, was
2.15x at 19497 words), through the size work in apojomovsky/epic-cc#474,
#475, #483, #485 (the umbrella #469 closed with the size-ladder entry
in #490). The remaining gap is behavioral, not size: the epic-cc hex
programs and runs, but its UART capture is 0 bytes where XC8's carries
a full session. Filed as apojomovsky/epic-cc#632 (same symptom
epic-cc#484 closed via #567; the HAL inputs are provably uninvolved,
see below).

## What it took to get here

Three real bugs surfaced getting `epic-menu-demo` to code-generate
cleanly under epic-cc, in the order found:

1. **`legalize: llvm.memset.p0.i16 must carry a dst`**: a partial
   aggregate initializer (`epic_lcd_config_t cfg = { .cols = 16U, ...,
   .row_addr = {0U} }`, `CCP_HandleTypeDef ccp = { 0 }`) lowers its
   implied zero-fill to an `llvm.memset` intrinsic epic-cc's legalize
   pass doesn't yet support without a materialized destination address,
   the same gap class `docs/combo-epiccc-conformance.md` already
   documents for `combo-multitimer`/`combo-adc-uart`. Fixed in
   `menu_demo_core.c` with unrolled, field-by-field initialization
   (epic-hal#251).
2. **`isel-pic18: cannot take the value of a GEP over Global(...)`**:
   the shared `pic18fxx5x_ccp.c` driver's `EPIC_CCP_Init` stored the
   address of a runtime-indexed global array element
   (`g_ccp_handles[h->Instance] = &g_ccp_storage[h->Instance]`) into
   another array. Scalar stores through the same dynamic index are
   fine; only materializing that address as an rvalue panics. Fixed at
   the driver level (epic-hal#252/#253) by removing the redundant
   pointer-array indirection entirely (the weak ISR handlers only ever
   need a compile-time-constant index anyway). The epic-cc bug itself
   is filed as apojomovsky/epic-cc#468 and remains open; the driver fix
   sidesteps it rather than waiting on it, and is a worthwhile
   simplification on its own.
3. **`isel-pic18: cannot materialize GEP over Slot(...) in move to
   slot`**: **not an epic-cc bug.** `menu_demo_init` declared its
   `epic_lcd_ops_t ops` as an ordinary stack local, then handed its
   address to `epic_lcd_init`, which stores it into the *global*
   `g_lcd.ops`, a pointer that dangles the moment `menu_demo_init`
   returns, read by every later `epic_lcd_*` call from a completely
   different stack frame. This is undefined behavior in C, full stop;
   XC8 happened to tolerate it in practice (this repo's non-reentrant
   PIC call graphs often give automatics effectively-static storage),
   but epic-cc's isel correctly refused to materialize the escaping
   stack address as a value to store. Fixed by making `ops` `static`
   (epic-hal#251), a real correctness fix epic-cc caught that XC8
   silently masked, not a workaround.

## Verification

- `make xc8-build MODULE=epic-menu-demo MCU=18F4550`: clean.
- Real MPLAB SIM gate (`scripts/sim-mdb-run.sh`, uart mode,
  `eeprom_writes=4`, 1500 ms wait): `EPIC_HARNESS_RESULT: PASS` with no
  FAIL marker in the capture, reproduced at the close-out rerun and
  earlier under epic-hal#251. A longer wait overruns the harness bound
  and re-enters `main`, which the gate's own marker check reads as FAIL.
- Close-out rerun (`scripts/compare-toolchains.sh epic-menu-demo 18F4550
  PIC18F4550`, driver `epic-cc 0.1.0+885fe58` built inside the dev image,
  not a cached host binary, see epic-hal#240): epic-cc links clean at
  11888/16384 words and 743/2048 bytes; its mdb run produces no UART
  capture at either wait (filed as apojomovsky/epic-cc#632). The HAL tree
  is ruled out as a cause: no menu, xx5x, serial, LCD, taskmgr or EEPROM
  input differs from master, and the XC8 hex from the same tree behaves
  normally.
