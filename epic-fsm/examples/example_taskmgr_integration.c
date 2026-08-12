/**
 * epic-fsm composed with epic-taskmgr: a task callback owns an fsm_t and
 * dispatches into it, no special integration needed. Built only with
 * -DEPIC_FSM_BUILD_TASKMGR_EXAMPLE=ON.
 */

#include <stddef.h>
#include "fsm.h"
#include "task_manager.h"
#include "core/epic_harness.h"

enum { ST_IDLE, ST_ARMED };
enum { EV_PRESS, EV_TIMEOUT };

typedef struct {
    fsm_t    fsm;
    uint8_t  arm_count;
} button_t;

/** @brief Button action: count and log the arming. */
static void on_arm(void *ctx)
{
    button_t *b = ctx;
    b->arm_count++;
    epic_harness_log("  armed (count=%u)\n", (unsigned)b->arm_count);
}

static const fsm_transition_t button_transitions[] = {
    { ST_IDLE,  EV_PRESS,   NULL, on_arm, ST_ARMED },
    { ST_ARMED, EV_TIMEOUT, NULL, NULL,   ST_IDLE  },
};

static button_t g_button;

/**
 * @brief Task callback that dispatches button events into the FSM.
 *
 * The task callback knows nothing of fsm.h's internals beyond calling
 * epic_fsm_dispatch: that is the entire integration surface.
 */
static void button_task(void *arg)
{
    button_t *b = arg;
    static uint16_t tick = 0;

    tick++;
    if (tick == 2U) {
        epic_fsm_dispatch(&b->fsm, EV_PRESS);
    } else if (tick == 5U) {
        epic_fsm_dispatch(&b->fsm, EV_TIMEOUT);
    }
}

/** @brief Compose an fsm_t with epic-taskmgr and drive it via the harness. */
int main(void)
{
    FSM_INIT(&g_button.fsm, button_transitions, ST_IDLE, &g_button);
    g_button.arm_count = 0;

    task_manager_init();
    task_spawn(button_task, &g_button, 1U, 0U);

    epic_harness_init(10U);
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
        task_manager_tick();      /* drive the scheduler's own tick counter directly;
                                    * a real target would wire this to a timer ISR via
                                    * task_manager_attach_timer0() instead. */
        task_manager_run_once();
    }

    epic_harness_log("final state: %s, arm_count=%u\n",
                     epic_fsm_state(&g_button.fsm) == ST_IDLE ? "IDLE" : "ARMED",
                     (unsigned)g_button.arm_count);
    return epic_harness_report(epic_fsm_state(&g_button.fsm) == ST_IDLE &&
                                g_button.arm_count == 1U);
}
