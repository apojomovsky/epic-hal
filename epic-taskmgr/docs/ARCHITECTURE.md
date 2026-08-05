# Architecture

The design of the 8-bit PIC task manager: the cooperative model, the tick
source, concurrency, RAM scaling, and the constraints of an 8-bit PIC with
banked RAM and an 8-level hardware stack.

## Why cooperative, not preemptive

A preemptive RTOS needs a separate stack per task. These 8-bit PIC cores
have no software stack, only an 8-level hardware call stack, and modest
banked RAM (192 B on a PIC16F873A up to 2 KB on a PIC18F4550), so per-task
stacks and context switching are not feasible. This module provides a
cooperative scheduler instead: tasks run to completion in priority
order on each tick, are never preempted, and return like interrupt handlers.
That makes it a scheduler, not an RTOS.

## The model

1. `task_manager_init()` clears the task table.
2. `task_spawn(fn, arg, period_ticks, priority)` registers and arms a task.
   `period_ticks == 0` means **one-shot**: run once, then free the slot.
3. A periodic **tick** drives the scheduler. The helper
   `task_manager_attach_timer0(reload, prescaler)` wires a HAL Timer0 overflow
   to `task_manager_tick()`. The tick ISR is short: it decrements each enabled
   task's countdown and marks due tasks ready, and never runs user code, so it
   is safe in interrupt context.
4. The main loop calls `task_manager_run()` (or `task_manager_run_once()` in a
   custom loop) to run ready tasks in priority order with interrupts
   re-enabled.

A task keeps its state in storage reached through `arg` (a struct you own),
not in locals, which do not survive between calls. This is the standard
cooperative-scheduler pattern, and it is why the example carries its LED pin
and toggle count in a small `blink_arg_t`.

### Period semantics

The tick logic is "if `countdown == 0` then fire, else decrement." A periodic
task reloads to `period - 1` (not `period`) on each fire, so the spacing is
constant at exactly `period` ticks; otherwise every fire would take
`period + 1` ticks. A one-shot (`period == 0`) uses `countdown == 0`, so it is
ready on the first tick after it is spawned, and its slot is freed once it
runs.

A periodic task's first fire happens after `period` ticks: a period-5 task
first runs at tick 5, not tick 0.

### One-shots free their slot

When a one-shot task finishes, `task_manager_run_once()` clears its slot
atomically (under a short critical section). A periodic task that re-spawns
one-shots, like the example's supervisor, therefore does not permanently
consume a slot per spawn; the same slot is recycled. This is what lets the
supervisor spawn blips indefinitely without exhausting the table.

## Two execution models, one source

The scheduler and its examples build for two execution models with no
`#ifdef`, reusing the HAL's host/target harness seam
(`core/epic_harness.h`):

- **Host simulator**: a normal program. `task_manager_run()` calls
  `epic_harness_tick()` each iteration, which pumps the simulated
  Timer0; its overflow ISR calls `task_manager_tick()` synchronously, marking
  due tasks ready; `task_manager_run_once()` then runs them. The loop is
  bounded by the harness so the test can report pass/fail.
- **Real target**: firmware. The Timer0 overflow fires asynchronously from
  hardware; `task_manager_tick()` runs in interrupt context and marks tasks
  ready; the main loop's `task_manager_run_once()` runs them. The loop never
  exits.

On the host, `epic_harness_log` is a `printf`; on the target it is a
no-op (no stdout). That is how the example streams a dispatch log on the host
while staying build-agnostic.

> The sim models the Timer0 overflow and IRQ plumbing, not real Fosc timing
> (as in the HAL's `example_idle_blink`), so periods are in ticks: deterministic
> on the sim, approximately wall-clock on the target. On a 20 MHz target with
> prescaler 1:256 and reload 61, one tick is ~9.98 ms.

## Concurrency: keeping the shared table race-free

`task_manager_tick()` runs in interrupt context while
`task_manager_run_once()` and the mutators run in main context. They share one
thing: the task table. Three rules keep it race-free:

1. `run_once` snapshots the ready set under a brief critical section, clears
   each ready flag, then runs the tasks with interrupts re-enabled, so a tick
   that arrives during a long task arms the task for the next round instead of
   being lost, and the ISR is not blocked by long user code.
2. The mutators (`task_spawn`, `task_stop`, `task_set_period`) fill their slot
   under a brief critical section, so a tick that fires mid-spawn never sees a
   half-initialized task control block. This makes `task_spawn` safe to call
   from inside a running task (e.g. a supervisor that spawns one-shot children
   at runtime).
3. A one-shot is freed atomically after it runs (the same critical section),
   so the tick ISR never observes a half-freed slot.

The critical sections use the HAL's `EPIC_IRQ_Disable/Restore`, which
save and restore the GIE bit. On the host sim the tick is delivered
synchronously from inside `epic_harness_tick()`, so tick and `run_once`
never truly overlap; the critical sections are a no-op there but remain
correct on the target. `task_manager_ticks()` also takes a critical section
because a 16-bit read is not atomic on an 8-bit PIC.

## RAM scaling

Each task control block is ~12 B: two 3-byte PIC16 pointers for `fn` and
`arg`, two `uint16` for `period`/`countdown`, a `priority` byte, and a packed
`flags` byte. The state bits (used / enabled / ready) are packed into one
`flags` byte rather than three bools, which keeps the TCB small enough to bank
cleanly into the smallest parts.

`TASK_MGR_MAX_TASKS` defaults scale to the part:

| Part | RAM | Default slots | Table size |
|---|---|---|---|
| PIC16F873A / 874A | 192 B | 6 | ~72 B |
| PIC16F876A / 877A | 368 B | 8 | ~96 B |

Override with `-DTASK_MGR_MAX_TASKS=N` before including `task_manager.h`.

> Keep arg structs pointer-free. A banked pointer is 3 bytes on the PIC16 and
> may not place in the 192 B of the 873A/874A; a pin index plus a `uint8_t`
> counter banks cleanly and holds per-task state directly.

## Constraints

- **No preemption.** A task that overruns its slot delays the others. Keep
  tasks short: do a little work and return; the scheduler calls you again next
  period. For long work, break it into a small state machine stored in the
  task's `arg`.
- **One stack.** All tasks share the ordinary call stack, so the PIC16's
  8-level hardware stack is the depth limit. The example's
  supervisor → `task_spawn` path stays well within it; XC8 warns if a deeper
  chain risks overflow.
- **Tick resolution** is set by the Timer0 reload and prescaler (~10 ms here).
  The finest period is one tick; periods are in ticks, not milliseconds.
