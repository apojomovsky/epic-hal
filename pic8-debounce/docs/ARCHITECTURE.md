# `pic8-debounce` architecture

A vendor-agnostic, instantiable digital-input debouncer, one `debounce_t`
per input, no global state, no per-family backend, no inline asm.

## What it is

Given a raw, possibly-bouncy pin read, decide when the *stable* state has
actually changed and emit a press/release edge event. Multiple instances
cover multiple inputs. The algorithm is a timestamp comparison (not a hardware
operation), so `src/debounce.c` is one file that compiles unchanged for host,
PIC16, and PIC18, the host unit suite tests the exact code that ships.

## Vendor-agnostic: the read callback

The caller supplies a `debounce_read_fn` returning `true` when the pin reads
"active", the callback resolves active-high vs. active-low. The debounce core
never sees a HAL type, so it's equally useful over a HAL GPIO pin
(`EPIC_GPIO_ReadPin`), an I2C-expander bit (`pic8-bus`), or a mock pin in a
test. This is the design choice that makes the module "reusable on any gpio."

## Depends on `pic8-tick` directly (not an injected clock)

The algorithm needs elapsed-time. It calls `pic8_tick_get()` /
`pic8_tick_elapsed_since()` directly, not a second injected callback, so the
host test suite exercises genuinely real timing semantics under simulated tick
advancement (a stronger correctness signal for an algorithm whose entire job is
timing). Only `src/debounce.c` includes `pic8_tick.h`; the public header
`debounce.h` is dependency-free (two `#include`s: `<stdint.h>`, `<stdbool.h>`).

## Poll-driven, not interrupt-driven

Call `debounce_poll()` once per scheduler tick or main-loop iteration. Simpler
than interrupt-on-change wiring, and naturally robust to contact bounce (the
whole point is "don't trust the first transition," which a poll loop does for
free). Interrupt-driven debounce is a plausible future addition, not in scope.

## Does not use `pic8-fsm` internally

A 2-state debounce (candidate vs. stable) is a direct timestamp comparison;
expressing it as an `fsm_t` transition table would cost more flash for no
clarity gain. `pic8-fsm` composability holds at the *call site*:
`debounce_poll()`'s return value is a perfectly good `fsm_dispatch()` event.

## The algorithm

```
raw = read(ctx)
if raw != candidate_state:
    candidate_state = raw
    candidate_since = pic8_tick_get()
    return NONE
if raw != stable_state and elapsed_since(candidate_since) >= debounce_ms:
    stable_state = raw
    return raw ? PRESSED : RELEASED
return NONE
```

`candidate_state` and `stable_state` are the two bits of `flags`
(`DEBOUNCE_FLAG_CANDIDATE`, `DEBOUNCE_FLAG_STABLE`). `debounce_init` reads the
pin once and sets both to that initial reading (with `candidate_since` =
`pic8_tick_get()`), so a button already held down at boot does NOT spuriously
fire a PRESSED event after the window elapses.

## Scope: press/release edges only

`debounce_poll` emits `PRESSED`/`RELEASED` only. Click detection and
long-press are built by a caller composing edge events with a timestamp or a
small `pic8-fsm` machine one level up, not built into this module.

## Footprint

`debounce_t` is a small plain struct: one function pointer + one `void*` +
one `uint16_t` + one `uint32_t` + one `uint8_t` = 8 bytes on an 8-bit PIC
(pointers are 1-2 bytes). Comparable to `pic8-fsm`'s `fsm_t`.
