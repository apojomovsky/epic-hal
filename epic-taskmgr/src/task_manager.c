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

    /* Claim and fill a free slot under a critical section so a tick ISR
     * can never observe a half-initialised TCB. */
    uint8_t prev = EPIC_IRQ_Disable();
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
    EPIC_IRQ_Restore(prev);
    return id;
}

void task_start(task_id_t id)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
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
    uint8_t prev = EPIC_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].flags &= (uint8_t)~(TM_FLAG_ENABLED | TM_FLAG_READY);
    }
    EPIC_IRQ_Restore(prev);
}

void task_reset(task_id_t id)
{
    /* Re-arm: restart the countdown and ensure the task is enabled; same
     * critical section as the other mutators, safe from a running task. */
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
    /* 16-bit read is not atomic on an 8-bit PIC; take a critical section so
     * a tick ISR landing between the byte reads can't tear the value. */
    uint8_t  prev = EPIC_IRQ_Disable();
    uint16_t v    = g_ticks;
    EPIC_IRQ_Restore(prev);
    return v;
}

uint8_t task_manager_run_once(void)
{
    /* Snapshot the ready set in priority order and clear each ready flag
     * under a critical section, then run the tasks with interrupts enabled
     * so a tick during a long task arms the task for the next round. */
    task_id_t order[TASK_MGR_MAX_TASKS];
    uint8_t   n = 0U;

    uint8_t prev = EPIC_IRQ_Disable();
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
    EPIC_IRQ_Restore(prev);

    /* Run with interrupts enabled. */
    for (uint8_t k = 0; k < n; k++) {
        task_t *t = &g_tasks[order[k]];
        t->fn(t->arg);
        if (t->period == 0U) {
            /* One-shot: free the slot atomically so a periodic task that
             * re-spawns one-shots does not exhaust the table. */
            uint8_t p = EPIC_IRQ_Disable();
            t->flags = 0U;
            t->fn    = NULL;
            EPIC_IRQ_Restore(p);
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
    uint8_t count = 0U;
    uint8_t prev  = EPIC_IRQ_Disable();
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        if (g_tasks[i].flags & TM_FLAG_USED) {
            count++;
        }
    }
    EPIC_IRQ_Restore(prev);
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
