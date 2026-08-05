/**
 * @file    example_multi_blink.c
 * @brief   Four independent LED blinks at distinct rates driven by the
 *          cooperative task manager, one source for the host simulator
 *          and a real XC8 target, with no `#ifdef` in the code.
 *
 * @details
 *   Demonstrates periodic tasks at different periods, priority ordering
 *   (the supervisor runs first each round), and runtime one-shot spawning
 *   (the supervisor spawns "blip" children that free their slot after one
 *   run). PORTB is used so the example builds unchanged for every device
 *   in the family, including the 28-pin parts with no PORTD/PORTE. Wiring:
 *   an LED and resistor on each of RB0..RB3 to GND, 20 MHz HS crystal.
 */

#include "pic8_hal.h"          /* family-neutral HAL entry point         */
#include "core/pic8_harness.h"

#include "task_manager.h"

/* ───────────────────────── timing ────────────────────────────────── */

/** Timer0 reload for a ~10 ms tick on a 20 MHz target: Fosc/4=5 MHz,
 *  prescaler 1:256 (51.2 us/count), reload 61 -> 195 counts ~= 9.98 ms. */
#define TICK_RELOAD       61U
#define TICK_PRESCALER    TIMER0_PRESCALER_1_256

/** Host run length: long enough for the period-40 supervisor to fire, its
 *  one-shot blip to land, and the period-20 slow blink to toggle a few times.
 *  Override with -DSIM_CYCLES=. */
#ifndef SIM_CYCLES
#define SIM_CYCLES        4000000UL
#endif

/* Task periods in ticks (~10 ms each). Chosen for clearly distinct visible
 * rates on hardware and comfortable margins on the sim (counts need only be
 * monotonic: fast > med > slow >= 2, plus >= 1 blip). */
#define PERIOD_FAST        5U    /* ~50 ms  → RB0 (fastest) */
#define PERIOD_MED        10U    /* ~100 ms → RB1 */
#define PERIOD_SLOW       20U    /* ~200 ms → RB2 */
#define PERIOD_SUPERVISOR 40U    /* ~400 ms → spawns a blip on RB3 */

/* ───────────────────────── per-task state ─────────────────────────── */

/** Per-LED state carried through each blink task's @ref task_spawn `arg`,
 *  since locals don't survive between calls. Pointer-free to fit the
 *  192 B 28-pin parts. */
typedef struct {
    GPIO_TypeDef      port;   /* Which port the LED lives on. */
    uint8_t           pin;    /* Bit index 0..7 of the LED pin. */
    volatile uint8_t  count;  /* Toggle count (host assertion; uint8 is plenty). */
} blink_arg_t;

static blink_arg_t arg_fast = { GPIOB, 0U, 0U };   /* RB0 */
static blink_arg_t arg_med  = { GPIOB, 1U, 0U };   /* RB1 */
static blink_arg_t arg_slow = { GPIOB, 2U, 0U };   /* RB2 */
static blink_arg_t arg_blip = { GPIOB, 3U, 0U };   /* RB3 (spawned at runtime) */

/* ───────────────────────── tasks ──────────────────────────────────── */

/** Map a LED's pin index to a short label for the log, padded to 4 chars
 *  so the columns line up. */
static const char *led_name(uint8_t pin)
{
    switch (pin) {
        case 0: return "fast";
        case 1: return "med ";
        case 2: return "slow";
        case 3: return "blip";
        default: return "?   ";
    }
}

/** Periodic blink task: toggle the LED, bump its count, log a line. The
 *  same function serves all four LEDs, each with its own arg. */
static void task_blink(void *arg)
{
    blink_arg_t *a = (blink_arg_t *)arg;
    EPIC_GPIO_TogglePin(a->port, PIC8_BIT(a->pin));
    a->count++;
    pic8_harness_log("[t=%3u] %s  #%u\n",
                           (unsigned)task_manager_ticks(),
                           led_name(a->pin), (unsigned)a->count);
}

/** Periodic supervisor (priority 0, runs first each round): every
 *  PERIOD_SUPERVISOR ticks, spawns a fresh one-shot blip on RB3, which
 *  fires once and frees its own slot. */
static void task_supervisor(void *arg)
{
    (void)arg;
    task_spawn(task_blink, &arg_blip, 0U, 2U);   /* one-shot (period 0) */
    pic8_harness_log("[t=%3u] super  spawned blip\n",
                           (unsigned)task_manager_ticks());
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);
    task_manager_init();

    /* 1. RB0..RB3 as outputs, all starting low. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                  GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB,
                      GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                      GPIO_PIN_RESET);

    /* 2. Priority 0 = supervisor runs first; the three blinks share
     *    priority 1 and run in spawn order. */
    task_spawn(task_supervisor, NULL, PERIOD_SUPERVISOR, 0U);
    task_spawn(task_blink, &arg_fast, PERIOD_FAST, 1U);
    task_spawn(task_blink, &arg_med,  PERIOD_MED,  1U);
    task_spawn(task_blink, &arg_slow, PERIOD_SLOW, 1U);

    /* 3. Wire the ~10 ms Timer0 tick to the scheduler. This sets TMR0IE;
     *    arm it on the target by enabling global interrupts (harmless on
     *    the sim, where the IRQ fires regardless). */
    task_manager_attach_timer0(TICK_RELOAD, TICK_PRESCALER);
    EPIC_IRQ_Restore(1);

    /* 4. Run the scheduler. On the host the harness bounds the loop to
     *    SIM_CYCLES; on the target it runs forever. */
    task_manager_run();

    /* 5. Host-only epilogue: the verdict after the dispatch stream. On the
     *    target these lines are unreachable (the loop never returns). */
    pic8_harness_log("done: fast=%u med=%u slow=%u blips=%u "
                           "(ticks=%u, tasks=%u)\n",
                           (unsigned)arg_fast.count, (unsigned)arg_med.count,
                           (unsigned)arg_slow.count, (unsigned)arg_blip.count,
                           (unsigned)task_manager_ticks(),
                           (unsigned)task_manager_count());

    /* Pass when the four tasks ran at four distinct rates and the
     * supervisor spawned at least one blip. */
    int ok = (arg_fast.count > arg_med.count) &&
             (arg_med.count  > arg_slow.count) &&
             (arg_slow.count >= 2U) &&
             (arg_blip.count >= 1U);
    return pic8_harness_report(ok);
}
