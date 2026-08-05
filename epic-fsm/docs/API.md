# API reference

Full surface of the FSM engine. Header: `include/fsm.h`. Design rationale
and the semantics behind each decision are covered in
[ARCHITECTURE.md](ARCHITECTURE.md); a quick start is in the
[README](../README.md).

## Types & constants

```c
#ifndef FSM_STATE_TYPE
#define FSM_STATE_TYPE uint8_t
#endif
typedef FSM_STATE_TYPE fsm_state_t;
typedef FSM_STATE_TYPE fsm_event_t;

#define FSM_ANY_STATE  ((fsm_state_t)-1)

typedef bool (*fsm_guard_fn)(void *ctx);
typedef void (*fsm_action_fn)(void *ctx);

typedef struct {
    fsm_state_t   state;
    fsm_event_t   event;
    fsm_guard_fn  guard;
    fsm_action_fn action;
    fsm_state_t   next_state;
} fsm_transition_t;

typedef struct {
    const fsm_transition_t *table;
    uint8_t                 table_len;
    fsm_state_t             state;
    void                   *ctx;
} fsm_t;
```

### `FSM_STATE_TYPE`

The only build-time knob this library has. `uint8_t` by default (states/events
0..254; 255 is `FSM_ANY_STATE`). `#define FSM_STATE_TYPE uint16_t` (or any
other unsigned integer type) before `#include "fsm.h"` if a machine genuinely
needs more than 255 states or events. Every other property of this library
(handle size, table-in-flash placement, dispatch cost) does not scale with
this choice enough to be worth a second knob.

### `fsm_transition_t`

One row: "when in `state` (or any state, via `FSM_ANY_STATE`) and `event`
arrives, if `guard` allows it (or `guard` is `NULL`), run `action` (if
non-`NULL`) and move to `next_state`." A whole machine is a `static const`
array of these — see the README for a worked example.

### `fsm_t`

A running instance: which table it uses, the table's row count, current
state, and the opaque `ctx` passed to every guard/action. Plain data, no
hidden state; multiple instances (even sharing one table) never interfere
with each other.

## Functions

### `void fsm_init(fsm_t *fsm, const fsm_transition_t *table, uint8_t table_len, fsm_state_t initial_state, void *ctx)`

Initialize a machine instance.

- `table`, the transition table; must outlive `fsm` (a `static const` array
  is the normal case, placed in flash by the compiler).
- `table_len`, number of rows in `table`. Prefer `FSM_INIT` below, which
  computes this for you.
- `initial_state`, the machine's starting state.
- `ctx`, opaque pointer passed to every guard/action; may be `NULL` if none
  of them need it.

### `FSM_INIT(fsm, table, initial_state, ctx)`

```c
#define FSM_INIT(fsm, table, initial_state, ctx) \
    fsm_init((fsm), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), \
             (initial_state), (ctx))
```

Convenience macro wrapping `fsm_init` that computes `table_len` from the
array itself, so a row added to the table can never silently fall outside
the length passed in.

> **Must be called with the actual array**, not a pointer to it — a function
> parameter of array type has decayed to a pointer, and `sizeof` on it would
> silently give the pointer's size instead of the array's. Call `FSM_INIT`
> where the table is declared or still in scope by its array type (as in
> every example in this repo); if you need to initialize a machine from
> inside a function that only has a pointer to the table, call `fsm_init`
> directly with the real row count instead.

### `bool fsm_dispatch(fsm_t *fsm, fsm_event_t event)`

Feed one event to the machine. Scans `table` top-to-bottom for the first row
whose `state` matches the current state (or is `FSM_ANY_STATE`) *and* whose
`event` matches, skipping rows whose `guard` rejects the event and
continuing the scan (see ARCHITECTURE.md for why this fall-through is the
chosen behavior, with the canonical example). When a row fires: its
`action` runs (if non-`NULL`), then the state becomes that row's
`next_state`.

Returns `true` if a row fired, `false` if no row matched at all (state is
left unchanged in that case). This library imposes no policy on unhandled
events — no logging, no assert — so it stays usable in a firmware image
with no logging facility. Check the return value if the caller needs to
know.

### `fsm_state_t fsm_state(const fsm_t *fsm)`

Current state of the machine.

## Cheat sheet

| Function/macro | Purpose |
|---|---|
| `fsm_init(fsm, table, table_len, initial, ctx)` | Initialize a machine explicitly. |
| `FSM_INIT(fsm, table, initial, ctx)` | Same, computing `table_len` via `sizeof`; prefer this. |
| `fsm_dispatch(fsm, event)` | Feed an event; returns `true` if a row fired. |
| `fsm_state(fsm)` | Query the current state. |
| `FSM_ANY_STATE` | Wildcard row: matches from any current state. |
| `FSM_STATE_TYPE` | Override before `#include` for >255 states/events. |
