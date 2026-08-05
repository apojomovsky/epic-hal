# Architecture

The design of `epic-fsm`: why it's vendor-agnostic, why the transition table
is shaped the way it is, the three semantic decisions that were made
deliberately rather than obviously, and why this module needed neither a
per-family backend nor a three-tier test strategy the way `epic-math` did.

## Why vendor-agnostic, not HAL-integrated

A finite state machine is pure control-flow logic: given a current state and
an event, decide the next state and run a side effect. None of that touches
a register, a pin, or a peripheral. Coupling the engine to `epic_hal.h` or
any family header would buy nothing and cost portability — the same engine
should be usable in a project with no PIC HAL in it at all.

It still composes cleanly with `epic-taskmgr` (and anything else) by staying
decoupled from it, the same way the HAL and the task manager compose without
either depending on the other: a task's callback just owns an `fsm_t` and
calls `fsm_dispatch()`. There is no FSM-specific hook anywhere in
`task_manager.h`, and no task-manager awareness anywhere in `fsm.h`. See
`examples/example_taskmgr_integration.c` for the demonstration — the whole
integration surface is one `fsm_dispatch()` call inside a task function.

## Why table-driven

The task's stated priority was being "easy to assemble and understand at a
glance." A `static const fsm_transition_t[]` array puts the entire machine's
behavior in one place, in the caller's own file, as a literal, code-reviewed
table — no `switch` per state scattered across functions, no hidden control
flow. It reads the same whether the reader is looking at a 3-row traffic
light or a 30-row protocol handler: same four columns, same scan.

The one real cost against a hand-rolled per-state `switch`: `fsm_dispatch`
is a linear scan, O(rows), and guard/action are indirect calls through
function pointers rather than inlined code. For the realistic size of a
table in embedded use (measured in dozens of rows, not thousands), this is
immaterial next to the clarity win — see "Footprint" below for what it
actually costs.

## Three semantics, decided deliberately

These were discussed and locked in explicitly (not the "obvious" reading of
the struct), so they're restated here with the reasoning, not just the
behavior:

### 1. `FSM_ANY_STATE` is core, not an extension

A row with `state == FSM_ANY_STATE` matches regardless of the machine's
current state — the mechanism for a global transition, like a fault or
reset event valid from anywhere. This shipped in the first version rather
than being added later because it would have forced a table-format change:
every existing hand-written table would need its row-matching semantics
re-read once wildcard rows became possible. The cost of including it now is
one extra `||` comparison per row scanned — free by comparison.

### 2. `ctx` is `void *`, not a typed pointer

The alternative considered was a macro that generates a typed dispatch
function per context struct, so guard/action signatures would show the real
type instead of `void *`. Rejected: that would duplicate `fsm_dispatch`'s
scan loop once per FSM type declared in a firmware image, which costs flash
for every additional machine — a real concern on parts with a few KB of
flash total. The chosen convention instead is that every guard/action casts
`ctx` back on its first line:

```c
static bool has_sufficient_credit(void *ctx) {
    turnstile_ctx_t *c = ctx;
    return c->credit_cents >= TURNSTILE_FARE_CENTS;
}
```

One `fsm_dispatch` serves every machine in the firmware, and `ctx` can hold
any shape a caller needs without ever requiring a change to this library —
which is the more important property for a library the task explicitly
wanted to "not have to touch again."

### 3. A rejected guard continues the scan; it does not stop dispatch

`fsm_dispatch` evaluates rows top-to-bottom. If a row's `(state, event)`
matches but its `guard` returns `false`, the scan keeps going rather than
concluding "no match." This is what lets one `(state, event)` pair have
several candidate rows disambiguated by guards, most simply as a
guarded-row-then-unconditional-fallback pair:

```c
{ ST_LOCKED, EV_COIN, has_sufficient_credit, do_unlock,     ST_UNLOCKED },
{ ST_LOCKED, EV_COIN, NULL,                  buzz_rejected, ST_LOCKED   },
```

A `COIN` event with insufficient credit falls through the first row to the
second, which buzzes and stays `LOCKED`. Without fall-through, expressing
this would require folding the credit check into a single action that
internally branches — which is exactly the hidden, un-glanceable control
flow this library exists to avoid. `tests/test_fsm.c`'s
`test_guard_fallthrough` encodes this exact table as a regression.

## `FSM_STATE_TYPE`: the one build-time knob

States and events are stored as `FSM_STATE_TYPE`, `uint8_t` by default
(0..254; 255 is `FSM_ANY_STATE`). Overridable via `#define FSM_STATE_TYPE
<type>` before including the header, for the rare machine needing more than
255 states or events. This is deliberately the *only* configuration surface
the library has: handle size, table-in-flash placement, and dispatch cost
are fixed-cost enough on both PIC16 and PIC18 that no other axis earns a
flag. Compare to `epic-taskmgr`'s `TASK_MGR_MAX_TASKS`, which follows the
same override-before-include convention for the one thing that library
needed tunable.

## Why no per-family backend, and what that buys for testing

Every other module in this repo (`pic16f87xa-hal`, `pic18fxx5x-hal`, and
`epic-math`'s inline-asm primitives) has a PIC16 body and a PIC18 body
because the underlying registers or instruction sets genuinely differ. This
engine has neither: `src/fsm.c` is one file, no `#ifdef`, and it is the
literal file that links into the host build, the PIC16 cross-compile, and
the PIC18 cross-compile alike.

That collapses what would otherwise be a three-tier testing problem (as
`epic-math`'s plan has to accept, since inline asm can't be executed by this
repo's host "sim") down to one tier that is also the strongest one: the host
unit suite (`tests/test_fsm.c`) tests the exact code that ships on-target,
not a stand-in reference implementation. `mcu/*/Makefile` cross-compiling
`src/fsm.c` with real XC8 for both families is still done (see
`mcu/target_sizecheck.c`), but it is a portability and footprint sanity
check, not a second correctness gate.

## Footprint

Measured via `mcu/*/Makefile` with a 2-row table (`target_sizecheck.c`),
real XC8 v3.10:

| Target | Program space | Data space |
|---|---|---|
| PIC16F877A | 165 words (2.0% of 8 KW) | 14 B (3.8% of 368 B) |
| PIC18F4550 | 320 B (1.0% of 32 KB) | 11 B (0.5% of 2 KB) |

Data space is just the `fsm_t` handle (a table pointer, a length byte, a
state byte, a context pointer) — the transition table itself is `static
const`, so XC8 places it in flash, not RAM. Program space scales with the
number of distinct `fsm_dispatch` call sites' surrounding code and the
guard/action bodies a real machine defines, not with table row count (more
rows are more flash-resident data, not more code).

## Constraints

- **Row count is `uint8_t`-limited** (max 255 rows per table via
  `fsm_init`'s `table_len` parameter) — ample for any hand-written table; if
  a generated table ever needs more, that's a sign the table should be
  split into cooperating machines rather than the limit raised.
- **No entry/exit hooks.** This is a Mealy-style engine (side effects on
  transitions, not on state entry/exit) by design — one concept instead of
  two keeps a table's four columns telling the whole story. A state that
  needs "always do X on entry regardless of which edge arrived" is
  expressible today by repeating the same `action` on every row that leads
  to that state; if that repetition becomes real friction in practice, an
  optional per-state entry table is the natural extension point, additive
  to the existing table format rather than a breaking change to it.
- **No built-in unhandled-event policy.** `fsm_dispatch` returns `false` and
  changes nothing; whether that's silently ignored, logged, or asserted is
  entirely the caller's decision. Keeps the library usable in a firmware
  image with no logging facility at all.
