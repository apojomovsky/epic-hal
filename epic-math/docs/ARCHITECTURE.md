# `epic-math` architecture

> Status: **Phases 0-5 complete.** The inline-asm operand-binding convention
> (the XC8 round-trip probe), the per-family backend write-up, and the
> testing-tier status are all filled in below. See `docs/epic-math-plan.md`
> (repo root) for the source material and the original implementation plan.

## What it is

`epic-math` is a stateless fixed-point math utility library for the 8-bit
PIC families this repo already supports (PIC16F87XA, PIC18F2455/2550/4455/
4550), ported from Microchip application notes AN526 and AN544 and
modernized into one family-agnostic C library with a hand-written
inline-asm core. See `docs/epic-math-plan.md` (repo root) for the source
material and the full implementation plan.

## Backend split (one API, three implementations)

| Layer | Source | Built by | Notes |
|---|---|---|---|
| Leaf primitives (mul/div/addsub/bcd), **host** | `src/host/` | CMake | Portable C; the Tier-1 oracle |
| Leaf primitives, **PIC16** | `src/pic16/` | XC8 Makefile | shift-add (no HW multiply) |
| Leaf primitives, **PIC18** | `src/pic18/` | XC8 Makefile | hardware `MULWF`, `addwfc`/`subfwb` |
| Derived routines (sqrt/numeric/rand), **all** | `src/common/` | both | One portable-C body, no asm |

The build selects the backend by source-list, never `#ifdef`, the same
pattern the HALs use for their host/target split.

## Why the host build links no HAL

Unlike `epic-taskmgr` (which `add_subdirectory()`s a HAL), `epic-math`'s
CMake host build links only `src/host/` + `src/common/`. The public header
needs only `<stdint.h>`/`<stdbool.h>` and the library is stateless
computation, so the Tier-1 host tests need no peripheral model. The family
dimension (and the HAL the Tier-3 on-target self-test needs for USART)
lives entirely in the XC8 Makefile targets under `mcu/`. This is the
plan-consistent reading of Tier 1 ("build `src/host/` + `src/common/` as a
host static lib and test it directly").

## Inline-asm binding, the XC8 round-trip probe

The plan required establishing, before any arithmetic bodies were written,
exactly how a C parameter reaches a register and a computed result comes
back out through XC8's `asm()`, and said to stop and pick the
plan-consistent option if the assumed binding did not work. This section
is the result of that empirical probe (XC8 v3.10, both families, `-O0`
and `-O2`, inspecting the generated `.s`).

### What does NOT work

XC8's inline assembler is **not** GNU extended asm: there is no `%0`-style
operand substitution and no input/output constraint list. More importantly,
the assembler can only reference symbols that have a file-scope storage
location. Concretely, all of these fail to link with `undefined symbol`:

```c
uint8_t add_param(uint8_t a, uint8_t b) {
    asm("movf _a,w");      /* error: undefined symbol "_a" -- params are NOT symbols */
    asm("addwf _b,w");
    ...
}
uint8_t add_static(uint8_t a, uint8_t b) {
    static volatile uint8_t sa, sb, sr;   /* function-local statics are NOT symbols either */
    sa = a; sb = b;
    asm("movf _sa,w");     /* error: undefined symbol "_sa" */
    ...
}
```

So **function parameters, autos, and even function-local `static`s are not
addressable from inline asm.** Only **file-scope** variables are.

### What does work, the established convention

Each leaf primitive is a C wrapper that takes its operands **by value**
(the public, reentrant-from-the-caller's-view API), copies them into
**file-scope `static volatile` operand scratch** (one set per function,
name-prefixed so no two routines share), runs the `asm()` body against
those named symbols, and returns the `volatile` result:

```c
/* src/pic18/pic_math_mul.c (XC8-only; #include <xc.h> for SFR symbols) */
static volatile uint8_t  m_mul_u8_a, m_mul_u8_b;
static volatile uint16_t m_mul_u8_r;

uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)
{
    m_mul_u8_a = a;             /* C store -- volatile, so not elided */
    m_mul_u8_b = b;
    asm("movf  m_mul_u8_a,w");   /* asm reads the file-scope scratch by _name */
    asm("mulwf m_mul_u8_b");     /* PIC18 hardware 8x8->16 into PRODH:PRODL  */
    asm("movf  PRODL,w");
    asm("movwf m_mul_u8_r+0");
    asm("movf  PRODH,w");
    asm("movwf m_mul_u8_r+1");
    return m_mul_u8_r;           /* C read -- volatile, so not elided */
}
```

Confirmed in the generated assembly (PIC18, `-O2`):

```
movf  m_mul_u8_a,w      ; the C "m_mul_u8_a = a" store preceded this
mulwf m_mul_u8_b
movf  PRODL,w           ; PRODL/PRODH resolve as built-in xc.h SFR symbols
movwf m_mul_u8_r
movf  PRODH,w
movwf m_mul_u8_r+1
```

The load-bearing details:

- **`volatile` is required on every scratch byte.** The asm is opaque to
  the compiler, so without `volatile` the `m_mul_u8_a = a` store (no C code
  reads it) and the `return m_mul_u8_r` read (no C code wrote it) would be
  optimized away as dead. `volatile` forces both, bracketing the asm.
- **`#include <xc.h>` in the XC8-only asm `.c` files** makes the device SFR
  symbols (`STATUS`, `WREG`, `PRODL`, `PRODH`) available to the assembler.
  The public header `pic_math.h` stays `xc.h`-free (only `<stdint.h>`/
  `<stdbool.h>`), so the host build and library consumers never pull it in.
- **STATUS bits are referenced by number, not name**: `btfsc STATUS,0` for
  carry, `STATUS,2` for zero, `STATUS,1` for digit-carry. XC8's assembler
  does **not** know the `C`/`Z`/`DC` bit aliases (`undefined symbol "C"`),
  so the numeric form is mandatory. (PIC18 adds `STATUS,3` = OV,
  `STATUS,4` = N, unused by the arithmetic here.)
- **Multi-byte scratch** is addressed with byte offsets: `m_name+0`,
  `m_name+1`, `m_name+2`, `m_name+3` (little-endian; `+0` is the low byte).
- **No `pic18fxx5x_sfr.h` edit was needed.** The plan predicted adding
  `PIC_REG_WREG`/`PIC_REG_PRODL`/`PIC_REG_PRODH` there; the probe showed
  the asm uses the `xc.h` SFR symbols directly and the C code never touches
  `PROD`, so the HAL's SFR header is left untouched. `epic-math` stays
  self-contained, it adds no symbols to any other module.

### Per-family scratch layout (banking)

The file-scratch convention has one per-family wrinkle the probe surfaced:

- **PIC18**: file-scope `static volatile` variables land in `COMRAM` (the
  access bank, 0x00-0x5F). One `banksel _m_x` per routine (emitting `movlb`)
  covers the whole routine's scratch when XC8 co-locates it. PRODL/PRODH and
  other SFRs are addressed via the access bank automatically by the assembler.
- **PIC16**: file-scope `static volatile` *separate variables*, when there
  are many, get scattered across banks 0-2 by the linker; an inline-asm
  operand that resolves to a bank-2 address (>= 0x100) does not fit the
  instruction's file-address field and causes a link-time "fixup overflow".
  The fix is to make each routine's scratch a **single struct** -- one
  object, which XC8 cannot split across banks, so it lands whole in one bank
  and one `banksel` covers every member, accessed by byte offset `(_m_x)+N`.
  The one shared object, the 16-byte `pic16_mscratch` buffer every PIC16
  asm leaf routine operates on (pic_math_scratch.h), is `__at`-pinned to
  common RAM (0x70-0x7F) instead of left to the linker: an unpinned
  placement landed at 0xA0 (bank 1) on PIC16F876A/877A and the operand
  fixups overflowed at link time. Common RAM is the same physical
  addresses in every bank, so the per-routine `banksel` is a harmless
  no-op and the whole window is reachable regardless of bank selection.
  Mid-range also lacks `setf` (carry/borrow-out is recorded with `incf`,
  read back as a bool) and lacks `addwfc`/`subwfb`/`rlcf`/`bra` (uses the
  `btfsc/btfss STATUS,0` + `incf` carry idiom, `rlf`/`rrf`, `goto`).

- **PIC16** (mid-range, no `ADDWFC`/`SUBWFB`, no hardware multiply): carry
  propagation uses AN526's own idiom, `btfsc STATUS,0` + `incf`, which
  ports to PIC16F87XA unchanged (mid-range is a superset of the baseline
  core AN526 targeted, for arithmetic). Multiply is shift-and-add.
- **PIC18** (has `MULWF`, `addwfc`, `subfwb`): use the single-instruction
  carry-add/subtract directly, do **not** carry AN526/AN544's
  skip-and-increment idiom onto PIC18 where one instruction does it.

### Where hand-asm actually adds value

The probe confirmed XC8's optimizer already emits `mulwf` for idiomatic
`return (uint16_t)a * (uint16_t)b` with `uint8_t` operands on PIC18. So
for `pic_math_mul_u8` the hand-asm and the plain-C paths produce
equivalent `mulwf` code. The hand-written asm genuinely earns its keep on
the **structured** routines, the 16×16 multiply built from three partial
products, the restoring shift-subtract divide loop, the BCD digit-adjust.
where the compiler does not auto-structure the optimal sequence. That
matches the plan's "minimal asm surface" priority: asm on the leaves where
it matters, portable C on everything derived.

### The one relaxation of "no global mutable state"

The plan's headline API improvement was "no global mutable arithmetic
state anywhere," replacing AN526/AN544's fixed shared `ACCa`/`ACCb` RAM.
XC8's operand-binding constraint forces a partial walk-back: the leaf
primitives keep **per-function** file-scope scratch. This is deliberately
narrower than the app notes' hazard, and the testability concern is fully
eliminated:

- **Per-function, name-prefixed scratch**, `pic_math_mul_u8`'s scratch
  is distinct from `pic_math_divmod_u16`'s, so two call sites of *different*
  routines can never collide (AN526's cross-routine `ACCa` collision is
  impossible; callers never see or name the scratch).
- **Written before read, every call**, no stale-state bug; the wrapper
  initializes the scratch from the value parameters on entry.
- **The public API is still by-value**, callers pass values/pointers, as
  the plan requires; the scratch is a hidden implementation detail.

The one residual hazard is **interrupt re-entrancy of the same leaf
function**: if an ISR runs the same routine while main is mid-computation
in it, the shared scratch is corrupted. This is the same limitation the
PIC16 compiled-stack already imposes on any non-`reentrant` function, and
AN526/AN544 had it too. Callers that need to call a `pic_math_*` primitive
from both main-loop and ISR context should bracket the ISR call with
`EPIC_IRQ_Disable`/`EPIC_IRQ_Restore` (the family-neutral contract the rest
of this repo already uses); a future `PIC_MATH_REENTRANT` build knob could
add that bracket inside the wrappers if a caller needs it as a default.

## Testing tiers

- **Tier 1, host unit tests** (automated, every `ctest` run, the bulk of
  coverage): build `src/host/` + `src/common/` as a host static lib and test
  it directly under `tests/`. `test_mul` is exhaustive over all 256x256 u8
  pairs; `test_sqrt` is exhaustive over 0..65535; `test_bcd` is exhaustive
  over the whole valid BCD range; the rest are randomized (fixed seed) plus
  boundaries and the documented edge contracts (divide-by-zero, `INT16_MIN /
  -1`, BCD invalid nibbles). A C reference of the restoring-division algorithm
  is machine-verified against native `/`,`%` so the asm hand-traces map to a
  tested algorithm. 8/8 tests pass.
- **Tier 2, asm regression** (optional, best-effort): `tools/gpsim_selftest.sh`
  feature-detects `gpsim` and, if present, replays `golden_vectors.h` against
  the PIC16 inline asm. It is skipped cleanly if `gpsim` is absent (it is, in
  this environment -- no passwordless sudo to install it). No PIC18
  equivalent FOSS instruction-level simulator is available; PIC18 asm
  correctness relies on Tier 3 + the hand-traces.
- **Tier 3, on-target validation** (manual/deferred): `tests/target_selftest.c`
  replays `golden_vectors.h` through the per-family inline-asm routines on
  real silicon and streams `PASS=`/`FAIL=` over `EPIC_USART_*`. It is the
  acceptance step before any firmware ships depending on this library.
  - **PIC18** (`mcu/pic18fxx5x-math-mplabx`): builds and links the full
    golden-vector self-test against the full HAL (28.9% of the 32 K-flash
    PIC18F4550). This is the canonical Tier-3 artifact.
  - **PIC16** (`mcu/pic16f87xa-math-mplabx`): the 8 K-word parts
    (16F876A/16F877A) run the FULL golden-vector replay. That became
    possible on 2026-08-11 when the asm leaves were `__at`-pinned
    below 0x800 (flash page 0, the internal-goto constraint) and two
    real asm bugs the replay exposed were fixed: the 16x16
    multiplier's `bk >>= 1` rrf chain shifted the low byte first (the
    carry must flow from the high byte down), and every carry-into-
    the-next-byte site used `incf`/`addlw`, which loses the carry when
    the byte wraps 0xFF (fixed with the `incfsz` skip idiom). The
    4 K-word parts (16F873A/16F874A) cannot hold the selftest plus the
    pins (link fragmentation, error 1347) and keep
    `tests/target_smoke16.c` via the manifest's per-MCU example
    variant. PIC16 Tier-3 correctness = the full replay on the 8 K
    parts + the exhaustive Tier-1 host tests.

`golden_vectors.h` is generated by `tools/gen_golden_vectors.c` (a host
tool built against the oracle) and is the one artifact Tiers 2 and 3 share.
A future regression in any backend shows up as the same vector index failing
in whichever tier can currently run.
