# Combo firmware headroom under epic-cc (HAL-3g, epic-hal#92)

Status: 2026-08-27. The 13 `epic-combo-*` integration firmwares link
several modules into one image, the whole-program overlay's real test.
On the 877A the overlay must fit 368 bytes GPR; on the 4550 it has
2048 bytes. This doc records which combos build and gate under epic-cc,
and the XC8 vs epic-cc RAM/flash headroom per combo.

## Method

- Build: `python3 scripts/epic_build.py build --module <combo> --mcu <part>
  --variant sim --toolchain epic-cc` (emits `build-sim/<m>/<mcu>/build.sh`)
  then `docker run ... epic-cc-dev:local sh <script>` with
  `PIC8_CLANG_UNWRAPPED/PIC8_CLANG_RESOURCE_DIR` (Makefile `epiccc-build`).
- Gate: `env SIM_MDB_SKIP_BUILD=1 scripts/sim-mdb-run.sh local <mcu>
  <device> <combo> 8000 uart` in the hal toolchain image (captures
  `EPIC_HARNESS_RESULT: PASS` over UART). The sim firmware is the same
  `tests/combo_*.c` that gates under XC8; the mdb harness
  (`pic16_harness_mdb.c` / `pic18_harness_mdb.c`) prints the marker.
- XC8 numbers: `make xc8-build MODULE=<combo> MCU=<part>` (hal image,
  XC8 frontend, DFP packs). RAM = GPR bytes from the map; flash = program
  words from the hex.

| Combo | XC8 build | XC8 gate | epic-cc build | epic-cc gate | XC8 RAM | XC8 flash | epic-cc RAM | Notes |
|---|---|---|---|---|---|---|---|---|
| combo-uart-ssp | PASS | PASS | FAIL | - | 193 | 2374 | - | `iselcore: no gep for pointer %7` in `pic16_harness_mdb.c:epic_harness_log` (harness pointer chain base missing), epic-cc#138-adjacent, filed |
| combo-multitimer | PASS | PASS | FAIL | - | 296 | 3746 | - | `legalize: llvm.memset.p0.i16 must carry a dst`, struct `{i16,..}` zero-init for `TIMER1_HandleTypeDef` / `TIMER0/TIMER2` handles |
| combo-adc-uart | PASS | PASS | FAIL | - | 228 | 2956 | - | `legalize: llvm.memset.p0.i64 must carry a dst`, `TIMER1_HandleTypeDef` zero-init (12 bytes, i64 dst form) |
| combo-rb-uart | PASS | PASS | FAIL | - | 208 | 2981 | - | `alloc: GPR demand exceeds 0x1EF (0x01f0)`, overlay needs 1 byte past the 877A window on this branch (tick-serial repros identically) |
| combo-encoder-tick | PASS | PASS | FAIL | - | 201 | 3886 | - | `alloc: no arrangement of 16 globals fits p16f877a's 4 GPR bank window(s) (total demand 377, capacity 352)`, five `EPIC_HARNESS_LOG_STATIC` sites own 291 bytes |
| combo-swuart-tick | PASS | PASS | FAIL | - | 317 | 6100 | - | `legalize: unknown intrinsic "llvm.umin.i32"`, `EPIC_SWUART_Init` clamp `if (cycles > 65535u)` lowers to `llvm.umin/umax` |
| combo-tick-serial | PASS | PASS | FAIL | - | 318 | 4509 | - | `alloc: GPR demand exceeds 0x1EF (0x01f0)`, serial+tick stack (USART+TIMER2+harness) at the 877A ceiling |
| combo-rx-loopback | PASS | PASS | FAIL | - | 236 | 2562 | - | `alloc: GPR demand exceeds 0x1EF (0x01f0)`, 1 byte past the window, same as tick-serial |

XC8 gates are green on `origin/master` CI (`family-check` PIC16F87XA 16F877A).
All eight manifest `epiccc_hal_sources_by_family` slices are declared; the
sources are epiccc-clean (no array allocas, no const-address materialization,
see `epic_harness.h:EPIC_HARNESS_LOG_STATIC` and the per-combo `static`
buffers). The remaining epic-cc failures are in shared HAL/driver or
compiler lowering, not in the combo sources themselves.

| Combo | XC8 build | XC8 gate | epic-cc build | epic-cc gate | XC8 RAM | XC8 flash | epic-cc RAM | Notes |
|---|---|---|---|---|---|---|---|---|
| combo-eeprom-isr | PASS | PASS | FAIL | - | 214 | 5649 | - | `irparse: unsupported type "i64"`, PIC18 backend has no 64-bit lowering yet (first PIC18 combo probed) |
| combo-lcd-tick | PASS | PASS | FAIL | - | 436 | 10415 | - | same reshapes applied (fail() static, literals via `EPIC_HARNESS_LOG_STATIC`); build not yet probed past the i64 gate |
| combo-modbus-full | PASS | PASS | FAIL | - | 618 | 16370 | - | same |
| combo-taskmgr-serial | PASS | PASS | FAIL | - | 478 | 10919 | - | same |
| combo-tick-settings | PASS | PASS | FAIL | - | 282 | 7927 | - | same |

Family `epiccc_sources` for PIC18Fxx5x is declared
(`gpio+timer2+usart+eeprom+irq+wdt/vector/dispatch+harness`) and the
`pic18_irq_dispatch_epiccc.c` tier (USART+TMR2+EE, same gating as the
full fan-out) is added. PIC18 has 2048 bytes GPR, so fit is not the
wall; the wall is codegen for `i64` and the still-unproven PIC18 isel
paths for `eeprom`, `usart`, and `tick` under epic-cc.

## What landed in this PR

- `pic16f87xa_sfr.h` / `pic16f88x_sfr.h`: `PIC_REG_PIE1/PIE2` (0x8C/0x8D).
- `pic16f87xa_platform.h` / `pic16f88x_platform.h` (epiccc): `EPIC_PIE1_READ_*`
  helpers for `TMR2IE/SSPIE/ADIE/CCP1IE` + `PIE2:C CP2IE`, so dispatch tiers
  read PIE through the same 0x8C/0x8D derefs as the helper family.
- `pic16*_hal/src/epiccc/pic16_irq_dispatch_tiers_inc.h`: gates for
  `TMR0/RB/SSP/ADC/EE` (each gated on its enable bit, else clear) so combo
  tiers need not hand-roll RMW clearing.
- Five new PIC16 tier files (both families, 10 files total):
  `serial_timers`, `serial_rb_tick`, `serial_timer1_adc`,
  `serial_tick_ssp_ee`, `swuart_tick` (each a 5-line define list over the
  shared body).
- `pic18fxx5x-hal/src/epiccc/pic18_irq_dispatch_epiccc.c` (USART+TMR2+EE).
- `epic-common/manifest/modules.toml`: family `epiccc_sources` for
  PIC18Fxx5x plus 13 per-combo `epiccc_hal_sources_by_family` overrides
  (each verbatim, mdb harness in place of the target no-op harness).
- `epic-common/include/core/epic_harness.h`: `epic_harness_report` now
  copies the marker into a static RAM buffer before logging (so no const
  address is materialized; epic-cc#138), plus `EPIC_HARNESS_LOG_STATIC`
  for gate firmware literals.
- 13 combo sources: `char c[N]` locals become `static` buffers, string
  literals go through `EPIC_HARNESS_LOG_STATIC` or merged static buffers,
  preserving the captured UART byte stream exactly (verified: the mdb
  harness's `while(*fmt) putc` never transmits the NUL terminator, so a
  merged `F+hi+lo+'.'` buffer emits the same 4 bytes as four 1-char logs).

## Filing

- `apojomovsky/epic-cc#138` (const-table window) remains the umbrella for
  const-address materialization. New gaps surfaced here and filed:
  `irparse` array `alloca` / `i6` / `i64` type gaps, `legalize` memset
  dst / `llvm.umin/umax` gaps, `iselcore` harness pointer chain gap, and
  the 877A GPR bin-packing regression (tick-serial repros on current
  `epic-cc:master` even before this PR's tiers). Each combo's Notes row
  above names the first panic that blocks it.

## Next steps (not in this PR)

- epic-cc: `alloca [N x i8]`, struct zero-init `llvm.memset` with typed
  dst, `llvm.umin/umax`, `i64` on PIC18, and the harness `epic_harness_log`
  pointer chain (or a `epic_harness_report` that never calls through a
  const pointer at all).
- Once a fix lands and the 13 `epiccc-build` invocations advance past the
  first panic, re-run the mdb gates and fill the `epic-cc RAM` / `gate`
  columns above; the XC8 `RAM` / `flash` columns are the headroom
  baseline until then.
