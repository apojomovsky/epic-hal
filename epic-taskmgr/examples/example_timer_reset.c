/**
 * Verify task_reset re-arms a task from the full period. A period-10
 * marker records the scheduler tick at each fire; a period-25 supervisor
 * calls task_reset(marker) on its first fire. Marker fires at 10, 20,
 * then 35 (reset_tick + period), not the originally scheduled 30. Host
 * sim only; the XC8 target build uses example_multi_blink.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "epic_taskmgr.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

/* Tick config matched to example_multi_blink (reload 61, 1:256); fire
 * ticks are deterministic in scheduler-tick units regardless of
 * cycles-per-tick, so the tick-number assertions hold on both families. */
#define TICK_RELOAD     61U
#define TICK_PRESCALER  TIMER0_PRESCALER_1_256
#define SIM_CYCLES      4000000UL

#define MARKER_PERIOD   10U
#define SUPERVISOR_PERIOD 25U
#define MAX_FIRES       32U

static volatile uint16_t fire_ticks[MAX_FIRES];
static volatile uint8_t  n_fires = 0U;
static volatile uint8_t  did_reset = 0U;
static volatile uint16_t reset_tick = 0U;
static task_id_t g_marker_id = TASK_ID_INVALID;

/** @brief Periodic marker: record the scheduler tick of each fire. */
static void task_marker(void *arg)
{
    (void)arg;
    if (n_fires < MAX_FIRES) {
        fire_ticks[n_fires] = epic_taskmgr_ticks();
        n_fires++;
    }
}

/** @brief Supervisor: on its first fire, reset the marker task. */
static void task_supervisor(void *arg)
{
    (void)arg;
    if (!did_reset) {
        reset_tick = epic_taskmgr_ticks();
        task_reset(g_marker_id);
        did_reset = 1U;
        epic_harness_log("[t=%u] supervisor reset marker\n", (unsigned)reset_tick);
    }
}

/** @brief Verify task_reset re-arms from the full period via tick assertions. */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    epic_taskmgr_init();

    g_marker_id = epic_taskmgr_spawn(task_marker, NULL, MARKER_PERIOD, 1U);
    (void)epic_taskmgr_spawn(task_supervisor, NULL, SUPERVISOR_PERIOD, 0U);

    epic_taskmgr_attach_timer0(TICK_RELOAD, TICK_PRESCALER);
    EPIC_IRQ_Restore(1);

    epic_taskmgr_run();

    /* 1. Enough fires to cover both pre-reset fires (10, 20), the first
     *    post-reset fire (35), and one more confirming resumed spacing (45). */
    CHECK(n_fires >= 4U, "marker did not fire enough times");
    CHECK(fire_ticks[0] == 10U, "first fire not at tick 10");
    CHECK(fire_ticks[1] == 20U, "second fire not at tick 20");
    CHECK((uint16_t)(fire_ticks[1] - fire_ticks[0]) == MARKER_PERIOD,
          "pre-reset gap != period");

    /* 2. The supervisor reset at tick 25. */
    CHECK(did_reset == 1U, "supervisor never reset the marker");
    CHECK(reset_tick == 25U, "reset did not land at tick 25");

    /* 3. First fire after reset is exactly period ticks later (35, not 30),
     *    confirming the reset re-armed from the full period. */
    uint8_t j = 0U;
    while (j < n_fires && fire_ticks[j] <= reset_tick) j++;
    CHECK(j < n_fires, "no fire after the reset");
    CHECK((uint16_t)(fire_ticks[j] - reset_tick) == MARKER_PERIOD,
          "first post-reset fire not at reset_tick + period (reset did not re-arm)");
    CHECK((uint16_t)(fire_ticks[j] - fire_ticks[j - 1U]) > MARKER_PERIOD,
          "gap across reset did not widen");
    /* 4. Spacing resumes at the period after the reset. */
    CHECK((j + 1U) < n_fires, "no second post-reset fire to confirm spacing");
    CHECK((uint16_t)(fire_ticks[j + 1U] - fire_ticks[j]) == MARKER_PERIOD,
          "post-reset spacing did not resume at the period");

    epic_harness_log("OK: task_reset re-arms from the full period "
                     "(fires 10,20 | reset@25 | next@35,45).\n");
    return epic_harness_report(1);
}
