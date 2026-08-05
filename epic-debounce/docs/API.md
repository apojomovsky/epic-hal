# `epic-debounce` API reference

Authoritative declarations: [`include/debounce.h`](../include/debounce.h).

### `typedef bool (*debounce_read_fn)(void *ctx)`
Pin-read callback. Returns `true` when the pin currently reads "active." The
callback resolves active-high vs. active-low; the debounce core never sees a
HAL type.

### `void debounce_init(debounce_t *db, debounce_read_fn read, void *read_ctx, uint16_t debounce_ms)`
Initialize one instance. Reads the pin once and sets both the stable and
candidate state to that initial reading (so a button already held at boot
does not spuriously fire a PRESSED event after the window elapses). `debounce_ms`
is the stability window (e.g. 20-50 ms).

### `debounce_event_t debounce_poll(debounce_t *db)`
Poll the input once (call per scheduler tick or main-loop iteration). Returns
`DEBOUNCE_EVENT_PRESSED`, `DEBOUNCE_EVENT_RELEASED`, or
`DEBOUNCE_EVENT_NONE`. Reads the pin via the callback, applies the
timestamp-comparison debounce, and commits a new stable state only after the
raw reading has held for the full `debounce_ms` window.

### `bool debounce_is_active(const debounce_t *db)`
The last committed (debounced) stable state, `true` if active. Reflects the
committed state, not the raw/candidate state.

## Usage

```c
/* A button on RA0, active-high, 20 ms debounce. */
static bool read_button(void *ctx) {
    (void)ctx;
    return EPIC_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
}

debounce_t btn;
debounce_init(&btn, read_button, NULL, 20);

/* In the main loop or a task: */
debounce_event_t ev = debounce_poll(&btn);
if (ev == DEBOUNCE_EVENT_PRESSED)  { /* button just settled to active */ }
if (ev == DEBOUNCE_EVENT_RELEASED) { /* button just settled to inactive */ }

/* Query the debounced state without polling: */
if (debounce_is_active(&btn)) { /* button is held */ }
```

## Cheat sheet

| Symbol | Purpose |
|---|---|
| `debounce_read_fn` | pin-read callback (true = active) |
| `debounce_t` | one instance (plain data, one per input) |
| `debounce_init` | set up + read the pin once |
| `debounce_poll` | one poll → PRESSED / RELEASED / NONE |
| `debounce_is_active` | committed stable state |
| `DEBOUNCE_FLAG_STABLE` / `_CANDIDATE` | packed-flags bits (internal) |
