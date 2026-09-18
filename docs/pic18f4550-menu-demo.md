# PIC18F4550 flagship demo: dual-toolchain comparison (epic-hal#250)

Status: 2026-09-19. `epic-menu-demo` is a realistic, product-shaped
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
| XC8 | PASS | PASS (`EPIC_HARNESS_RESULT: PASS`) | 9068/16384 (55.3%) | 639/2048 (31.2%) |
| epic-cc | **FAIL** (flash overflow) | not reached | 19497/16384 (119%, over budget) | not reached |

epic-cc now **code-generates this entire module correctly**: every
compiler panic hit along the way is fixed (see below), but the
resulting program needs 19497 flash words against the PIC18F4550's
16384-word budget, about 2.15x XC8's own 9068. This is a genuine,
newly-quantified epic-cc code-density gap on a real, full-featured
program (not a narrow combo), not a correctness bug; no epic-cc issue
is filed for it here, since "epic-cc's PIC18 backend is less code-dense
than a decades-tuned commercial compiler" is an expected, general
characteristic, not a specific defect to chase. Revisit once epic-cc's
PIC18 backend has had more general size-optimization work; this module
is one of the largest real epic-cc PIC18 targets that exists as of this
writing, so it's a good regression benchmark for that work as it lands.

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
  `eeprom_writes=4`): `EPIC_HARNESS_RESULT: PASS`, reproduced multiple
  times (epic-hal#251).
- epic-cc build: code-generates and assembles successfully through to
  the final link step, which then reports the flash overflow above
  (verified against a freshly-built epic-cc binary from current
  master, not a cached one, see epic-hal#240 on why that distinction
  matters).
