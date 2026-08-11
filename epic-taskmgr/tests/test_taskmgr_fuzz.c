/**
 * Host property test for the cooperative scheduler: random
 * spawn/stop/start/reset/set_period sequences through the real public
 * API, every fire verified against a model of the documented semantics.
 * No timers: the test drives task_manager_tick()/run_once() directly, so
 * the checks are deterministic and exact.
 */

#include "task_manager.h"

#include <stdio.h>
#include <string.h>

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

/* Fixed-seed LCG, deterministic. */
static uint32_t g_seed = 0x7A5E0001u;
static uint32_t rnd(void)
{
    g_seed = (1664525u * g_seed + 1013904223u);
    return g_seed;
}

/* Per-slot model of the documented semantics. */
typedef struct {
    uint8_t  used;
    uint8_t  enabled;
    uint16_t period;
    uint16_t next_fire;       /* tick at which the task goes READY */
    uint16_t fires;           /* model fire count */
} model_task_t;

static model_task_t g_model[TASK_MGR_MAX_TASKS];

/* Per-slot run counters observed through the task args. */
static volatile uint16_t g_runs[TASK_MGR_MAX_TASKS];

static void task_bump(void *arg)
{
    uintptr_t slot = (uintptr_t)arg;
    g_runs[slot]++;
}

/* Spawn with the predicted first-free slot as the task arg, so runs
 * can be attributed per slot; verifies the slot prediction. */
static void do_spawn(uint16_t period, uint8_t priority, uint16_t tick)
{
    uint8_t slot = TASK_MGR_MAX_TASKS;
    for (uint8_t s = 0; s < TASK_MGR_MAX_TASKS; s++) {
        if (!g_model[s].used) {
            slot = s;
            break;
        }
    }
    if (slot == TASK_MGR_MAX_TASKS) {
        /* Table full: spawn must fail. */
        task_id_t id = task_spawn(task_bump, (void *)(uintptr_t)0u, period, priority);
        CHECK(id == TASK_ID_INVALID, "spawn on full table returns INVALID");
        return;
    }

    g_runs[slot] = 0u;   /* fresh task: reset the observable counter */
    task_id_t id = task_spawn(task_bump, (void *)(uintptr_t)slot, period, priority);
    CHECK(id == (task_id_t)slot, "spawn claims the first free slot");
    if (id == TASK_ID_INVALID) {
        return;
    }
    g_model[slot].used      = 1u;
    g_model[slot].enabled   = 1u;
    g_model[slot].period    = period;
    g_model[slot].next_fire = (period == 0u) ? (uint16_t)(tick + 1u)
                                             : (uint16_t)(tick + period);
    g_model[slot].fires     = 0u;
}

int main(void)
{
    task_manager_init();
    memset(g_model, 0, sizeof(g_model));
    memset((void *)g_runs, 0, sizeof(g_runs));

    uint16_t tick = 0u;
    for (int it = 0; it < 5000; it++) {
        switch (rnd() % 7u) {
        case 0: {   /* spawn a periodic task */
            uint16_t p = (uint16_t)(rnd() % 40u) + 1u;
            do_spawn(p, (uint8_t)(rnd() % 4u), tick);
            break;
        }
        case 1:     /* spawn a one-shot */
            do_spawn(0u, (uint8_t)(rnd() % 4u), tick);
            break;
        case 2: {   /* stop a random used task */
            uint8_t s = (uint8_t)(rnd() % TASK_MGR_MAX_TASKS);
            if (g_model[s].used) {
                task_stop((task_id_t)s);
                g_model[s].enabled = 0u;
            }
            break;
        }
        case 3: {   /* start a stopped task */
            uint8_t s = (uint8_t)(rnd() % TASK_MGR_MAX_TASKS);
            if (g_model[s].used && !g_model[s].enabled) {
                task_start((task_id_t)s);
                g_model[s].enabled   = 1u;
                g_model[s].next_fire = (uint16_t)(tick + g_model[s].period);
            }
            break;
        }
        case 4: {   /* reset a random used task */
            uint8_t s = (uint8_t)(rnd() % TASK_MGR_MAX_TASKS);
            if (g_model[s].used) {
                task_reset((task_id_t)s);
                g_model[s].enabled   = 1u;
                g_model[s].next_fire = (uint16_t)(tick + g_model[s].period);
            }
            break;
        }
        case 5: {   /* change a random used periodic task's period */
            uint8_t s = (uint8_t)(rnd() % TASK_MGR_MAX_TASKS);
            if (g_model[s].used && g_model[s].period != 0u) {
                uint16_t p = (uint16_t)(rnd() % 40u) + 1u;
                task_set_period((task_id_t)s, p);
                g_model[s].period = p;   /* takes effect on next arming */
            }
            break;
        }
        default:    /* just tick */
            break;
        }

        /* Advance one tick and run the ready set. */
        task_manager_tick();
        tick++;
        (void)task_manager_run_once();

        /* Fire accounting for every slot. */
        for (uint8_t s = 0; s < TASK_MGR_MAX_TASKS; s++) {
            if (!g_model[s].used) continue;
            if (g_model[s].enabled && g_model[s].next_fire == tick) {
                g_model[s].fires++;
                if (g_model[s].period == 0u) {
                    /* One-shot: freed after its single run. */
                    g_model[s].used = 0u;
                    CHECK(g_runs[s] == 1u, "one-shot fired exactly once");
                } else {
                    g_model[s].next_fire = (uint16_t)(tick + g_model[s].period);
                    CHECK(g_runs[s] == g_model[s].fires, "periodic run count matches");
                }
            }
        }

        /* Invariants. */
        CHECK(task_manager_ticks() == tick, "ticks counter advances exactly once");
        uint8_t used = 0u;
        for (uint8_t s = 0; s < TASK_MGR_MAX_TASKS; s++) {
            if (g_model[s].used) used++;
        }
        CHECK(task_manager_count() == used, "count matches model");

        /* No unexpected fires: every used slot's observable run count
         * must equal the model count (this catches a task firing off
         * its predicted grid, e.g. a one-shot with a wrong countdown). */
        for (uint8_t s = 0; s < TASK_MGR_MAX_TASKS; s++) {
            if (g_model[s].used) {
                if (g_runs[s] != g_model[s].fires) {
                    CHECK(0, "unexpected fire off the predicted grid");
                }
            }
        }
    }

    /* End-of-run: no leaked one-shot slots, tick counter exact. */
    CHECK(task_manager_ticks() == 5000u, "final ticks count");
    printf("test_taskmgr_fuzz: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
