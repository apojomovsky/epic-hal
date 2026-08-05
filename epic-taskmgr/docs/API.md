# API reference

Full surface of the task manager. Headers: `include/task_manager.h`.
Configuration and the concurrency model are covered in
[ARCHITECTURE.md](ARCHITECTURE.md); a quick start is in the
[README](../README.md).

## Types & constants

```c
typedef void (*task_fn_t)(void *arg);   /* a task: runs to completion, returns */
typedef uint8_t task_id_t;               /* opaque task id */
#define TASK_ID_INVALID  ((task_id_t)0xFFU)
#define TASK_MGR_MAX_TASKS  8            /* default; 6 on 192 B parts, see below */
```

A task is a plain function that runs to completion and returns. It is called
with the `arg` passed to `task_spawn`. Persist per-task state in storage reached
through `arg` (a struct you own), not in locals, which do not survive between
calls.

### `TASK_MGR_MAX_TASKS`

Maximum number of simultaneously registered tasks. Each slot costs ~12 B of
RAM (two 3-byte PIC16 pointers + two `uint16` + a flags byte). The default
scales to the part, 6 on the 192 B PIC16F873A/874A, 8 on the 368 B
PIC16F876A/877A, so the table banks cleanly into every device in the family.
Override by defining `TASK_MGR_MAX_TASKS` before including the header.

One-shot tasks free their slot after they run (see `task_manager_run_once`),
so a periodic task that re-spawns one-shots does not permanently consume a slot
per spawn.

## Lifecycle

### `void task_manager_init(void)`

Initialise the scheduler. Clears every task slot and zeroes the tick counter.
Call once before spawning tasks or attaching a tick source. Idempotent.

### `task_id_t task_spawn(task_fn_t fn, void *arg, uint16_t period_ticks, uint8_t priority)`

Register a task and arm it.

- `fn`, entry point (must not be `NULL`).
- `arg`, opaque pointer passed back to `fn` (may be `NULL`).
- `period_ticks`, call interval in scheduler ticks. `0` = one-shot (called
  once on the next tick, then its slot is freed). For periodic tasks the first
  call happens after `period_ticks` ticks.
- `priority`, scheduling priority within a round; lower numbers run first.
  Ties break by spawn order.

Returns the new task id, or `TASK_ID_INVALID` if `fn` is `NULL` or all
`TASK_MGR_MAX_TASKS` slots are in use.

> Safe to call from within a running task (e.g. a supervisor that spawns
> one-shot children). The slot fill is a short critical section, so it does not
> race with the tick ISR.

### `void task_start(task_id_t id)`

Enable a previously stopped task. Its countdown is reset to its period.

### `void task_stop(task_id_t id)`

Disable a task so the scheduler skips it until `task_start`.

### `void task_set_period(task_id_t id, uint16_t period_ticks)`

Change a task’s period at runtime. Takes effect on the next arming.

## The scheduler

### `void task_manager_tick(void)`

Advance the scheduler by one tick. For each enabled task, decrement its
countdown; when it reaches zero, mark the task ready and (for periodic tasks)
reload the countdown to `period - 1`. One-shot tasks are marked ready
immediately on their first tick.

Call this from a timer interrupt service routine, typically the Timer0
overflow wired by `task_manager_attach_timer0`. It is short (O(n),
n ≤ `TASK_MGR_MAX_TASKS`) and never runs user code, so it is safe in interrupt
context.

### `uint16_t task_manager_ticks(void)`

Current tick counter since `task_manager_init`. Wraps at 65535. Takes a brief
critical section (a 16-bit read is not atomic on an 8-bit PIC).

### `uint8_t task_manager_run_once(void)`

Run every task that is ready *now*, in priority order, then return. Each ready
task is snapshotted and its ready flag cleared under a short critical section,
then run with interrupts re-enabled, so a tick arriving during a long task arms
the task for the next round rather than being lost. Non-blocking: if nothing is
ready, returns immediately.

Returns the number of tasks actually run this round. Call repeatedly from your
main loop, or use `task_manager_run` for the common case.

### `void task_manager_run(void)`

The canonical scheduler loop. Equivalent to:

```c
for (;;) {
    epic_harness_tick();    /* host: pumps sim → Timer0 ISR → tick */
    task_manager_run_once();
    EPIC_WDT_Refresh();            /* no-op on the host */
}
```

On the host the harness bounds the loop (so the test can report pass/fail and
return); on a real target it runs forever. Use this for the common case, or
call `task_manager_run_once` directly inside your own loop if you need to do
per-iteration work.

### `uint8_t task_manager_count(void)`

Number of tasks currently registered (used slots), any state.

## Optional tick source

### `void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler)`

Wire a HAL Timer0 overflow to `task_manager_tick` and start it. Configures
Timer0 for internal Fosc/4, the given prescaler assigned to Timer0, the given
reload, and `task_manager_tick` as the overflow callback. The TMR0 interrupt
enable is set; arm it for real by calling `EPIC_IRQ_Restore(1)`
afterwards.

- `reload`, TMR0 reload value (0..255). On a 20 MHz target, prescaler 1:256,
  reload 61 → ~10 ms per tick.
- `prescaler`, a `TIMER0_PrescalerTypeDef` (1:2 .. 1:256).

The tick ISR reloads TMR0 each overflow for a constant period (Timer0 has no
auto-reload, unlike Timer2); writing TMR0 also clears the prescaler
(DS39582B §5.3), so every tick starts a clean count.

> The sim does not model real Fosc timing, it reproduces the overflow/IRQ
> plumbing, not the wall-clock period (same caveat as the HAL’s
> `example_idle_blink`). Periods are therefore in *ticks*, deterministic on the
> sim and ~wall-clock on the target.

You may instead call `task_manager_tick()` from any other timer ISR; the
scheduler does not care where the tick comes from.

## Cheat sheet

| Function | Purpose |
|---|---|
| `task_manager_init()` | Clear the table; call once at startup. |
| `task_spawn(fn, arg, period, prio)` | Register + arm a task; returns its id (or `TASK_ID_INVALID`). `period==0` → one-shot. |
| `task_start(id)` / `task_stop(id)` | Enable / disable a task at runtime. |
| `task_set_period(id, period)` | Change a task’s period. |
| `task_manager_tick()` | Advance one tick, call from a timer ISR. |
| `task_manager_run_once()` | Run all due tasks once (priority order); returns count run. |
| `task_manager_run()` | Canonical loop (bounded on sim, forever on target). |
| `task_manager_ticks()` | Tick counter since init (wraps at 65535). |
| `task_manager_count()` | Number of registered tasks. |
| `task_manager_attach_timer0(reload, prescaler)` | Wire a HAL Timer0 to the tick. |
