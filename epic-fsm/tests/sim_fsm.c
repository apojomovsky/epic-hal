/**
 * @file    sim_fsm.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-fsm's
 *          first-ever real `mdb` gate. Drives a small 3-state machine
 *          (IDLE -> RUN -> DONE) through a scripted event sequence via
 *          the real API, records the observed state after every
 *          dispatch, and verifies the recorded sequence matches the
 *          expected one exactly, including rejected events (a guard
 *          block and several invalid-event rejections). The entry/exit
 *          transition callbacks count their invocations in volatile
 *          counters, which the final check also compares. Reports
 *          PASS/FAIL over the target's real hardware USART the same way
 *          every other family's own `.sim` variant does (see
 *          pic16f87xa-hal/src/core/pic16_harness_sim_target.c).
 *
 * @details
 *   Distinct from `tests/test_fsm.c` (the host unit tests, which run on
 *   the build machine) and `mcu/target_sizecheck.c` (the real-target
 *   footprint build, an unbounded loop with no
 *   `core/epic_harness.h` dependency). This file follows the bounded
 *   host/target-agnostic contract `core/epic_harness.h` defines
 *   instead, the same pattern `epic-swuart/tests/sim_target_swuart.c`
 *   and `epic-tick/examples/example_tick.c` use for their own `.sim`
 *   entries. Pure computation only: no hardware waits, no MPLAB SIM RX
 *   injection needed (the gate exercises the transition engine, guard
 *   fall-through, and invalid-event rejection, all TX-of-record over
 *   the harness USART).
 */
#include "fsm.h"
#include "core/epic_harness.h"

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  Pure computation, no hardware waits; far more than enough. */
#define SIM_ITERATIONS 100000UL

/* The small scripted machine: IDLE -> RUN -> DONE -> IDLE. */
enum { ST_IDLE, ST_RUN, ST_DONE };

enum { EV_START, EV_STOP, EV_RESET, EV_NONE };

#define SEQ_MAX 9u   /* 1 post-init state + 8 dispatched steps */

typedef struct {
    uint8_t          allow_start;    /* guard flag: EV_START fires only when set */
    volatile uint8_t enter_run;      /* transitions INTO RUN */
    volatile uint8_t exit_run;       /* transitions OUT of RUN */
    volatile uint8_t enter_done;     /* transitions INTO DONE */
    volatile uint8_t exit_done;      /* transitions OUT of DONE */
    uint8_t          seq[SEQ_MAX];   /* recorded state after each step */
    uint8_t          seq_len;
} fsm_ctx_t;

static bool guard_can_start(void *ctx)
{
    return ((fsm_ctx_t *)ctx)->allow_start != 0u;
}

static void act_start(void *ctx)
{
    ((fsm_ctx_t *)ctx)->enter_run++;
}

static void act_stop(void *ctx)
{
    fsm_ctx_t *c = (fsm_ctx_t *)ctx;
    c->exit_run++;
    c->enter_done++;
}

static void act_reset(void *ctx)
{
    ((fsm_ctx_t *)ctx)->exit_done++;
}

static const fsm_transition_t transitions[] = {
    { ST_IDLE, EV_START, guard_can_start, act_start, ST_RUN  },
    { ST_RUN,  EV_STOP,  NULL,            act_stop,  ST_DONE },
    { ST_DONE, EV_RESET, NULL,            act_reset, ST_IDLE },
};

/* One scripted step: set the guard flag, dispatch, record the state,
 * return whether a row fired. */
static uint8_t step(fsm_t *fsm, fsm_ctx_t *ctx, fsm_event_t ev,
                    uint8_t allow_start)
{
    uint8_t fired;

    ctx->allow_start = allow_start;
    fired = fsm_dispatch(fsm, ev) ? 1u : 0u;
    ctx->seq[ctx->seq_len++] = fsm_state(fsm);
    return fired;
}

int main(void)
{
    fsm_ctx_t ctx = { 0u, 0u, 0u, 0u, 0u, {0u}, 0u };
    fsm_t fsm;
    uint8_t fired[8];
    uint8_t i;
    uint32_t iter;
    int ok;

    /* expect_state[0] is the post-init state; expect_fired[0] is a
     * placeholder for that same step, fired[0..7] map to steps 1..8. */
    static const uint8_t expect_state[SEQ_MAX] = {
        ST_IDLE, ST_IDLE, ST_IDLE, ST_IDLE, ST_RUN,
        ST_RUN,  ST_DONE, ST_DONE, ST_IDLE,
    };
    static const uint8_t expect_fired[SEQ_MAX] = { 0u, 0u, 0u, 0u, 1u,
                                                   0u, 1u, 0u, 1u };

    epic_harness_init(SIM_ITERATIONS);

    FSM_INIT(&fsm, transitions, ST_IDLE, &ctx);
    ctx.seq[ctx.seq_len++] = fsm_state(&fsm);

    /* 1: EV_STOP in IDLE: no row, rejected. */
    fired[0] = step(&fsm, &ctx, EV_STOP, 0u);
    /* 2: EV_START in IDLE, guard blocks: rejected, stays IDLE. */
    fired[1] = step(&fsm, &ctx, EV_START, 0u);
    /* 3: EV_NONE: no row anywhere, rejected. */
    fired[2] = step(&fsm, &ctx, EV_NONE, 0u);
    /* 4: EV_START in IDLE, guard passes: IDLE -> RUN. */
    fired[3] = step(&fsm, &ctx, EV_START, 1u);
    /* 5: EV_RESET in RUN: no row, rejected. */
    fired[4] = step(&fsm, &ctx, EV_RESET, 1u);
    /* 6: EV_STOP in RUN: RUN -> DONE. */
    fired[5] = step(&fsm, &ctx, EV_STOP, 1u);
    /* 7: EV_START in DONE: no row, rejected. */
    fired[6] = step(&fsm, &ctx, EV_START, 1u);
    /* 8: EV_RESET in DONE: DONE -> IDLE. */
    fired[7] = step(&fsm, &ctx, EV_RESET, 1u);

    for (iter = 0; epic_harness_running(iter); iter++) {
        epic_harness_tick();
    }

    ok = 1;
    for (i = 0; i < ctx.seq_len; i++) {
        if (ctx.seq[i] != expect_state[i]) {
            ok = 0;
        }
    }
    for (i = 0; i < 8u; i++) {
        if (fired[i] != expect_fired[i + 1u]) {
            ok = 0;
        }
    }
    /* The guard-rejected EV_START (step 2) must not have run its
     * action: enter_run must be exactly 1 (step 4 only). */
    if (ctx.enter_run != 1u || ctx.exit_run != 1u ||
        ctx.enter_done != 1u || ctx.exit_done != 1u) {
        ok = 0;
    }

    epic_harness_log(ok ? "fsm sim: IDLE->RUN->DONE sequence ok\n"
                        : "fsm sim: sequence mismatch\n");
    return epic_harness_report(ok);
}
