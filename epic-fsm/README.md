# epic-fsm

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](../LICENSE)
[![Runs on: host & silicon](https://img.shields.io/badge/runs%20on-host%20%26%20silicon-orange.svg)](#quick-start)

A vendor-agnostic, table-driven finite state machine engine. Unlike every
other module in this repo, it has **no HAL dependency and no per-family
backend** — a state machine is pure control-flow logic, so the same
`src/fsm.c` compiles unchanged for the host, PIC16, and PIC18. It composes
with [epic-taskmgr](../epic-taskmgr) (or anything else) by staying decoupled
from it: a task callback just owns an `fsm_t` and calls `fsm_dispatch()`.

> 📖 **Documentation**: [Architecture](docs/ARCHITECTURE.md) · [API reference](docs/API.md) · [Implementation plan](../docs/epic-fsm-plan.md)

## Why

The whole point is being easy to **assemble** and to **understand at a
glance**: a machine's entire behavior lives in one `static const` array in
the caller's own file — no `switch` per state scattered across functions.

```c
enum { ST_RED, ST_GREEN, ST_YELLOW, ST_FAULT };
enum { EV_TIMER, EV_FAULT };

static const fsm_transition_t light_transitions[] = {
    { ST_RED,        EV_TIMER, NULL, NULL,       ST_GREEN  },
    { ST_GREEN,      EV_TIMER, NULL, on_caution, ST_YELLOW },
    { ST_YELLOW,     EV_TIMER, NULL, NULL,       ST_RED    },
    { FSM_ANY_STATE, EV_FAULT, NULL, on_fault,   ST_FAULT  },
};

fsm_t light;
FSM_INIT(&light, light_transitions, ST_RED, NULL);
fsm_dispatch(&light, EV_TIMER);
```

Four columns — `state`, `event`, `guard`, `action`/`next_state` — tell the
whole story. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the
reasoning behind the wildcard row, the `void *ctx` convention, and the
guard-fallthrough dispatch semantics.

## Features

- **Table-driven**: the whole machine is one `static const` array, placed in
  flash, not RAM.
- **`FSM_ANY_STATE` wildcard**: a row that fires from any current state (a
  global fault/reset transition, typically).
- **Guarded transitions with fallthrough**: multiple candidate rows for the
  same `(state, event)` pair, disambiguated by guard functions evaluated in
  table order.
- **No hidden state**: `ctx` is explicit and opaque; multiple instances,
  even sharing one table, never interfere with each other.
- **One implementation, every target**: no `#ifdef`, no per-family variant —
  the host test suite proves the exact code that ships on PIC16 and PIC18.
- **Tiny footprint**: ~14 B RAM per instance on PIC16F877A, ~11 B on
  PIC18F4550 (measured, see ARCHITECTURE.md); table size doesn't touch RAM
  at all.

## Quick start

### Host

```sh
cmake -B build -S .
cmake --build build

./build/test_fsm                 # unit tests
./build/example_traffic_light    # dependency-free demo
ctest --test-dir build           # or just: cd build && ctest
```

To also build the `epic-taskmgr` composition demo:

```sh
cmake -B build -S . -DEPIC_FSM_BUILD_TASKMGR_EXAMPLE=ON
cmake --build build
./build/example_taskmgr_integration
```

### Real target (cross-compile sanity check)

`fsm.c` has no per-family variant, so this is a portability/footprint check,
not a second correctness gate — correctness is already fully proven on host.

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

python3 scripts/epic_build.py build --module epic-fsm --mcu 16F877A --run
python3 scripts/epic_build.py build --module epic-fsm --mcu 18F4550 --run
```

## Use it in your own firmware

```c
#include "fsm.h"

enum { ST_IDLE, ST_ARMED };
enum { EV_PRESS, EV_TIMEOUT };

static const fsm_transition_t button_transitions[] = {
    { ST_IDLE,  EV_PRESS,   NULL, on_arm, ST_ARMED },
    { ST_ARMED, EV_TIMEOUT, NULL, NULL,   ST_IDLE  },
};

static fsm_t g_button;

void button_task(void *arg) {
    /* fsm_dispatch() is the entire integration surface — nothing else about
     * this task needs to know an FSM is involved. */
    fsm_dispatch(&g_button, some_event_read_this_tick());
}

int main(void) {
    FSM_INIT(&g_button, button_transitions, ST_IDLE, NULL);
    /* ...wire button_task into your scheduler or main loop... */
}
```

See the [API reference](docs/API.md) for the full surface and
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for why it's shaped this way.

## License

MIT; see the top-level [LICENSE](../LICENSE).
