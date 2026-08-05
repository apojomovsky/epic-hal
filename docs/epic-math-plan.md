# `epic-math`: fixed-point math utility library — implementation plan

Status: **implemented**, Phases 0-5 complete; see `epic-math/docs/ARCHITECTURE.md`
for the as-built design. This document is kept as the original design record.
Ported from two 1997 Microchip application notes and modernized into a
family-agnostic C library with a hand-written inline-asm core.

## Source material

- **AN526** ([00526e.pdf](https://ww1.microchip.com/downloads/en/AppNotes/00526e.pdf)), *"PIC16C5X / PIC16CXXX Math Utility Routines"* —
  targets the baseline/mid-range PIC16 core (no hardware multiply, 12/14-bit
  instruction word, W-register-centric). Routines: 8×8 unsigned multiply,
  16×16 multiply → 32-bit (signed/unsigned), a family of fixed-point divide
  routines at several bit widths (Table 3: 32/16, 32/15, 31/15, 16/16, 16/15,
  15/15, 16/8, 16/7, 15/7, signed and unsigned), 16-bit add/subtract, BCD↔binary
  conversion (8-bit and 16-bit), BCD add/subtract, and a 16-bit Newton-Raphson
  square root built on the division routine.
- **AN544** ([00544d.pdf](https://ww1.microchip.com/downloads/en/AppNotes/00544d.pdf)), *"Math Utility Routines"* — same functional scope,
  targeting the PIC17C42. The PIC17 core is a different, more capable
  architecture than PIC16 (WREG, `MOVFP`/`MOVPF`, `ALUSTA`, more addressing
  modes) — architecturally closer to PIC18 than to PIC16, though not
  identical to either. AN544 adds three routines AN526 doesn't have: 3-point
  numerical differentiation, Simpson's-3/8-rule numerical integration, a
  16-bit LFSR pseudo-random generator, and a Gaussian generator built from it
  via the Central Limit Theorem.

**Neither note targets PIC18 directly.** This repo already has both a PIC16
family (`pic16f87xa-hal`) and a PIC18 family (`pic18fxx5x-hal`) with a proven
shared-core/per-family-backend pattern (`epic-common`, `docs/multi-family-plan.md`).
This plan reuses that pattern rather than transliterating either app note's
assembly verbatim: one neutral public API, a PIC16 backend, a PIC18 backend,
and a portable-C backend for host tests — with PIC18's backend exploiting
hardware the app notes' target chips didn't have (see "Per-family backend
strategy" below).

## Instruction-set target mismatch (read before writing any asm)

Neither app note's target chip is what this repo actually builds for, and
the gap matters differently for each family:

- **AN526 targets the *baseline* PIC16C5X core** (12-bit instruction word,
  no `ADDWFC`/`SUBWFB` — carry propagation is done via the `btfsc STATUS,C`
  + `incf` idiom AN526's own listings use, e.g. Appendix C's `D_add`). This
  repo's PIC16 family, **PIC16F87XA, is *mid-range* core** (14-bit word).
  Mid-range is a superset of baseline for arithmetic purposes and *also*
  lacks `ADDWFC`/`SUBWFB` (that instruction pair didn't arrive until
  enhanced mid-range/PIC18), so AN526's carry-propagation idiom ports to
  PIC16F87XA basically unchanged. Cross-check register/bit names against
  `pic16f87xa_sfr.h` and the datasheet
  ([DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf))
  if a routine ever touches an SFR, but the arithmetic routines in scope here
  are pure `W`/`STATUS`/GPR-file operations, so this is mostly a
  non-issue — flagging it so the implementing agent doesn't assume it needs
  to hunt for baseline-only quirks that don't actually apply.
- **AN544 targets the PIC17C42**, which is a different architecture from
  PIC18, not merely an older revision of it: PIC17 has no `MULWF` hardware
  multiply and uses `MOVFP`/`MOVPF`/`ALUSTA` instead of PIC18's
  `MOVFF`/`WREG`/`STATUS`. Treat AN544 as an *algorithm* reference only
  (the restoring-division loop shape, the negate-and-multiply-unsigned sign
  handling, the BCD digit-adjust logic) — write genuinely PIC18-native
  inline asm against the datasheet
  ([DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)), using
  `movff`, `WREG`, `STATUS,C`/`STATUS,Z`, and — unlike mid-range PIC16 —
  PIC18 *does* have `addwfc`/`subfwb`, so use them; don't carry over
  AN526/AN544's skip-and-increment carry idiom onto the PIC18 backend where
  a single-instruction carry-add/subtract is available and more idiomatic.

## Toolchain facts to establish before writing the arithmetic bodies

- **MPLAB XC8 v3.10 is installed** in this environment at
  `/opt/microchip/xc8/v3.10/bin/xc8-cc` — every inline-asm backend must be
  compiled with the real compiler as it's written, not just reasoned about
  from documentation. The only inline asm that exists anywhere in this repo
  today is zero-operand (`asm("clrwdt")`, `asm("sleep")` in the two
  `*_wdt_sleep_target.c` files) — **how to get a C parameter's value into a
  register and a computed result back out via XC8's `asm()` has not been
  established in this codebase yet.** XC8's inline asm is not GNU-extended
  asm (no `%0`-style operand substitution); the binding is almost certainly
  done by referencing C variable/parameter names directly as symbols the
  compiler has placed in known storage, but confirm this empirically:
  before writing the full multiply/divide bodies, build one minimal
  one-instruction round-trip (e.g. an 8-bit add of two parameters returning
  a result) for both the PIC16 and PIC18 backends, get it compiling and
  producing correct-looking object code with `xc8-cc`, and write up what
  you found (the exact operand-binding pattern, any constraints on which C
  storage classes/locations are addressable from inline asm) in
  `docs/ARCHITECTURE.md` before proceeding to the rest of Phase 1. If this
  turns out not to work the way the plan assumes, stop and pick the option
  most consistent with this plan's priorities (minimal asm surface,
  explicit state, thorough testing) rather than silently working around it.
- **`gpsim` is NOT installed** in this environment. Tier 2 (see Testing
  below) is correctly scoped as optional/best-effort — feature-detect it
  (`which gpsim`) and skip cleanly rather than making any build or test
  step depend on it being present.
- **`cmake`, `gcc`, and `clang` are available** for the host-sim build and
  Tier-1 tests; no toolchain gap there.

## Key architectural decision: minimize the inline-asm surface

The app notes hand-write *every* routine in assembly, including square root,
differentiation, integration, and the RNGs — but AN544 itself implements
square root by **calling its own division and addition routines**, not fresh
asm. Follow that lead and go further: only the true leaf arithmetic
primitives get per-family inline asm. Everything expressible in terms of
those primitives is written once, in portable C, against the neutral header —
identical on every family, and directly host-testable with no simulator.

- **Needs inline asm (3 backends: PIC16, PIC18, host-C reference):**
  multiply (u8×u8, u16×u16 unsigned/signed), divide/modulo (u16/u16
  unsigned/signed, one wide u32/u16 convenience form), 16- and 32-bit
  add/subtract/negate, BCD digit adjust (the `DAW`-style ±6 correction) and
  BCD↔binary digit conversion.
- **Portable C only, built on the above (1 implementation, no asm):**
  square root, 3-point differentiation, Simpson's-3/8 integration, the LFSR
  PRNG (a shift + XOR-tap is not worth hand-optimizing in asm), and the
  Gaussian generator.

This halves the amount of hand-written, execution-only-testable assembly
compared to a literal port, while keeping every performance-sensitive
primitive as real inline asm per the task's requirement.

## Per-family backend strategy

- **PIC16 (baseline/mid-range, no hardware multiply):** port AN526's
  shift-and-add algorithms directly. AN526 itself offers a speed/code-size
  trade-off for multiply (straight-line vs. looped code) — preserve that as
  a build-time knob, `PIC_MATH_OPTIMIZE_FOR_SIZE`, rather than picking one
  and dropping the other; it's a meaningful trade-off on a part with as
  little as 1.5 KB of flash.
- **PIC18: do not port AN544's shift-and-add multiply.** PIC18 has a genuine
  hardware `MULWF` instruction (single-cycle 8×8→16 unsigned multiply into
  `PRODH:PRODL`) that neither PIC16 nor the PIC17C42 AN544 targeted had. Use
  it directly for `pic_math_mul_u8`, and build `pic_math_mul_u16` from three
  partial products (`AH*BH`, `AL*BL`, cross term `AH*BL + AL*BH`) the
  standard textbook way — this is both smaller and dramatically faster than
  a 16-iteration shift-add loop. Signed multiply still uses the app notes'
  negate-operands/negate-result trick on top of the unsigned hardware path.
  This is the concrete instance of "smart enough to use a different
  implementation if needed" the task asked for — it's not just a second
  transliteration, it's a better algorithm enabled by real hardware
  differences.
- **Division (both families):** no hardware divide on either core. Both
  backends use the same restoring shift-subtract algorithm shape AN526/AN544
  use; PIC18 comes out faster in practice from having more addressing modes
  and fewer bank switches, not from a different algorithm.
- **BCD, add/sub/negate:** algorithmically identical; only the asm bodies
  differ per ISA (PIC16: `movf`/`btfsc STATUS,C`/`rrf`; PIC18: `movff`,
  `WREG`-relative ops, `ALUSTA`-equivalent `STATUS` bits).
- **Host backend:** plain C using native wider types (e.g. multiply via
  `(uint32_t)a * (uint32_t)b`, divide via `/` and `%`) — not a transliteration
  of either app note, just the obvious correct C, used for (a) the host-sim
  library build every other module in this repo already supports, and (b) as
  the independent oracle in Tier-1 tests (see Testing below).

## Public API

Follow `epic-taskmgr`'s shape, not the peripheral-driver `EPIC_ADC_*` shape:
this is a stateless computation library, not a peripheral, so plain
`pic_math_*` snake_case functions with explicit parameters/return values, no
hidden state. This is also a deliberate improvement over the source material:
AN526/AN544 hang every routine's operands off **fixed, conventionally-agreed
RAM addresses** (`ACCaLO`/`ACCaHI`/…, documented in a "Data RAM Requirements"
table the caller must respect and never reused across calls). That's a
reentrancy and testability hazard — nothing stops two call sites from
colliding on `ACCa` if one forgets the convention. The new API passes
everything by value/pointer instead; no global mutable arithmetic state
anywhere, including the RNGs (explicit `uint16_t *state` in/out, not a
hidden global "current seed").

```c
/* epic-math/include/pic_math.h — family-agnostic, no #ifdef */

/* ---- multiply --------------------------------------------------- */
uint16_t pic_math_mul_u8(uint8_t a, uint8_t b);
uint32_t pic_math_mul_u16(uint16_t a, uint16_t b);
int32_t  pic_math_mul_s16(int16_t a, int16_t b);

/* ---- divide/modulo ------------------------------------------------
 * `ok` is set false and the struct fields are zeroed on divide-by-zero,
 * instead of AN526/AN544's documented "produces incorrect results,
 * caller must ensure denominator != 0" behavior. */
typedef struct { uint16_t quotient, remainder; } pic_math_udiv16_t;
typedef struct { int16_t  quotient, remainder; } pic_math_sdiv16_t;

pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok);
pic_math_sdiv16_t pic_math_divmod_s16(int16_t  num, int16_t  den, bool *ok);
pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok);

/* ---- add/sub/negate, with explicit carry/borrow out -------------- */
uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out);
uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out);
int16_t  pic_math_negate_s16(int16_t v);
int32_t  pic_math_negate_s32(int32_t v);

/* ---- BCD ----------------------------------------------------------
 * "16"/"8" name the binary width; BCD width follows (5 digits / 2 digits).
 * bcd values are packed BCD (one nibble per digit), not ASCII. */
uint16_t pic_math_bcd16_to_bin(uint32_t bcd5);   /* 5-digit BCD -> 0..99999 */
uint32_t pic_math_bin_to_bcd16(uint16_t value);  /* 0..99999 -> 5-digit BCD */
uint8_t  pic_math_bcd8_to_bin(uint8_t bcd2);     /* 2-digit BCD -> 0..99 */
uint8_t  pic_math_bin_to_bcd8(uint8_t value);    /* 0..99 -> 2-digit BCD */
uint8_t  pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out);
uint8_t  pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out);

/* ---- built on the above, portable C, one implementation ---------- */
uint16_t pic_math_sqrt_u16(uint16_t value);   /* floor(sqrt(value)) */

int16_t pic_math_diff3(int16_t x_prev2, int16_t x_prev1, int16_t x_now,
                        int16_t inv_2h_q8 /* Q8.8 fixed-point 1/(2h) */);

int32_t pic_math_integrate_simpson38(int16_t f0, int16_t f1, int16_t f2,
                                     int16_t f3,
                                     int32_t three_h_over_8_q16 /* Q16.16 */);

/* explicit state: reentrant, testable, no hidden global RNG */
uint16_t pic_math_rand_next(uint16_t *state);
int16_t  pic_math_rand_gauss(uint16_t *state);
```

Fixed-point scale factors for `diff3`/`integrate_simpson38` are passed in
explicitly (as AN544 does — it precomputes `1/2h` and `3h/8` into a RAM
constant once, since multiply is cheaper than divide) rather than baked into
the function; document the Q-format precisely in the header so callers get
it right the first time.

## Scope decisions (flagging explicitly, not silently)

AN526 Table 3 lists nine divide bit-width variants (32/16, 32/15, 31/15,
16/16, 16/15, 15/15, 16/8, 16/7, 15/7) — these existed to squeeze cycles/ROM
on a PIC16C54 with 512 words of program memory. On parts with kilobytes of
flash, implementing all nine adds surface area (and untested-asm risk) for
marginal benefit. **Plan is to implement only `u16/u16`, `s16/s16`, and one
wide `u32/u16` convenience form** (covers "scale a 16-bit ADC reading by a
16-bit factor without overflow," the most common real use). If a future
caller needs a narrower specialized form for cycle-counting reasons, add it
then, following the same three-backend pattern — don't pre-build it
speculatively. Flag this decision to the user before Phase 1 lands in case
they had a specific narrow-division use case in mind.

## Repo layout (mirrors `epic-taskmgr/`)

```
epic-math/
  CMakeLists.txt              # family-selectable: -DEPIC_FAMILY=PIC16|PIC18
  include/
    pic_math.h                # the public header above, no #ifdef
  src/
    host/                     # portable-C bodies; linked for the host-sim build
      pic_math_mul.c  pic_math_div.c  pic_math_addsub.c
      pic_math_bcd.c  pic_math_sqrt.c pic_math_numeric.c pic_math_rand.c
    pic16/                    # PIC16 inline-asm leaf primitives (XC8 target build)
      pic_math_mul.c  pic_math_div.c  pic_math_addsub.c  pic_math_bcd.c
    pic18/                    # PIC18 inline-asm leaf primitives, incl. MULWF path
      pic_math_mul.c  pic_math_div.c  pic_math_addsub.c  pic_math_bcd.c
    common/                   # portable C shared by every backend (no asm)
      pic_math_sqrt.c  pic_math_numeric.c  pic_math_rand.c
  mcu/
    pic16f87xa-math-mplabx/Makefile
    pic18fxx5x-math-mplabx/Makefile
  tests/
    test_mul.c  test_div.c  test_addsub.c  test_bcd.c
    test_sqrt.c  test_numeric.c  test_rand.c
    golden_vectors.h           # checked-in input/output tables, shared by
                               # host tests AND the on-target self-test below
    target_selftest.c          # Tier-3: runs golden_vectors on real silicon,
                               # reports PASS/FAIL over USART (EPIC_USART_*)
  docs/
    ARCHITECTURE.md
    API.md
```

Only `sqrt`/`numeric`/`rand` live under `src/common/` (one body, no
per-family variant); `mul`/`div`/`addsub`/`bcd` get a body under each of
`host/`, `pic16/`, `pic18/`. The `CMakeLists.txt` picks `host/` for the
host-sim build and links `common/` always; the two `mcu/*/Makefile`s each
pick their own family dir plus `common/`, exactly the way `pic8-family.mk`
already picks per-family peripheral sources for the HALs.

## Testing strategy — three tiers

Inline PIC asm cannot be executed by this repo's existing host "sim" — that
simulator (`pic16f87xa_sim.c` / `pic18_sim.c`) is a peripheral/SFR model
driven by `tick()`, not an instruction-set simulator; it does not execute
arbitrary compiled asm. Say this plainly rather than pretend host tests
cover the shipped asm — they cover the *algorithm*, and a separate tier
covers the *asm*.

**Tier 1 — host unit tests (automated, every CI run, the bulk of coverage).**
Build `src/host/` + `src/common/` as a host static lib (reusing the existing
CMake pattern) and test it directly:
- `test_mul.c`: **exhaustive** for `pic_math_mul_u8` (all 256×256 pairs
  compared against `(uint16_t)a*b`); randomized (fixed seed, reproducible) +
  boundary cases (`0`, `1`, `UINT16_MAX`, `INT16_MIN`, `INT16_MAX`,
  power-of-two operands) for the 16-bit forms, cross-checked against native
  `uint32_t`/`int32_t` arithmetic.
- `test_div.c`: same shape, plus explicit divide-by-zero cases asserting
  `ok == false` and zeroed output, plus `INT16_MIN / -1` (the one signed
  divide that can overflow) — must not crash or silently wrap in a way the
  header doesn't document.
- `test_bcd.c`: exhaustive over all 100 values for the 8-bit forms; for the
  16-bit forms, exhaustive over 0..99999 (the whole valid BCD range —
  cheap on a host CPU even though it'd be unthinkable on-target) plus
  explicit invalid-nibble inputs (a digit nibble > 9) with documented
  behavior.
- `test_sqrt.c`: exhaustive over 0..65535 against `(uint16_t)floor(sqrt(v))`
  computed in double precision, since this is cheap on host and removes any
  doubt about Newton-Raphson convergence/rounding at every input.
- `test_numeric.c`: differentiation/integration checked against known
  analytic functions (e.g. linear and quadratic sequences) within a
  documented error bound, not exact equality (these are numerical
  approximations by construction).
- `test_rand.c`: not "randomness" in the cryptographic sense — assert the
  LFSR's documented properties instead: period length before repeating (for
  the taps chosen), that `rand_next` never gets stuck at the all-zero state
  it can't recover from (a classic LFSR pitfall — check this explicitly),
  and a coarse distribution/bucket-histogram sanity check for
  `rand_gauss` mirroring AN544 Figure 3.

**Tier 2 — asm-level regression, best-effort/optional.** `gpsim` (FOSS,
supports the PIC16F87x family this repo already targets) can load the real
compiled hex, poke input registers, step N instructions, and read output
registers headlessly — script it against `golden_vectors.h` so the PIC16
inline asm itself, not just the host reference, is proven against the same
vectors Tier 1 uses. Feature-detect `gpsim` and skip cleanly if absent; don't
make CI depend on installing it. **gpsim does not support PIC18** — there is
no equivalent FOSS instruction-level simulator for that family available
here, so PIC18 asm correctness relies on Tier 3 plus code review (worked
hand-traces in comments, as AN526/AN544 themselves include, showing register
state at each step of a non-trivial routine like the multiply or divide
loop).

**Tier 3 — on-target validation, manual/deferred (matches this repo's
existing stance — see `docs/multi-family-plan.md`, "real-silicon deferred").**
`tests/target_selftest.c` builds against each family's real HAL, runs every
vector in `golden_vectors.h` through the real inline-asm routines on actual
silicon, and streams a PASS/FAIL summary over USART using the already-proven
`EPIC_USART_*` driver (mirrors `tests/example_usart.c` in both HAL trees):
no new hardware dependency, and it is the strongest available proof the
shipped object code is correct on real chips of both families. Document it
as the acceptance step before any firmware ships depending on this library,
consistent with how this repo already treats real-silicon testing elsewhere.

`golden_vectors.h` is generated once (a small host-side script, checked in
alongside its generator) and is the one artifact all three tiers share —
Tier 1 replays it against the host reference, Tier 2 against PIC16 asm via
gpsim, Tier 3 against both families' asm on real silicon. Any future
regression in any backend shows up as the same vector index failing in
whichever tier can currently run.

## Milestones

- **Phase 0 — scaffolding.** Repo layout above; `CMakeLists.txt` mirroring
  `epic-taskmgr/CMakeLists.txt` (family-selectable, `add_subdirectory` on the
  chosen HAL); empty `pic_math.h`; `mcu/*/Makefile`s stubbed; a trivial link
  smoke test. No routines yet.
- **Phase 1 — core arithmetic.** `mul_u8/u16/s16`, `divmod_u16/s16/u32_16`,
  `add_u16/sub_u16/negate_s16/negate_s32`, all three backends. Tier-1 tests
  for all of it (this is the bulk of the exhaustive/randomized suite).
  PIC18's hardware-`MULWF` path lands here — call it out in review as the
  concrete "different implementation per family" deliverable.
- **Phase 2 — BCD.** `bcd16_to_bin/bin_to_bcd16/bcd8_to_bin/bin_to_bcd8/
  bcd_add8/bcd_sub8`, three backends, exhaustive Tier-1 tests.
- **Phase 3 — derived portable-C routines.** `sqrt_u16`, `diff3`,
  `integrate_simpson38` — single implementation in `src/common/`, built on
  Phase 1/2 primitives, no per-family asm. Tier-1 accuracy-bound tests.
- **Phase 4 — RNGs.** `rand_next` (LFSR), `rand_gauss` (CLT over
  `rand_next`) — portable C, explicit state. Tier-1 statistical tests
  (period length, all-zero-state handling, distribution sanity).
- **Phase 5 — validation & docs.** `golden_vectors.h` + generator script,
  `tests/target_selftest.c` (Tier 3), optional gpsim Tier-2 script for
  PIC16, `docs/ARCHITECTURE.md` + `docs/API.md` (mirroring
  `epic-taskmgr/docs/`), top-level `README.md` entry for the new component
  (mirroring the table in the root `README.md`).

Each phase should land as its own reviewable change with its tests green
before the next phase starts, the same granularity the existing "Phase 4:
port PIC18 A/D Converter driver" / "Phase 4: port PIC18 Streaming Parallel
Port driver" commits in this repo's history already use.
