# `pic8-fsm`: table-driven finite state machine engine — implementation plan

Status: **implemented**; see `pic8-fsm/docs/ARCHITECTURE.md` for the as-built
design. This document is kept as the original design record (see chat log
for the design discussion it closes out).

## What this is and why it's simple

A vendor-agnostic, table-driven FSM engine. Unlike `pic8-math`
(`docs/pic8-math-plan.md`), this module needs **no per-family backend split
and no inline asm**: a state machine is pure control-flow logic with zero
hardware dependency, so the entire engine is one C99 implementation that
compiles unchanged for the host, PIC16, and PIC18. That has a real payoff for
testing: there is no three-tier problem here. The host unit test suite tests
the literal same `fsm.c` that ships on-target — not a reference
re-implementation standing in for untestable asm, as `pic8-math` had to
accept. Cross-compiling for PIC16/PIC18 via `mcu/*/Makefile` is still worth
doing (catches non-portable C, reports flash/RAM footprint), but it is a
sanity check, not a correctness gate — correctness is already fully proven
on host.

It composes with `pic8-taskmgr` (and anything else) by staying completely
decoupled from it: `fsm.h` includes only `<stdint.h>`/`<stdbool.h>`, never
`pic8_hal.h` or any peripheral header. A task's callback just owns an
`fsm_t` and calls `fsm_dispatch()` — no FSM-specific hook anywhere in
`task_manager.h`, exactly the same "compose by not depending on each other"
relationship the HAL and task manager already have.

## Public API (finalized in design discussion — do not redesign)

```c
/* pic8-fsm/include/fsm.h */
#ifndef FSM_H
#define FSM_H
#include <stdint.h>
#include <stdbool.h>

/* Override before #include if a machine needs >255 states/events.
 * This is the ONE build-time knob this library has; every other property
 * (handle size, table-in-flash, dispatch cost) is fixed-cost enough on
 * both PIC16 and PIC18 that no other axis is worth making tunable. */
#ifndef FSM_STATE_TYPE
#define FSM_STATE_TYPE uint8_t
#endif

typedef FSM_STATE_TYPE fsm_state_t;
typedef FSM_STATE_TYPE fsm_event_t;

/* All-ones sentinel — correct for uint8_t or a wider FSM_STATE_TYPE override. */
#define FSM_ANY_STATE  ((fsm_state_t)-1)

typedef bool (*fsm_guard_fn)(void *ctx);
typedef void (*fsm_action_fn)(void *ctx);

typedef struct {
    fsm_state_t   state;       /* row applies when current state matches, or FSM_ANY_STATE */
    fsm_event_t   event;
    fsm_guard_fn  guard;       /* NULL = always allowed */
    fsm_action_fn action;      /* NULL = transition with no side effect */
    fsm_state_t   next_state;
} fsm_transition_t;

typedef struct {
    const fsm_transition_t *table;
    uint8_t                 table_len;
    fsm_state_t             state;
    void                   *ctx;      /* opaque; every guard/action casts it back */
} fsm_t;

void fsm_init(fsm_t *fsm, const fsm_transition_t *table, uint8_t table_len,
              fsm_state_t initial_state, void *ctx);

/* Convenience: computes table_len via sizeof/sizeof at the call site.
 * MUST be called with the actual array, not a decayed pointer — document
 * this footgun prominently in the header doc comment and API.md. */
#define FSM_INIT(fsm, table, initial_state, ctx) \
    fsm_init((fsm), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), \
             (initial_state), (ctx))

bool        fsm_dispatch(fsm_t *fsm, fsm_event_t event);  /* true if a row fired */
fsm_state_t fsm_state(const fsm_t *fsm);

#endif
```

Locked-in semantics from the design discussion, restated so the
implementation doesn't drift from them:

- **Wildcard `FSM_ANY_STATE` rows stay in core** (cheap: one extra `||` per
  row scanned) — a global fault/reset transition is common enough, and
  bolting it on later would break the table format, so it ships now.
- **`ctx` is `void *`, no typed-wrapper macro.** One shared `fsm_dispatch()`
  for every machine in the firmware, not a macro-expanded dispatch per
  context type (which would duplicate the scan loop and cost flash on every
  additional FSM type declared). The convention — cast `ctx` back on the
  first line of every guard/action — goes in the header doc comment with an
  example, not left implicit.
- **Guard rejection continues scanning, it does not stop the dispatch.**
  Rows are evaluated top-to-bottom; the first row whose `(state, event)`
  matches *and* whose guard passes (or has none) wins. This lets one
  `(state, event)` pair have multiple candidate rows disambiguated by
  guards — e.g. a `COIN` event that goes to `UNLOCKED` if credit is
  sufficient, falls through to a `buzz` action staying in `LOCKED`
  otherwise. Document this explicitly with exactly that example, since it's
  the one semantic a reader can't get right from the struct shape alone.

## Repo layout (single implementation — no `host/`/`pic16/`/`pic18/` split)

```
pic8-fsm/
  CMakeLists.txt                    # host static lib + unit tests; no family selection needed
  include/fsm.h
  src/fsm.c
  examples/
    example_traffic_light.c         # pure-host, zero HAL dependency — proves the API reads
                                     # at a glance with no other module in scope
    example_taskmgr_integration.c    # an fsm_t driven from a pic8-taskmgr task callback,
                                     # proving the "compose by not depending on each other" claim
  tests/
    test_fsm.c                       # exhaustive: init, dispatch match/no-match, ANY_STATE,
                                      # guard-reject-falls-through, multiple independent
                                      # instances sharing one table, FSM_INIT table_len macro
  mcu/
    pic16f87xa-fsm-mplabx/Makefile   # cross-compile sanity check + flash/RAM size report
    pic18fxx5x-fsm-mplabx/Makefile   # same for PIC18
  docs/
    ARCHITECTURE.md
    API.md
  README.md
```

## Milestones

- **Phase 1 — core engine + full test suite.** `fsm.h`/`fsm.c`,
  `CMakeLists.txt`, `tests/test_fsm.c` covering every locked-in semantic
  above. This is the only correctness-bearing phase; everything after is
  demonstration and portability sanity-checking.
- **Phase 2 — examples.** `example_traffic_light.c` (no HAL — the
  at-a-glance proof) and `example_taskmgr_integration.c` (links
  `pic8-taskmgr`, proves zero coupling).
- **Phase 3 — cross-compile sanity check.** `mcu/*/Makefile` for both
  families against real XC8; report flash/RAM footprint for a small table
  (a handful of rows) since that's the number an embedded user actually
  wants before adopting this.
- **Phase 4 — docs.** `docs/ARCHITECTURE.md` (the design decisions above,
  with rationale), `docs/API.md`, `pic8-fsm/README.md`, and a root
  `README.md` component-table row mirroring the existing entries.

Not committing any of this until the user reviews — implementation proceeds
directly in this session per their instruction, but git history stays theirs
to curate.
