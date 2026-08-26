# HAL-3b follow-up: remove the `__EPIC_CC__` stubs (epic-hal#98)

**Tickets:** `epic-hal#98`<br>
**Depends on:** local epic-cc `0.1.0` binary (`~/.cache/epic-cc/target/release/epic-cc`, Aug 26 15:31, the binary `make epiccc-build` uses)

## Goal

Delete the `#ifdef __EPIC_CC__` stubs from `epic-fsm`, `epic-pid`, `epic-encoder`, `epic-debounce` where the compiler now supports the real bodies; keep the XC8 path byte-identical; prove module logic executes on the epic-cc path, not merely compiles.

## Probe evidence (local epic-cc binary, all probes compiled exactly as `make epiccc-build` emits)

| Module | Real body (stubs stripped) | Result |
|---|---|---|
| epic-pid | `epic_pid_update` + `epic-math/src/host/epic_math_mul.c` | **compiles** (final_pid, scratch/pid_real.c) |
| epic-encoder | full API + real `epic_tick_get`/`epic_tick_elapsed_since` calls, glitch gate, i32 stores | **compiles** (final_enc, scratch/encoder_real.c) |
| epic-fsm | `epic_fsm_dispatch` with non-NULL guard/action in a `static const` table | **SPIKE** `irparse:319 non-constant struct field value "ptr @guard"` |
| epic-debounce | real `epic_debounce_init` + `epic_debounce_poll` (indirect call `db->read(db->read_ctx)`) | **SPIKE** `isel:444 no gep for pointer %4 (epic_debounce_poll::4)` |

Boundary facts: debounce init-only and poll-only each compile; init+poll together panic (the memory round-trip of the fn ptr defeats isel's resolution). The earlier "i8/i16 stores" panic was a probe artifact (clang narrowed a `bool` global to i1), not a module blocker; encoder's i32 stores compile fine. The i1-returning indirect call is the discriminator (u8-returning variants pass). fsm's failure is distinct from epic-cc#73 (runtime-stored mutable callback works); it is const-data-with-ptr-field decode.

## Scope decision

- **Remove fully**: pid and encoder stubs (proven compilable, real drivers).
- **Keep, with issue-naming comments**: fsm dispatch and debounce init/poll stubs, both load-bearing compiler gaps, filed as new epic-cc issues. The `epic_tick_get`/`epic_tick_elapsed_since` macro redefs in debounce.c stay (#86-owned; the redefs are what keep the stub compilable). The encoder tick macro redefs are removed with the stubs; the driver provides the two tick symbols.
- Acceptance criterion "no `#ifdef __EPIC_CC__` remains" is therefore only partly met; the PR states this honestly with the filed issues.

## Steps

1. **File epic-cc issues** (repro + context in each, matching recent issue format):
   - fsm: irparse const-struct-with-fn-ptr-field SPIKE (area:frontend).
   - debounce: isel no-gep on indirect call through a stored fn ptr (area:codegen).
2. **epic-pid/src/pid.c**: delete the `#ifdef __EPIC_CC__` return-0 stub in `epic_pid_update`.
3. **epic-encoder/src/encoder.c**: delete the tick macro redefs, the six API stubs, and the three inner `#ifdef __EPIC_CC__` guards (`last_edge_tick`, glitch gate); keep `#include "epic_tick.h"` and `#include "core/hal_irq.h"`.
4. **epic-fsm/src/fsm.c**, **epic-debounce/src/debounce.c**: update stub comments to name the filed epic-cc issues.
5. **epic-pid/mcu/target_sizecheck_epiccc.c**: real driver. `init(kp=256, ki=0, kd=0, -1000, 1000)`; `update(100,0)` then `update(-200,0)`; `PORTB = (uint8_t)out` (0x38); spin. Exercises real mul, clamping, AUTO path.
6. **epic-encoder/mcu/target_sizecheck_epiccc.c**: real driver with local `g_now` timebase + `epic_tick_get`/`epic_tick_elapsed_since`. `init(4,5,5ms,0x00)`; edges 0x20 (g+10), 0x30 (g+10), then 0x10 at g+1 (glitch-rejected through the real gate); `PORTB = (uint8_t)glitch_count` (0x01); spin. Proves decode, read-twice-retry, and the tick glitch gate executing.
7. **scripts/epic_build.py**: after the epic-math source drop, append `epic-math/src/host/epic_math_mul.c` for `epic-pid`; refresh stale comments (pid/encoder are real drivers now; fsm/debounce remain pure probes).
8. **scripts/tests/test_epic_build.py**: extend the fixture with `epic-math` and `epic-pid`; assert the pid script links the host mul (not the pic16 asm mul) and the encoder script keeps the sizecheck driver and drops tick sources.

## Verification

- `make epiccc-build MODULE=epic-{fsm,pid,encoder,debounce} MCU=16F877A` and `MCU=16F887` all green.
- XC8 byte-identity: `make xc8-build MODULE=epic-pid|epic-encoder MCU=16F877A|16F887`, sha256 vs recorded baselines (877A pid `c5940bfe…`, encoder `9cd638b9…`; 887 pid `0483b7a0…`, encoder `0194611b…`).
- Execution: `make mdb-hex HEX=build/epiccc/16F877A-pid-sizecheck.hex DEVICE=PIC16F877A` prints PORTB == 0x38; same for encoder == 0x01.
- Host suite: `make test` (module ctest loop) for the touched modules.

## Blocked / not done

- fsm and debounce real bodies remain behind stubs until the two filed epic-cc issues land; the PR records the gap.
