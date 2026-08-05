# `epic-encoder`: interrupt-driven incremental quadrature decoder, implementation plan

Status: **implemented**, all six phases landed (0a HAL hook, 0b
scaffold, 1 core+tests, 2 examples, 3 cross-compile+footprint, 4 docs).
This plan was written to stand alone with no other conversation context.
Every design decision below was made in the design discussion this plan
closes out; only the items explicitly marked **pending**/**left to
implementation** were still open at planning time. The two resolution
notes worth flagging post-implementation: the plan's verbatim QUAD_TABLE
makes the physical direction it labeled "forward, +8" actually count -8
(the table is the non-redesignable shipped artifact, so the test asserts
the table's actual output and documents the sign as a wiring convention,
see `epic-encoder/docs/ARCHITECTURE.md`), and the host-sim RBIF modeling
landed on the documented test-only fallback (asserting RBIF directly),
not the faithful "snapshot on every PORTB read" model, for the
proportionality reasons in the same doc.

## What this is, and the HAL gap it depends on

A vendor-agnostic decoder for a standard 2-channel (A/B) incremental
quadrature encoder, x4 resolution (counts every edge of both channels),
composable with `epic-pid` (`docs/epic-pid-plan.md`) the same way this
whole plan was motivated: feed `encoder_get_position()` into
`pid_update()`'s `measurement` argument every control cycle.

Neither PIC16F87XA nor PIC18F2455/2550/4455/4550 has any encoder-aware
peripheral (no QEI mode, confirmed by grepping both families'
`peripherals/*_ccp.h`: capture-mode options are edge/4th/16th capture into
a timer, nothing quadrature-aware). This has to be done in software,
interrupt-driven, using the one thing both families do have: the RB<7:4>
"interrupt-on-change" (IOC) source (`PIC16_IRQ_RB` / `PIC18_IRQ_RB` in the
existing `core/pic16_irq.h` / `core/pic18_irq.h`, "RB<7:4> change" on
both). Wire encoder channel A and B to two of RB4-RB7; any edge on either
fires one shared interrupt; decode with a software Gray-code
state-transition table.

**This surfaced a real gap while planning**: unlike Timer0/Timer1/Timer2
(which every peripheral header already exposes a weak `*_IRQHandler` +
callback for, e.g. `TIMER2_IRQHandler`/`OverflowCallback`,
`pic16f87xa-hal/include/peripherals/pic16f87xa_timer2.h`), **neither HAL
currently has any hook for the RB-change interrupt at all**: no weak
handler, no callback registration, and it is not in either family's
`epic_dispatch_all_irqs` fan-out (`src/core/pic16_irq_dispatch.c`,
confirmed by reading it: it calls `TIMER0_IRQHandler` through
`COMP_IRQHandler`/`PSP_IRQHandler`, nothing GPIO-related). So this plan
has a **Phase 0a that adds a small, genuinely-missing piece of HAL
surface to both `pic16f87xa-hal` and `pic18fxx5x-hal`** before the
`epic-encoder` module itself (Phase 0b onward) can be built on top of it.
This mirrors the existing Timer2 pattern exactly, it is not a new
convention, just applying the existing one to a peripheral nobody needed
from application code yet.

## Phase 0a design: the missing GPIO change-interrupt hook

Add to **both** `pic16f87xa_gpio.h`/`.c` and `pic18fxx5x_gpio.h`/`.c`
(same names on both, family-specific bodies, the repo's standard
fixed-contract split):

```c
/** Fires once per RB-change interrupt, given the freshly-read PORTB byte.
 *  NULL is safe (no-op). Whole-port, not per-pin: RB<7:4> change is a
 *  single shared source on this hardware generation, mirroring Timer2's
 *  one-callback-per-handle shape but simpler (no handle struct needed,
 *  there is only ever one PORTB). */
void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/** Weak RB-change ISR, override in user code to add application logic
 *  (mirrors every other *_IRQHandler in this HAL). */
void RB_IRQHandler(void) EPIC_WEAK;
```

### Why the callback takes the already-read PORTB byte (read-before-clear is mandatory, not stylistic)

DS39582B §14.11.3 / DS39632E's equivalent RB-change section both document
that clearing RBIF **before** reading PORTB is wrong: the mismatch
comparator latches "the value of PORTB at the last read", so reading
PORTB is what ends the current mismatch condition and re-arms detection
of the next one; clearing the flag first (or not reading at all) risks an
immediate spurious re-interrupt or, worse, silently missing a change that
lands in the gap. So the body of `RB_IRQHandler` **must** do, in this
exact order: read `PORTB` into a local, clear RBIF, then call the
callback with that already-read value, never let the callback (or
anything else) do a *second*, later read of PORTB, since by then it may
no longer reflect the byte the mismatch logic actually cleared against.
This is exactly the class of "don't trust datasheet prose alone" behavior
`AGENTS.md` asks to be probed, not assumed. It can't be fully probed on
the host sim as-is (see the sim note below), so get the C source's
operation order right by construction and say so plainly in a comment at
the read/clear site, since that ordering is what a future reader could
otherwise "simplify" into a bug.

```c
/* pic16f87xa_gpio.c (pic18fxx5x_gpio.c is the same shape) */
static void (*s_rb_change_callback)(uint8_t) = NULL;

void EPIC_GPIO_RegisterChangeCallback(void (*callback)(uint8_t))
{
    s_rb_change_callback = callback;
}

void RB_IRQHandler(void)
{
    if (!EPIC_IRQ_GetFlag(PIC16_IRQ_RB)) return;
    uint8_t portb = EPIC_REG8(PIC_REG_PORTB);  /* MUST read before ClearFlag, DS39582B §14.11.3 */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_RB);
    if (s_rb_change_callback) s_rb_change_callback(portb);
}
```

(`PIC_REG_PORTB` / the equivalent PIC18 macro: use whatever this file's
existing read helpers already use, mirror the surrounding driver code,
don't invent a new access idiom.)

Also add `RB_IRQHandler();` to the fan-out in both
`pic16_irq_dispatch.c` and the PIC18 equivalent
(`pic18_irq_dispatch.c`), following the exact same "declare a strong
extern prototype here, don't include the peripheral header" pattern the
file's own doc comment explains (the weak-symbol/host-link reason is
spelled out there already).

Add a family-neutral shim, mirroring `peripherals/hal_timer2.h` exactly
(a two-line file that just `#include`s the family-specific `*_gpio.h`),
at `peripherals/hal_gpio.h` in **both** families, this doesn't exist yet
either (only `hal_timer0.h`/`hal_timer2.h` do, because no family-agnostic
module has needed portable GPIO access before now). `epic-encoder` will
include this neutral header, never a family-specific one directly.

### Why one whole-port callback, not per-instance registration inside the HAL

`epic-encoder` will want to run more than one encoder off the same
RB<7:4> interrupt (see "at most two per port" below). The HAL layer does
**not** grow a multi-subscriber registry for this, one callback slot is
enough, exactly matching Timer2's one-`OverflowCallback`-per-handle
shape. Fanning one received byte out to N `encoder_t` instances is
application-level composition (demonstrated in `example_encoder_hal.c`,
Phase 2), the same "compose by staying decoupled" reasoning `epic-fsm`
and `epic-taskmgr` already use elsewhere in this repo, don't build a
registry into the HAL or into `epic-encoder` itself to save the
application five lines of a static array and a `for` loop.

### The host sim does not model RBIF yet: close that gap too, with a named fallback

Checked `pic16f87xa-hal/src/sim/pic16f87xa_sim.c`:
`pic16f87xa_sim_drive_input()` only overrides what a subsequent pin
*read* returns, it never asserts RBIF on a change. Faithfully testing
`RB_IRQHandler` on host therefore needs the sim to model the real
mismatch behavior: RBIF should assert when a pin within RB<7:4>
configured as input changes value relative to the PORTB value at the
*last CPU read of PORTB*, exactly the datasheet semantics above. Locate
wherever this sim's core intercepts SFR reads (search the file and its
neighbors for how other auto-asserting flags, e.g. `TXIF`'s
re-assert-every-step logic near the bottom of `pic16f87xa_sim.c`, hook
into the step/read path) and add the RB<7:4>-scoped mismatch tracking
there; do the equivalent in `pic18fxx5x-hal`'s sim. This is
**left to implementation** to locate the exact hook point precisely
(not fully mapped during planning): if a faithful "snapshot on every
PORTB read" model turns out to be more invasive than this one feature
warrants, the documented fallback is a **test-only** direct assertion of
the RBIF bit (poke the flag register directly in the test, then call
`RB_IRQHandler()` and assert on the callback's observed argument and the
post-call flag state), less faithful, still proves the handler's own
read/clear/callback logic is correct, which is the part that actually
matters here. Prefer the faithful sim model; fall back only if it proves
genuinely disproportionate.

## `epic-encoder` module design

### Why push (caller calls `encoder_update()` from its own ISR callback), not poll

Unlike `epic-debounce`'s `debounce_poll()` (called once per scheduler
tick, `docs/epic-debounce-plan.md`), a quadrature signal can transition
between poll calls faster than any reasonable poll period, a poll-driven
design would just miss counts. This has to be edge-triggered: the
application registers one function as the HAL's RB-change callback
(Phase 0a above), that function forwards the received PORTB byte to
`encoder_update()` for each `encoder_t` instance it owns.

### Why a Gray-code transition table, and why it needs no `epic-math`

The standard, widely-used technique for quadrature decode with no
dedicated hardware: track a 2-bit state `(A<<1)|B`, look up
`table[(prev_state<<2)|new_state]` for a `{-1, 0, +1}` step. With state
ordering `00,01,11,10` (true Gray code, adjacent states differ by one
bit) the table is:

```c
static const int8_t QUAD_TABLE[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0,
};
```

This is a well-established reference table (used in numerous open-source
rotary-encoder libraries), not novel work, but per this repo's own
convention of not trusting an unverified table blindly, Phase 1's test
suite includes an explicit "known-good full rotation" case that walks
the whole `00→01→11→10→00` sequence forward and backward and asserts the
expected total (see Testing strategy). If your physical wiring produces
the opposite sign (turns clockwise but the count goes down), that's a
wiring convention issue, document "swap `pin_a`/`pin_b` at `encoder_init`
to invert direction" in `docs/API.md`, don't add a `direction_invert`
flag to the struct for it, swapping two constructor arguments already
solves it.

Only integer add/compare/shift are needed, no multiply, so unlike
`epic-pid` this module does **not** link `epic-math`.

### Why it still needs a HAL family link (unlike `epic-fsm`/`epic-debounce`'s pure core)

`encoder_update()` itself is pure C, no HAL dependency, same as
`fsm_dispatch()`. But `encoder_get_position()` reads a `volatile int32_t`
that the ISR writes asynchronously to the caller's mainline code, exactly
`epic_tick_get()`'s situation (a 4-byte read on an 8-bit core than an ISR
can tear mid-read), and the fix is the same: wrap the read in
`EPIC_IRQ_Disable()`/`EPIC_IRQ_Restore()` (`core/hal_irq.h`, family-neutral
signature, `uint8_t EPIC_IRQ_Disable(void)` / `void
EPIC_IRQ_Restore(uint8_t)`). That one call is the entire HAL surface this
module touches. `epic-encoder/CMakeLists.txt` therefore needs a
`EPIC_FAMILY` selection and an `add_subdirectory` exactly like
`epic-tick/CMakeLists.txt` already does, mirror that file, don't
reinvent the selection mechanism.

### Why a per-instance minimum-edge-interval gate, off by default, separate from the transition-table's own error counting

From the design discussion: real mechanical/contact rotary encoders
bounce, Hall-effect encoders generally don't (no physical contact) but
can suffer electrical noise from nearby motor windings/PWM, and unlike
STM32's hardware QEI (counts in silicon, immune to firmware timing), this
software approach can also simply **miss edges** if they arrive faster
than the ISR can service them. Three distinct failure modes, two distinct
counters:

- **Impossible transitions** (`QUAD_TABLE` returns 0 for a genuine state
  change, i.e. both bits appeared to flip between samples) usually mean a
  missed edge or bit-level noise corrupted a sample. Tracked in
  `error_count`, always on, free (the table lookup already happens).
- **Bounce/glitch** (a real, valid-looking transition arriving suspiciously
  soon after the last accepted one) is a mechanical-contact problem, not
  a decode-correctness problem. Tracked separately in `glitch_count`, only
  active when `min_edge_interval_ms != 0` for that instance. Kept optional
  and per-instance (not a global config) so a clean Hall-effect channel on
  the same firmware isn't held to a timing gate a bouncy mechanical one
  next to it needs; `epic-tick`'s 1 ms resolution is coarse relative to
  true electrical bounce timescales but adequate for mechanical detent
  bounce (typically resolves within a few ms): document this resolution
  limit plainly, don't silently oversell precision the module doesn't have.

Keeping these as two counters (not one) is deliberate: a climbing
`error_count` means "the software isn't keeping up or something's
electrically corrupting samples", a climbing `glitch_count` means "the
gate is doing its job filtering real mechanical bounce". Conflating them
would destroy the diagnostic value that was the actual point of adding
either counter.

### Why `encoder_init`/`encoder_reset` take the current port byte, not a hardcoded initial state

Same reasoning `epic-debounce`'s `debounce_init` uses for reading the pin
once at init (`docs/epic-debounce-plan.md`): if `last_state` defaulted to
`0` regardless of the encoder's actual physical position at start-up, the
very first real edge could be misjudged (compared against a state the
hardware was never actually in), or worse counted as the "impossible
transition" case spuriously. The caller reads the port once (the same
byte its `EPIC_GPIO_RegisterChangeCallback` handler would receive, or a
one-off manual read at boot before interrupts are enabled) and passes it
in.

### At most two full encoders per port, and no third input source on these parts for a Z/index channel

RB<7:4> is 4 pins; one A/B pair uses 2, so at most two independent
`encoder_t` instances can share one port's IOC. This is a real hardware
ceiling on these two families (mismatch-style IOC exists **only** on
PORTB on both, confirmed via the IRQ enum: `PIC16_IRQ_RB`/`PIC18_IRQ_RB`
are the only IOC sources listed, nothing analogous for PORTA/C/D).
State this plainly in `docs/API.md` as a hard constraint, not a tunable.

## Public API (finalized in design discussion, do not redesign)

```c
/* epic-encoder/include/encoder.h, needs <stdint.h> and epic_tick.h
 * (for epic_tick_get/elapsed_since, used only when min_edge_interval_ms != 0) */

typedef struct {
    uint8_t           pin_a, pin_b;          /* bit positions 0-7 within the port byte
                                               * the change-interrupt callback receives */
    volatile int32_t  position;              /* x4 quadrature count; written only from
                                               * ISR context (encoder_update) */
    uint8_t           last_state;            /* 2-bit (a<<1|b), previous sample */
    uint16_t          min_edge_interval_ms;  /* 0 = glitch gate disabled */
    uint32_t          last_edge_tick;        /* epic_tick_get() at the last ACCEPTED edge;
                                               * meaningless when the gate is disabled */
    volatile uint16_t error_count;           /* impossible Gray transitions seen */
    volatile uint16_t glitch_count;          /* edges rejected by the interval gate */
} encoder_t;

void encoder_init(encoder_t *enc, uint8_t pin_a, uint8_t pin_b,
                   uint16_t min_edge_interval_ms, uint8_t port_value);

/* Re-syncs last_state from port_value and zeroes position/error_count/
 * glitch_count/last_edge_tick, gains (pin_a/pin_b/min_edge_interval_ms)
 * unchanged. For recovering after a fault without re-wiring the instance. */
void encoder_reset(encoder_t *enc, uint8_t port_value);

/* Call from the application's EPIC_GPIO_RegisterChangeCallback handler,
 * once per registered instance, passing the same received port byte. */
void encoder_update(encoder_t *enc, uint8_t port_value);

int32_t  encoder_get_position(const encoder_t *enc);    /* atomic (IRQ-disabled) read */
uint16_t encoder_get_error_count(const encoder_t *enc);
uint16_t encoder_get_glitch_count(const encoder_t *enc);
```

### `encoder_update` algorithm (implement exactly this)

```
encoder_update(enc, port_value):
    a = (port_value >> enc->pin_a) & 1
    b = (port_value >> enc->pin_b) & 1
    new_state = (a << 1) | b

    if new_state == enc->last_state:
        return                                   # nothing changed for THIS instance's pins

    if enc->min_edge_interval_ms != 0:
        now = epic_tick_get()
        if epic_tick_elapsed_since(enc->last_edge_tick) < enc->min_edge_interval_ms:
            enc->glitch_count++
            return                               # drop it; last_state intentionally NOT
                                                   # updated, next sample compares against
                                                   # the last ACCEPTED state, not this one
        enc->last_edge_tick = now

    delta = QUAD_TABLE[(enc->last_state << 2) | new_state]
    if delta == 0:
        enc->error_count++                        # new_state != last_state (checked above)
                                                    # yet not a valid single quadrature step
    enc->position += delta
    enc->last_state = new_state
```

`encoder_init` sets `position = 0`, `error_count = 0`, `glitch_count = 0`,
`last_edge_tick = epic_tick_get()`, and `last_state` from the given
`port_value` using the same `(a<<1)|b` extraction as above. `encoder_reset`
does the same minus re-storing `pin_a`/`pin_b`/`min_edge_interval_ms`.

`encoder_get_position`:
```c
int32_t encoder_get_position(const encoder_t *enc)
{
    uint8_t s = EPIC_IRQ_Disable();
    int32_t p = enc->position;
    EPIC_IRQ_Restore(s);
    return p;
}
```
(mirrors `epic_tick_get()`'s exact pattern; `encoder_get_error_count`/
`_get_glitch_count` are plain `uint16_t` reads, an 8-bit core reading a
16-bit value has the same tear risk in principle, wrap those the same
way for consistency even though a torn diagnostic counter is low-stakes.)

## Repo layout

```
epic-encoder/
  CMakeLists.txt                    # EPIC_FAMILY selection + add_subdirectory,
                                     # mirrors epic-tick/CMakeLists.txt exactly
  include/encoder.h
  src/encoder.c                     # single implementation, no per-family variant
  examples/
    example_encoder_hal.c           # two encoder_t sharing one port's RB<7:4> IOC,
                                     # host-sim driven via pic16f87xa_sim_drive_input
                                     # sequences, EPIC_GPIO_RegisterChangeCallback wiring
                                     # demonstrated end to end, logged via epic_harness_log
    example_encoder_pid_loop.c      # one encoder_t feeding encoder_get_position() into
                                     # pid_update() as `measurement` each simulated cycle
                                     # (links epic-pid too), the direct proof of the
                                     # "transparent alongside PID" goal this was built for
  tests/
    test_encoder.c                  # host tests, see below
  mcu/
    pic16f87xa-encoder-mplabx/Makefile   # cross-compile sanity check + footprint,
    pic18fxx5x-encoder-mplabx/Makefile   # linking the now-extended HAL + epic-tick
  docs/
    ARCHITECTURE.md
    API.md
  README.md
```

Plus the Phase 0a changes living **inside the existing HAL trees**, not
under `epic-encoder/`:

```
pic16f87xa-hal/
  include/peripherals/pic16f87xa_gpio.h   # + EPIC_GPIO_RegisterChangeCallback, RB_IRQHandler
  include/peripherals/hal_gpio.h          # new neutral shim (mirrors hal_timer2.h)
  src/peripherals/pic16f87xa_gpio.c       # + the callback slot + RB_IRQHandler body
  src/core/pic16_irq_dispatch.c           # + RB_IRQHandler() in the fan-out
  src/sim/pic16f87xa_sim.c                # + RBIF mismatch modeling (or the documented
                                           #   test-only fallback if that proves disproportionate)
  tests/ (or wherever this HAL's own peripheral tests live)  # + RB-change handler tests

pic18fxx5x-hal/  # same five changes, PIC18-shaped bodies
```

## Testing strategy

### Phase 0a (in each HAL's own test suite, not `epic-encoder`'s)

- `RB_IRQHandler` is a no-op (no callback invocation, flag untouched)
  when RBIF is not pending.
- When RBIF is pending (via the sim's RBIF modeling, or the documented
  test-only fallback if that's what got built): the registered callback
  fires exactly once, receives the correct PORTB byte, and RBIF is clear
  after the handler returns.
- A `NULL` registered callback (or none registered) doesn't crash.
- `epic_dispatch_all_irqs` reaches `RB_IRQHandler` (a smoke test that a
  full dispatch pass with RBIF pending still produces the callback
  invocation, proving the fan-out wiring, not just the handler in
  isolation).

### Phase 1 (`epic-encoder/tests/test_encoder.c`)

One implementation, so host tests prove the shipped code directly, same
reasoning as every other pure-logic module in this repo. Cases:

- **Full clean forward rotation**: drive the sequence
  `00→01→11→10→00→01→11→10→00` (two full electrical cycles) through
  `encoder_update`, assert `position` ends at exactly `+8` (4 steps per
  cycle x 2 cycles) and `error_count == 0`.
- **Full clean reverse rotation**: the reverse sequence, assert `-8` and
  `error_count == 0`. Together with the forward case, this is the
  "known-good against `QUAD_TABLE`" verification called for above.
- **Impossible transition**: from a known `last_state`, call
  `encoder_update` with a `port_value` implying the diagonally-opposite
  2-bit state (both bits flipped at once). Assert `position` is
  unchanged and `error_count` incremented by exactly 1.
- **No-op on unchanged pins**: call `encoder_update` twice with the same
  `port_value`. Assert the second call changes nothing (`position`,
  `error_count`, `glitch_count` all unchanged), this is the ordinary
  case of another instance's pins changing on the same port byte, not an
  error.
- **Glitch gate rejects a too-soon edge**: `min_edge_interval_ms > 0`,
  script `epic_tick`'s simulated time (via `epic_harness_tick()`
  advancement, same technique `epic-debounce`'s test suite already uses
  for real timing) so a second valid-looking transition arrives before
  the interval elapses. Assert it's rejected (`position` unchanged,
  `glitch_count` incremented, `last_state` NOT advanced to the rejected
  sample), then advance past the interval and repeat the same edge,
  assert it's now accepted.
- **Glitch gate disabled by default**: `min_edge_interval_ms == 0`, two
  transitions arriving on consecutive calls with no simulated time
  advancement between them are both accepted normally, `glitch_count`
  stays 0.
- **`encoder_init`/`encoder_reset` resync `last_state` from the given
  `port_value`**: init/reset with a non-zero starting state, then the
  very next `encoder_update` call for the *next* real edge does not
  spuriously register as an impossible transition (proving the resync
  actually took, not just that the counters got zeroed).
- **`encoder_get_position`/`_get_error_count`/`_get_glitch_count` read
  back exactly what direct struct inspection shows** after a scripted
  sequence (the atomic-read wrapper doesn't change the value, only how
  safely it's read, host build has nothing to tear against so this is
  mostly a "the getters aren't off-by-something" sanity check).
- **Two independent instances sharing one port byte never affect each
  other**: two `encoder_t` on different pin pairs of the same
  simulated PORTB, drive a byte sequence that changes both instances'
  pins across different calls, assert each instance's `position`/
  `error_count`/`glitch_count` matches what driving it alone would have
  produced.

## Milestones

- **Phase 0a, HAL prerequisite.** The GPIO change-interrupt hook design
  above, in both `pic16f87xa-hal` and `pic18fxx5x-hal`: the callback
  registration + weak `RB_IRQHandler` (read-before-clear order), the
  `peripherals/hal_gpio.h` neutral shims, the `epic_dispatch_all_irqs`
  fan-out addition, the sim RBIF modeling (or its documented fallback),
  and the host tests listed above, landed and green in each HAL's own
  existing test suite before Phase 0b starts.
- **Phase 0b, `epic-encoder` scaffolding.** Repo layout above;
  `CMakeLists.txt` mirroring `epic-tick/CMakeLists.txt`'s `EPIC_FAMILY`
  selection; empty `encoder.h` with the struct/prototypes above; `mcu/*/
  Makefile` stubs.
- **Phase 1, core engine + full test suite.** `encoder_init`/`_reset`/
  `_update`/`_get_position`/`_get_error_count`/`_get_glitch_count`,
  `QUAD_TABLE`, implementing the algorithm exactly as specified,
  `tests/test_encoder.c` covering every case above. This is the only
  correctness-bearing phase for the module itself.
- **Phase 2, examples.** `example_encoder_hal.c` (two instances, real
  `EPIC_GPIO_RegisterChangeCallback` wiring, sim-driven pin sequences) and
  `example_encoder_pid_loop.c` (one instance feeding `pid_update()`,
  linking `epic-pid`, the direct demonstration of this module's actual
  purpose).
- **Phase 3, cross-compile sanity check.** `mcu/*/Makefile` for both
  families against real XC8, linking the now-extended HAL and
  `epic-tick`; report flash/RAM footprint per `encoder_t` instance
  (expect a small plain struct, roughly 16-18 bytes, similar order to
  `pid_t`).
- **Phase 4, docs.** `docs/ARCHITECTURE.md` (every rationale above, plus
  whichever RBIF-sim-modeling approach Phase 0a actually landed on and
  why), `docs/API.md` (the port-byte/bit-position wiring convention, the
  "at most two encoders per port, PORTB only" hardware ceiling, the
  direction-swap-via-pin-order note, the 1 ms glitch-gate resolution
  caveat), `epic-encoder/README.md`, a root `README.md` component-table
  row, and a short addendum to each HAL's own `MANUAL.md` documenting the
  new `EPIC_GPIO_RegisterChangeCallback`/`RB_IRQHandler` surface (the
  manuals document per-peripheral register behavior already, this is a
  genuinely new piece of that surface, not implementation detail
  internal to `epic-encoder`).

Each phase should land as its own reviewable change with its tests green
before the next phase starts, the same granularity the rest of this
repo's history already uses.

## Explicitly out of scope (future work, not this plan)

- **Velocity/RPM computation.** Composes at the call site (two
  `encoder_get_position()` reads across a known `epic-tick` interval),
  exactly like `epic-adcfilter` composes with anything wanting filtered
  ADC readings. Nothing about it belongs inside `encoder_update`.
- **Z/index-pulse (absolute homing) support.** A third, infrequent
  single-pulse-per-revolution channel some encoders have; a different
  problem (single-edge-per-revolution capture, not quadrature decode) and
  would use a spare INTx pin on PIC18, not RB<7:4>. Separate module or
  extension if a concrete application needs homing.
- **A third+ encoder, or one on PORTA/C/D.** Not possible on this
  hardware: mismatch-style IOC exists only on PORTB on both families.
  Document as a hard ceiling, not a roadmap item.
- **Sub-millisecond glitch filtering.** Would need a free-running
  Timer1-based microsecond capture instead of `epic-tick`'s 1 ms
  resolution; meaningfully more complexity, not built until a concrete
  encoder's bounce characteristics actually demand it.
