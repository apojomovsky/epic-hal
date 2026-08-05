/**
 * @file    task_manager.h
 * @brief   Cooperative (non-preemptive) task scheduler for 8-bit PIC
 *          microcontrollers, family-agnostic via `epic_hal.h`.
 *
 * @details
 *   A single periodic tick (typically Timer0, see
 *   @ref task_manager_attach_timer0) marks due tasks ready;
 *   `task_manager_run_once()` runs them to completion in priority order,
 *   no preemption, one shared call stack. See docs/ARCHITECTURE.md for the
 *   host/target execution models and the concurrency/critical-section
 *   design, and examples/example_multi_blink.c for a complete use.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdint.h>
#include "peripherals/hal_timer0.h"   /* Timer0 tick source (family-neutral) */

/* ───────────────────────── configuration ────────────────────────── */

/**
 * @brief Maximum number of simultaneously registered tasks (~12 B/slot).
 *        Scales via @ref PIC8_FAMILY_RAM_BYTES: 6 on 192 B parts, 8
 *        otherwise. Override with `#define TASK_MGR_MAX_TASKS` before
 *        including this header.
 */
#ifndef TASK_MGR_MAX_TASKS
#  if PIC8_FAMILY_RAM_BYTES <= 192
#    define TASK_MGR_MAX_TASKS  6
#  else
#    define TASK_MGR_MAX_TASKS  8
#  endif
#endif

/* ───────────────────────── types ─────────────────────────────────── */

/** Opaque task identifier returned by @ref task_spawn. */
typedef uint8_t task_id_t;

/** Sentinel for "no task" / an invalid spawn. */
#define TASK_ID_INVALID  ((task_id_t)0xFFU)

/**
 * @brief Task entry point: runs to completion and returns, called with the
 *        `arg` passed to @ref task_spawn. Keep it short; nothing else runs
 *        while it does. Persist per-task state through `arg`, not locals.
 */
typedef void (*task_fn_t)(void *arg);

/**
 * @brief Task control block, one of @ref TASK_MGR_MAX_TASKS fixed slots;
 *        all fields internal, users address tasks by @ref task_id_t.
 */
typedef struct {
    task_fn_t   fn;          /**< Entry point (NULL in a free slot). */
    void       *arg;         /**< Opaque argument passed to `fn`. */
    uint16_t    period;      /**< Period in ticks; 0 = one-shot. */
    uint16_t    countdown;   /**< Ticks until the next ready. */
    uint8_t     priority;    /**< Lower number = runs first within a round. */
    uint8_t     flags;       /**< Packed @ref task_flags. */
} task_t;

/** @name Task state flags (packed into @ref task_t.flags). @{ */
#define TM_FLAG_USED     0x01U   /**< Slot is allocated. */
#define TM_FLAG_ENABLED  0x02U   /**< Slot is scheduled. */
#define TM_FLAG_READY    0x04U   /**< Due this round. */
/** @} */

/* ───────────────────────── lifecycle ─────────────────────────────── */

/**
 * @brief  Initialise the scheduler. Clears every task slot and zeroes the
 *         tick counter. Call once before spawning tasks or attaching a
 *         tick source. Idempotent.
 */
void task_manager_init(void);

/**
 * @brief  Register a task and arm it.
 *
 * @param  fn            Entry point (must not be NULL).
 * @param  arg           Opaque pointer passed back to `fn` (may be NULL).
 * @param  period_ticks   Call interval in ticks; 0 = one-shot (fires once,
 *                        then frees its slot).
 * @param  priority      Scheduling priority within a round; lower runs
 *                       first, ties break by spawn order.
 *
 * @return The new task id, or @ref TASK_ID_INVALID if `fn` is NULL or all
 *         slots are in use.
 *
 * @note   Safe to call from within a running task (e.g. a supervisor
 *         spawning one-shot children); the slot fill is a short critical
 *         section, so it does not race with the tick ISR.
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
 * @brief  Re-arm a task: restart its countdown from the full period and
 *         ensure it is enabled, without changing the period or freeing
 *         the slot (the debounce / feed-the-watchdog verb). Safe to call
 *         from a running task.
 */
void task_reset(task_id_t id);

/* ───────────────────────── the scheduler ─────────────────────────── */

/**
 * @brief  Advance the scheduler by one tick: decrement each enabled task's
 *         countdown, marking it ready (and reloading periodic ones) at
 *         zero. Call from a timer ISR, typically Timer0 via
 *         @ref task_manager_attach_timer0; short and never runs user code,
 *         so it is safe in interrupt context.
 */
void task_manager_tick(void);

/** Current tick counter since @ref task_manager_init. Wraps at 65535. */
uint16_t task_manager_ticks(void);

/**
 * @brief  Run every task ready now, in priority order, then return
 *         (non-blocking). Call repeatedly from your main loop.
 *
 * @return Number of tasks actually run this round.
 */
uint8_t task_manager_run_once(void);

/**
 * @brief  The canonical scheduler loop. Equivalent to:
 *
 *             for (;;) {
 *                 epic_harness_tick();   // pump sim / no-op on target
 *                 task_manager_run_once();
 *                 EPIC_WDT_Refresh();
 *             }
 *
 *         Bounded on host (harness reports pass/fail and returns), runs
 *         forever on target. Call @ref task_manager_run_once directly
 *         instead if you need per-iteration work of your own.
 */
void task_manager_run(void);

/** Number of tasks currently registered (used slots), any state. */
uint8_t task_manager_count(void);

/* ───────────────────────── optional tick source ──────────────────── */

/**
 * @brief  Wire a HAL Timer0 overflow to @ref task_manager_tick and start
 *         it (internal Fosc/4, TMR0 interrupt enable set). Call
 *         `EPIC_IRQ_Restore(1)` afterwards to actually arm it.
 *
 * @param  reload      TMR0 reload value (0..255). On a 20 MHz target,
 *                     prescaler 1:256, reload 61 -> ~10 ms per tick.
 * @param  prescaler   A @ref TIMER0_PrescalerTypeDef (1:2 .. 1:256).
 */
void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler);

#endif /* TASK_MANAGER_H */
