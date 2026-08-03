# `pic18fxx5x-hal` architecture: XC8 v4.00 codegen notes

> Status: **all three bugs found and fixed; `pic8-tick`'s PIC18
> sim-target test reaches a full `PASS`.** Written up during
> `docs/ci-plan.md` Phase 4's PIC18 follow-up, after the PIC16 side of
> the same phase reached a full fix (see `pic16f87xa-hal/docs/
> ARCHITECTURE.md`). None of what's below is the same bug class as
> PIC16's (PIC18's EUSART/Timer2 registers are all in the Access Bank,
> no `pic_select_bank`-equivalent exists in this family's drivers at
> all), so this was a genuinely separate investigation, not a port of
> the PIC16 fixes.

## Why this exists

Same rationale as `pic16f87xa-hal/docs/ARCHITECTURE.md`: things specific
to XC8's PIC18 codegen, discovered because this was the first time this
family's compiled firmware actually *ran* under MPLAB SIM instead of
only being linked. Not PIC18-datasheet material; belongs here, not in
a peripheral-reference manual.

## Finding 1: the sim-target harness's baud-rate math doesn't fit in 8 bits at this file's FOSC

**Confirmed, fixed.** `pic18_harness_sim_target.c` computed `SPBRG` with
`FOSC_HZ` (48 MHz, this file's default) and `BRGH=HIGH` (divisor 16):

```
x = (48000000 / (16 * 9600)) - 1 = 311
```

The 8-bit `SPBRG` register's range is 0..255. `USART_ComputeSPBRG`
correctly detected this (`if (x > max) return 0xFFFFU;`) and returned
its documented error sentinel, which then got silently truncated to
`(uint8_t)0xFFFF = 255` by the harness's own cast, misconfiguring the
baud rate with no compile-time or runtime indication anything was
wrong. Confirmed via a real-target `mdb` probe: `SPBRG` read back `255`,
not a computed value.

**Fix**: `BRGH=LOW` (divisor 64) needs only `x=77`, comfortably in
range (actual baud ~9615, ~0.16% error, irrelevant for a marker line
whose correctness doesn't depend on precise timing). Verified via `mdb`:
`SPBRG=77` after the fix.

## Finding 2: the sim-target build had no `HARNESS=sim` → `WDT=OFF` override

**Confirmed by inspection, fixed by analogy to PIC16's own fix, not
independently proven to be the cause of any specific symptom here.**
`pic8-tick/mcu/pic18fxx5x-tick-mplabx/Makefile` hardcoded
`#pragma config WDT = ON` unconditionally, unlike
`pic16f87xa-tick-mplabx/Makefile`, which already had a `HARNESS=sim` →
`CONFIG_WDTE=OFF` override (`docs/ci-plan.md` Phase 4, item 6, in the
PIC16 investigation) for exactly this reason: a bounded diagnostic build
that terminates and reports over USART has nothing petting the
watchdog, and doesn't need it running unattended the way real-target
firmware does. Applied the same fix here (`CONFIG_WDT` variable,
`OFF` for `HARNESS=sim`, `ON` otherwise). Tested in isolation: this
alone did not resolve the sim-test hang (Finding 3 is the actual
blocker), so unlike Finding 1, this fix's necessity wasn't confirmed
against a specific observed symptom, only carried over as good practice
from PIC16's own reasoning.

## Finding 3: a runtime-computed SFR address compiles to program-memory table access, not data-memory access

**Root cause of the remaining sim-target hang, found, precisely
localized, and fixed.** With Findings 1 and 2 applied, `pic8-tick`'s
PIC18 sim-target test still produces no UART output at all. Traced via
`mdb`
instruction-stepping (`stepi`, not `run`+`wait`, for the same reason
noted in `pic16f87xa-hal/docs/ARCHITECTURE.md` Finding 9: headless
`break`-set breakpoints don't reliably work in this toolchain):

- `pic8_tick_init`'s call sequence (`compute_period` →
  `HAL_TIMER2_Init` → `HAL_TIMER2_Start` → `HAL_IRQ_Restore(1)`)
  completes: confirmed by observing `PC` land inside
  `pic8_tick_delay_ms`'s busy-wait loop at a large step count, well past
  `pic8_tick_init`'s own address range.
- Yet `INTCON` (`GIEH`/`GIEL`) and `RCON<IPEN>` never leave their POR
  values, `INTCON=0`, `RCON=0x5C` (`IPEN` clear), even deep into that
  busy-wait, meaning `HAL_IRQ_Restore(1)` never actually took effect,
  so Timer2's interrupt never fires, `g_tick_ms` never increments, and
  `pic8_tick_delay_ms` spins forever. Exactly PIC16's very first Phase 4
  bug in spirit ("nothing ever enabled GIE"), but here the code
  unambiguously calls `HAL_IRQ_Restore(1)`; the call itself is not
  taking effect.
- Isolated with a minimal throwaway probe (`HAL_IRQ_Disable()` then
  `HAL_IRQ_Restore(1)` then spin, nothing else): same result, `INTCON`
  and `RCON` unchanged. A plain, unrelated SFR write in the same probe
  (`PIC8_REG8(PIC_REG_LATB) = 0x5AU`) *did* show up correctly
  (`LATB=0x5A`), ruling out an `mdb` display/caching issue and
  confirming the problem is specific to how `HAL_IRQ_Restore` (via
  `pic18_irq.c`'s internal `sfr_set`/`sfr_clr` helpers) accesses SFRs.
- Manually inlining the exact same read-modify-write logic
  `sfr_set`/`sfr_clr` implement, but using a **compile-time-constant**
  address (`PIC_REG_INTCON` used directly, not passed through a
  function parameter) instead of `sfr_set`/`sfr_clr`'s runtime `uint16_t
  addr` parameter, *did* work (`INTCON=0x80` after setting `GIEH`
  manually).
- Checked the actual generated assembly for `sfr_set`
  (`pic18_irq.c`, called with a runtime `addr`): it uses
  `movff addr,tblptrl` / `movff addr+1,tblptrh` / `tblrd *` to read, and
  the equivalent `tblwt *` to write. `TBLPTR`/`TABLAT`/`tblrd`/`tblwt`
  are PIC18's **program-memory (flash) table read/write** mechanism,
  documented as available on "All PIC18 devices" for flash
  self-programming, not data-memory SFR access at all. A raw `tblwt *`
  with no accompanying NVMCON unlock/write-cycle sequence writes to an
  internal latch that never commits anywhere meaningful on real
  hardware, silently doing nothing observable, matching exactly what
  was seen here.

**Why**: per the XC8 v4.00 User's Guide (§5.3.6.3, "Data Pointers"),
a plain (unqualified) pointer's target memory space is determined by
*scanning what addresses get assigned to it across the whole program*
unless "local optimizations" are in effect, in which case the `__ram`/
`__rom` pointer-target qualifiers (§5.3.6.3.2) can force a specific
classification. `pic8_sfr_read8`/`pic8_sfr_write8`
(`include/target/pic18_platform.h`) are defined as a bare
`(volatile uint8_t *)(uintptr_t)(addr)` cast with no such qualifier,
and the underlying value being cast is a **runtime integer**
(`uint16_t addr`, a function parameter in `sfr_set`/`sfr_clr`), not a
traceable pointer assignment the whole-program scan can classify as
RAM-only. The compiler's default, conservative choice for an
unclassifiable pointer is apparently a mixed-target-space
representation, which for PIC18 uses the table-read/write mechanism so
the *same* generated code can transparently reach either data or
program memory depending on the runtime value.

**Tried, did not fix it**: adding the `__ram` qualifier directly to
`pic8_sfr_read8`/`write8`'s cast, and separately adding `-flocal` to
the build (the flag §5.3.6.3.2 says gates whether `__ram`/`__rom` are
even honored), together and individually. Neither changed the generated
code at all, still `tblrd`/`tblwt` in both cases (checked the `.s`
output directly after each attempt). Whether that's because `-flocal`
needs to apply repo-wide (it changes storage-duration-object allocation
scope too, §4.6.x, not tried at that scope) or the qualifier needs a
different placement/syntax than tried, not determined.

**Fix**: the mechanically-safe approach, proven to work for the
equivalent PIC16 problem (`pic16f87xa-hal/docs/ARCHITECTURE.md`
Finding 9's SSP/EEPROM follow-up). `pic18_irq.c`'s entire
`pic18_irq_desc_t` lookup table and `sfr_set`/`sfr_clr` helpers were
removed; every function (`HAL_IRQ_Disable`/`Restore`/`Enable`/
`DisableSrc`/`ClearFlag`/`GetFlag`/`SetPriority`) is now a `switch` on
`irq` with one `case` per source, each naming its register directly
(a new `SFR_SET_BIT`/`SFR_CLR_BIT` macro pair, expanding to a plain
`pic8_sfr_read8`/`write8` pair against a literal `PIC_REG_*` token, so
the address is always a compile-time constant, never a value that
crossed a function-call boundary as a `uint16_t`). `HAL_IRQ_Disable`/
`Restore` needed the same treatment even though they already passed
constant addresses at their own call sites: `sfr_set`/`sfr_clr` still
received them as a genuine runtime parameter internally, so the bug
applied there too, not just to the table-driven per-source functions.

Verified via `mdb`: `pic8-tick`'s PIC18 sim-target test reaches
`PIC8_HARNESS_RESULT: PASS` reliably (3/3 runs). Host suite and all 22
previously-passing PIC18 `(module, MCU)` real-target builds re-verified
clean, no regressions.

## Open, for whoever picks this back up

- `pic18fxx5x_ccp.c`'s `HAL_CCP_*` functions use the same
  runtime-address shape Finding 3 just fixed in `pic18_irq.c`
  (`a->cprh`/`a->cprl`/`a->con`, struct-member-derived, not compile-time
  constants). Checked every other PIC18 peripheral driver for the same
  pattern (`grep` for `pic8_sfr_read8`/`write8` call sites, not
  `mdb`-verified beyond `pic18_irq.c` itself): `pic18fxx5x_usart.c`,
  `_ssp.c`, `_adc.c`, `_eeprom.c`, `_comp.c`, and `_spp.c` all pass a
  compile-time-constant `PIC_REG_*` macro directly, the pattern already
  confirmed safe; `_ccp.c` is the only other one that doesn't. Very
  likely has the same bug; not yet probed under `mdb` to confirm, nor
  fixed (`pic8-tick` doesn't exercise CCP, so it didn't block the
  sim-target pilot). Same fix shape would apply: dispatch on which CCP
  instance before touching any SFR, so each access site ends up with a
  literal register name.
- This document itself should be checked for staleness against whatever
  XC8 version `docker/ci-toolchain/Dockerfile` pins if that version is
  ever bumped; these findings are cited against v4.00 specifically.
