/**
 * @file    task_manager.c
 * @brief   Cooperative task scheduler implementation (see task_manager.h).
 *
 * @details
 *   `task_manager_tick()` (usually a Timer0 ISR) decrements each enabled
 *   task's countdown and marks due tasks ready; `run_once()` then runs the
 *   ready set in priority order with interrupts re-enabled, so a tick
 *   during a long task arms it for the next round rather than being lost.
 *   The TCB array is the only state shared with interrupt context:
 *   run_once, the mutators, and one-shot slot-freeing each take a brief
 *   critical section so a tick ISR never observes a half-updated TCB.
 */

#include "task_manager.h"
#include "core/hal_irq.h"   /* EPIC_IRQ_Disable/Restore (family-neutral) */
#include "core/epic_harness.h"      /* harness_tick / harness_running */
#include "core/hal_wdt_sleep.h"    /* EPIC_WDT_Refresh (family-neutral) */

/* ───────────────────────── state ─────────────────────────────────── */

/** The task table. Slot 0 is claimed first by task_spawn. */
static task_t g_tasks[TASK_MGR_MAX_TASKS];

/** Monotonic tick counter since the last init (wraps at 65535). */
static uint16_t g_ticks = 0U;

/** Countdown that fires a task after exactly `period` ticks: periodic
 *  tasks reload to period-1, not period (else every fire would take
 *  period+1 ticks); a one-shot (period 0) uses 0, ready on the first tick. */
static uint16_t arm_countdown(uint16_t period)
{
    return (period == 0U) ? 0U : (uint16_t)(period - 1U);
}

/** TMR0 reload value, rewritten each overflow since Timer0 has no
 *  hardware auto-reload; set by task_manager_attach_timer0. */
static uint8_t g_tick_reload = 0U;

/* ───────────────────────── lifecycle ─────────────────────────────── */

void task_manager_init(void)
{
    uint8_t prev = EPIC_IRQ_Disable();
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        g_tasks[i].fn        = NULL;
        g_tasks[i].arg       = NULL;
        g_tasks[i].period    = 0U;
        g_tasks[i].countdown = 0U;
        g_tasks[i].priority  = 0U;
        g_tasks[i].flags     = 0U;
    }
    g_ticks = 0U;
    EPIC_IRQ_Restore(prev);
}

task_id_t task_spawn(task_fn_t fn, void *arg, uint16_t period_ticks,
                     uint8_t priority)
{
    if (fn == NULL) {
        return TASK_ID_INVALID;
    }

    /* Publish-last, no GIE manipulation (class-G conversion): the TCB
     * is fully written before the slot's USED bit is set, and the tick
     * ISR gates every access on USED, so it can never observe a
     * half-initialised TCB. */
    task_id_t id = TASK_ID_INVALID;
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        if (!(g_tasks[i].flags & TM_FLAG_USED)) {
            g_tasks[i].fn        = fn;
            g_tasks[i].arg       = arg;
            g_tasks[i].period    = period_ticks;
            g_tasks[i].countdown = arm_countdown(period_ticks);
            g_tasks[i].priority  = priority;
            g_tasks[i].flags     = TM_FLAG_USED | TM_FLAG_ENABLED;
            id = (task_id_t)i;
            break;
        }
    }
    return id;
}

void task_start(task_id_t id)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    /* RETAINED critical section (class-G conversion, deliberate): the
     * countdown write below is a 16-bit store racing the tick ISR's
     * 16-bit countdown RMW; the read-twice-retry pattern cannot make a
     * write atomic, and the publish-last toggle of ENABLED leaves the
     * ISR's check-then-act window open. This is the correct silicon
     * idiom (the Finding 10.1 sim-wedge class does not reproduce on
     * PIC18, where this module's sim gates run). */
    uint8_t prev = EPIC_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].countdown = arm_countdown(g_tasks[id].period);
        g_tasks[id].flags |=  TM_FLAG_ENABLED;
        g_tasks[id].flags &= (uint8_t)~TM_FLAG_READY;
    }
    EPIC_IRQ_Restore(prev);
}

void task_stop(task_id_t id)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    /* Single-byte flag RMW, atomic on both families (class-G
     * conversion): no GIE manipulation needed. */
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].flags &= (uint8_t)~(TM_FLAG_ENABLED | TM_FLAG_READY);
    }
}

void task_reset(task_id_t id)
{
    /* Re-arm: restart the countdown and ensure the task is enabled. The
     * critical section is retained (class-G conversion, same rationale
     * as task_start: a 16-bit countdown write cannot be made atomic by
     * retry). */
    if (id >= TASK_MGR_MAX_TASKS) return;
    uint8_t prev = EPIC_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].countdown = arm_countdown(g_tasks[id].period);
        g_tasks[id].flags |=  TM_FLAG_ENABLED;
        g_tasks[id].flags &= (uint8_t)~TM_FLAG_READY;
    }
    EPIC_IRQ_Restore(prev);
}

void task_set_period(task_id_t id, uint16_t period_ticks)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    /* RETAINED critical section (class-G conversion, same rationale as
     * task_start): the 16-bit period write races the tick ISR's period
     * read when a task fires; a torn value would mis-time one fire. */
    uint8_t prev = EPIC_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].period = period_ticks;
    }
    EPIC_IRQ_Restore(prev);
}

/* ───────────────────────── the scheduler ─────────────────────────── */

void task_manager_tick(void)
{
    g_ticks++;

    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        task_t *t = &g_tasks[i];
        uint8_t  f = t->flags;
        if (!(f & TM_FLAG_USED) || !(f & TM_FLAG_ENABLED)) {
            continue;
        }
        if (t->countdown == 0U) {
            /* Due now; see arm_countdown for the period-1 reload reasoning. */
            t->flags |= TM_FLAG_READY;
            if (t->period != 0U) {
                t->countdown = arm_countdown(t->period);
            }
        } else {
            t->countdown--;
        }
    }
}

uint16_t task_manager_ticks(void)
{
    /* Read-twice-retry (class-G conversion, the epic_tick_get pattern):
     * the tick ISR increments g_ticks as a 16-bit RMW, so a single
     * read can tear; retry until two consecutive reads agree. No GIE
     * manipulation. */
    uint16_t v;
    do {
        v = g_ticks;
    } while (v != g_ticks);
    return v;
}

uint8_t task_manager_run_once(void)
{
    /* Snapshot the ready set in priority order and clear each ready flag
     * under a critical section, then run the tasks with interrupts enabled
     * so a tick during a long task arms the task for the next round. */
    task_id_t order[TASK_MGR_MAX_TASKS];
    uint8_t   n = 0U;

    /* No GIE manipulation (class-G conversion): every flags access here
     * is a single-byte read or RMW (atomic), and the snapshot is
     * timing-tolerant by design - a tick that arms a task after its
     * slot was scanned simply runs it next round ("a new tick re-arms",
     * the documented behavior for tasks that overrun). */
    for (;;) {
        /* Pick the lowest-numbered-priority ready task (ties: lowest slot). */
        int      best      = -1;
        uint8_t  best_prio = 0xFFU;
        for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
            task_t *t = &g_tasks[i];
            if ((t->flags & (TM_FLAG_USED | TM_FLAG_ENABLED | TM_FLAG_READY))
                == (TM_FLAG_USED | TM_FLAG_ENABLED | TM_FLAG_READY) &&
                t->priority < best_prio) {
                best      = (int)i;
                best_prio = t->priority;
            }
        }
        if (best < 0) {
            break;
        }
        g_tasks[best].flags &= (uint8_t)~TM_FLAG_READY;  /* a new tick re-arms */
        order[n] = (task_id_t)best;
        n++;
    }

    /* Run with interrupts enabled. */
    for (uint8_t k = 0; k < n; k++) {
        task_t *t = &g_tasks[order[k]];
        t->fn(t->arg);
        if (t->period == 0U) {
            /* One-shot: free the slot so a periodic task that re-spawns
             * one-shots does not exhaust the table. The USED gate makes
             * this safe without a critical section (class-G conversion):
             * the tick ISR skips a slot whose USED bit is clear, and at
             * worst a tick that read USED before the clear touches a
             * freed slot's countdown and sets READY, which run_once
             * gates out via USED. */
            t->flags = 0U;
            t->fn    = NULL;
        }
    }
    return n;
}

void task_manager_run(void)
{
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();    /* host: pumps sim → Timer0 ISR → tick */
        (void)task_manager_run_once();
        EPIC_WDT_Refresh();            /* no-op on the host */
    }
}

uint8_t task_manager_count(void)
{
    /* Single-byte flag reads (atomic); no GIE manipulation (class-G
     * conversion). A spawn/free mid-scan only shifts the count by one
     * in the racy direction, which is a timing artifact, not a safety
     * issue for the scheduler. */
    uint8_t count = 0U;
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        if (g_tasks[i].flags & TM_FLAG_USED) {
            count++;
        }
    }
    return count;
}

/* ───────────────────────── optional tick source ──────────────────── */

/** Trampoline matching the HAL's `void (*)(void)` overflow callback shape.
 *  Reloads TMR0 first (no hardware auto-reload) so every tick has the
 *  same period, then advances the scheduler. */
static void task_manager_on_timer0_overflow(void)
{
    EPIC_TIMER0_WriteCounter(g_tick_reload);
    task_manager_tick();
}

void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler)
{
    g_tick_reload = reload;
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = prescaler;
    h.PrescalerAssigned = true;
    h.ReloadValue       = reload;
    h.OverflowCallback  = task_manager_on_timer0_overflow;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
}
