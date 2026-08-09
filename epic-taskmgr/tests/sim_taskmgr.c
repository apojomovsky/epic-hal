/**
 * @file    sim_taskmgr.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-taskmgr's
 *          first-ever real `mdb` gate. Spawns two cooperative periodic
 *          tasks with known periods plus a one-shot, runs the canonical
 *          scheduler loop for a bounded iteration count, and verifies
 *          (a) both periodic tasks executed, (b) the 16-bit tick
 *          counter advanced, (c) the one-shot fired exactly once and
 *          freed its slot, and (d) task_set_period took effect.
 *          Reports PASS/FAIL over the target's real hardware EUSART via
 *          epic_harness_report, the same way every other family's own
 *          `.sim` variant does (see
 *          pic18fxx5x-hal/src/core/pic18_harness_sim_target.c).
 *
 * @details
 *   Follows the bounded host/target-agnostic contract
 *   `core/epic_harness.h` defines: on the sim target the Timer0
 *   overflow ISR (wired by task_manager_attach_timer0) drives
 *   `task_manager_tick()` in real simulated time, `task_manager_run()`
 *   terminates when `epic_harness_running()` runs out of iterations,
 *   and `epic_harness_report` emits the marker line the gate greps for.
 *
 *   Family choice: PIC18Fxx5x (18F4550). The module is pure logic, and
 *   MPLAB SIM's PIC16F87XA leg has a hard layout constraint: the
 *   interrupt-handler body must stay in flash page 0 (the vector's
 *   PCLATH-less goto), and the Timer0-tick ISR path proved unusable
 *   under SIM for this module (measured while tuning this gate: a
 *   512-cycle tick made the ISR longer than the tick period, so TMR0IF
 *   was pending at every retfie and the ISR re-entered continuously,
 *   starving the main loop; longer ticks then exposed a GIE bit-set on
 *   INTCON that never persisted under the xc8 v4.00 PIC16 codegen).
 *   PIC18 uses absolute calls and a different INTCON model, sidestepping
 *   both, and the epic-tick PIC18 sim gate already proves PIC18 Timer
 *   interrupts work under SIM.
 *
 *   Tick rate: 1:16 prescaler, reload 0 -> 256 counts x 16 = 4096
 *   instruction cycles per tick (~341 us at the 48 MHz / 12 MHz
 *   instruction rate this family's sim harness builds with). That
 *   accumulates ~50-100 ticks per run, far past the 25 ticks the
 *   checks below need, and leaves the ISR a small fraction of the tick
 *   period so the scheduler loop makes progress between interrupts.
 *   The scheduler is tick-rate agnostic: periods are counted in ticks.
 *
 *   The one-shot task is spawned at init (period 0, countdown 0, so it
 *   is due on the first tick), and its single run calls
 *   `task_set_period` on task C, lengthening C's period far beyond the
 *   run. C fires once more (the current countdown was armed at spawn)
 *   and then stops, so `runs_c == 1` proves the runtime period change
 *   took effect: without it C would fire every 5 ticks for the whole
 *   run. The scheduler frees the one-shot's slot after its single run,
 *   so `task_manager_count() == 3` at the end is the exact-once check.
 */

#include "task_manager.h"
#include "core/epic_harness.h"
#include "core/hal_irq.h"      /* EPIC_IRQ_Restore: enable GIE for TMR0 */

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  1500 iterations is a few hundred thousand instructions, which
 *  finishes well inside a 5000 ms mdb wait budget while accumulating
 *  ~50-100 ticks (see the tick-rate note above), far past the 25 ticks
 *  the checks below need. */
#define SIM_ITERATIONS 1500UL

/** Task periods in ticks: distinct rates so the two periodic tasks'
 *  counts prove repeated, independent firing. With the ~341 us tick,
 *  A = ~3.4 ms, B = ~6.8 ms, C = ~1.7 ms of simulated time. */
#define PERIOD_A 10U
#define PERIOD_B 20U
#define PERIOD_C  5U

/** Period the one-shot applies to task C: far beyond the ~100 ticks a
 *  run produces, so C's countdown never reaches 0 again. */
#define PERIOD_FROZEN 0xFFF0U

/** Timer0 tick for the diagnostic run: 1:16 prescaler, reload 0 ->
 *  256 counts x 16 = 4096 instruction cycles per tick (~341 us at
 *  48 MHz). See the tick-rate note above. */
#define TICK_RELOAD    0U
#define TICK_PRESCALER TIMER0_PRESCALER_1_16

/* ───────────────────────── per-task state ─────────────────────────── */

/** Counter carried through a periodic task's `arg`, since locals don't
 *  survive between runs. */
typedef struct {
    volatile uint16_t runs;
} run_count_t;

/** One-shot task's arg: its own run count plus the task id it freezes. */
typedef struct {
    volatile uint16_t runs;
    task_id_t         freeze_id;
} oneshot_arg_t;

static run_count_t   arg_a   = { 0U };
static run_count_t   arg_b   = { 0U };
static run_count_t   arg_c   = { 0U };
static oneshot_arg_t arg_one = { 0U, TASK_ID_INVALID };

/* ───────────────────────── tasks ──────────────────────────────────── */

/** Periodic counter task: bump the count and return. */
static void task_count(void *arg)
{
    run_count_t *r = (run_count_t *)arg;
    r->runs++;
}

/** One-shot: runs exactly once (the scheduler frees its slot right
 *  after), bumps its own count, and exercises task_set_period by
 *  freezing task C for the rest of the run. */
static void task_oneshot(void *arg)
{
    oneshot_arg_t *o = (oneshot_arg_t *)arg;
    o->runs++;
    task_set_period(o->freeze_id, PERIOD_FROZEN);
}

/* ───────────────────────── main ───────────────────────────────────── */

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);
    task_manager_init();

    /* Two periodic tasks with known periods, plus a one-shot (period 0)
     * whose single run freezes task C via task_set_period. Priorities:
     * A first, B and C share the middle, one-shot last. */
    task_spawn(task_count, &arg_a, PERIOD_A, 0U);
    task_spawn(task_count, &arg_b, PERIOD_B, 1U);
    task_id_t id_c = task_spawn(task_count, &arg_c, PERIOD_C, 1U);
    arg_one.freeze_id = id_c;
    task_spawn(task_oneshot, &arg_one, 0U, 2U);

    /* Wire the diagnostic Timer0 tick to the scheduler and enable
     * global interrupts (on the sim, the ISR fires regardless; the
     * GIE enable matters only on real hardware). */
    task_manager_attach_timer0(TICK_RELOAD, TICK_PRESCALER);
    EPIC_IRQ_Restore(1);

    /* The canonical scheduler loop. On the sim target the harness
     * terminates it after SIM_ITERATIONS; on real hardware it runs
     * forever (unreachable epilogue below). */
    task_manager_run();

    /* Verdict. A (period 10) fires at ticks 10, 20, 30, ...; B (period
     * 20) at 20, 40, ...; C fires once at tick 5 (countdown armed at
     * spawn) then freezes on the one-shot's task_set_period; the
     * one-shot fires on the first tick and frees its slot. */
    uint16_t ticks = task_manager_ticks();
    uint8_t  used  = task_manager_count();

    int ok = (arg_a.runs >= 2U) &&      /* (a) A executed (twice: re-armed) */
             (arg_b.runs >= 1U) &&      /* (a) B executed                  */
             (arg_c.runs == 1U) &&      /* set_period froze C              */
             (arg_one.runs == 1U) &&    /* (c) one-shot fired exactly once */
             (ticks >= 25U) &&          /* (b) 16-bit counter advanced    */
             (used == 3U);              /* one-shot freed its slot        */
    return epic_harness_report(ok);
}
