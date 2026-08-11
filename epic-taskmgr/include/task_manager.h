/**
 * Cooperative (non-preemptive) task scheduler for 8-bit PIC, family-
 * agnostic via `epic_hal.h`. A single periodic tick (typically Timer0)
 * marks due tasks ready; `task_manager_run_once()` runs them to
 * completion in priority order, no preemption, one shared call stack.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdint.h>
#include "peripherals/hal_timer0.h"   /* Timer0 tick source (family-neutral) */

/**
 * Maximum simultaneously registered tasks (~12 B/slot); scales via
 * EPIC_FAMILY_RAM_BYTES: 6 on 192 B parts, 8 otherwise. Override with
 * `#define TASK_MGR_MAX_TASKS` before including this header.
 */
#ifndef TASK_MGR_MAX_TASKS
#  if EPIC_FAMILY_RAM_BYTES <= 192
#    define TASK_MGR_MAX_TASKS  6
#  else
#    define TASK_MGR_MAX_TASKS  8
#  endif
#endif

/** Opaque task identifier returned by @ref task_spawn. */
typedef uint8_t task_id_t;

/** Sentinel for "no task" / an invalid spawn. */
#define TASK_ID_INVALID  ((task_id_t)0xFFU)

/**
 * Task entry point: called with the `arg` passed to task_spawn, runs to
 * completion and returns. Nothing else runs while it does; persist
 * per-task state through `arg`, not locals.
 */
typedef void (*task_fn_t)(void *arg);

/** Task control block, one of TASK_MGR_MAX_TASKS fixed slots; all fields
 *  internal, tasks addressed by task_id_t. */
typedef struct {
    task_fn_t   fn;          /**< Entry point (NULL in a free slot). */
    void       *arg;
    uint16_t    period;      /**< Period in ticks; 0 = one-shot. */
    uint16_t    countdown;
    uint8_t     priority;    /**< Lower number = runs first within a round. */
    uint8_t     flags;       /**< Packed task flags. */
} task_t;

/* Packed into task_t.flags. */
#define TM_FLAG_USED     0x01U   /**< Slot is allocated. */
#define TM_FLAG_ENABLED  0x02U   /**< Slot is scheduled. */
#define TM_FLAG_READY    0x04U   /**< Due this round. */

/**
 * Initialise the scheduler: clears every task slot and zeroes the tick
 * counter. Call once before spawning tasks or attaching a tick source.
 * Idempotent.
 */
void task_manager_init(void);

/**
 * Register a task and arm it. `fn` must not be NULL; returns
 * TASK_ID_INVALID if it is, or when all slots are in use.
 *
 * `period_ticks` 0 = one-shot (fires once, then frees its slot).
 * `priority`: lower runs first within a round; ties break by spawn order.
 * Safe to call from a running task (e.g. a supervisor spawning one-shot
 * children): the slot fill does not race with the tick ISR.
 */
task_id_t task_spawn(task_fn_t fn, void *arg, uint16_t period_ticks,
                     uint8_t priority);

/** Enable a previously stopped task. Its countdown is reset to its period. */
void task_start(task_id_t id);

/** Disable a task so the scheduler skips it until @ref task_start. */
void task_stop(task_id_t id);

/** Change a task's period at runtime. Takes effect on the next arming. */
void task_set_period(task_id_t id, uint16_t period_ticks);

/**
 * Re-arm a task: restart its countdown from the full period and ensure
 * it is enabled, without changing the period or freeing the slot (the
 * debounce / feed-the-watchdog verb). Safe to call from a running task.
 */
void task_reset(task_id_t id);

/**
 * Advance the scheduler one tick: decrement each enabled task's countdown,
 * marking it ready (and reloading periodic ones) at zero. Call from a
 * timer ISR (typically Timer0); it never runs user code, so it is safe
 * in interrupt context.
 */
void task_manager_tick(void);

/** Current tick counter since task_manager_init. Wraps at 65535. */
uint16_t task_manager_ticks(void);

/**
 * Run every task ready now, in priority order, then return
 * (non-blocking). Call repeatedly from your main loop.
 *
 * @return  Number of tasks actually run this round.
 */
uint8_t task_manager_run_once(void);

/** The canonical scheduler loop: pump the harness, run due tasks, refresh
 *  the watchdog. Bounded on host (harness reports pass/fail and returns),
 *  runs forever on target. Call task_manager_run_once directly if you
 *  need per-iteration work of your own. */
void task_manager_run(void);

/** Number of tasks currently registered (used slots), any state. */
uint8_t task_manager_count(void);

/**
 * Wire a HAL Timer0 overflow to task_manager_tick and start it. Call
 * EPIC_IRQ_Restore(1) afterwards to actually arm it.
 *
 * @param reload     TMR0 reload value (0..255); on a 20 MHz target with
 *                   prescaler 1:256, reload 61 -> ~10 ms per tick.
 * @param prescaler  A TIMER0_PrescalerTypeDef (1:2 .. 1:256).
 */
void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler);

#endif /* TASK_MANAGER_H */
