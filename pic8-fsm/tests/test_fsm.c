/**
 * @file    test_fsm.c
 * @brief   Unit tests for pic8-fsm; exercises the exact fsm.c that ships
 *          on-target (no per-family variant, no test-framework dependency).
 */

#include <stdio.h>
#include <string.h>
#include "fsm.h"

static int g_failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", (msg), __LINE__); \
        g_failures++; \
    } \
} while (0)

/* A minimal traffic light: RED -> GREEN -> YELLOW -> RED on EV_TIMER,
 * plus a global EV_FAULT -> FAULT transition from any state. */

enum { ST_RED, ST_GREEN, ST_YELLOW, ST_FAULT };
enum { EV_TIMER, EV_FAULT };

typedef struct {
    int caution_calls;
    int fault_calls;
} light_ctx_t;

static void on_caution(void *ctx)
{
    light_ctx_t *c = ctx;
    c->caution_calls++;
}

static void on_fault(void *ctx)
{
    light_ctx_t *c = ctx;
    c->fault_calls++;
}

static const fsm_transition_t light_transitions[] = {
    { ST_RED,        EV_TIMER, NULL, NULL,       ST_GREEN  },
    { ST_GREEN,      EV_TIMER, NULL, on_caution, ST_YELLOW },
    { ST_YELLOW,     EV_TIMER, NULL, NULL,       ST_RED    },
    { FSM_ANY_STATE, EV_FAULT, NULL, on_fault,   ST_FAULT  },
};

static void test_basic_cycle(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);

    CHECK(fsm_state(&fsm) == ST_RED, "initial state is RED");

    CHECK(fsm_dispatch(&fsm, EV_TIMER) == true, "RED+TIMER fires");
    CHECK(fsm_state(&fsm) == ST_GREEN, "RED+TIMER -> GREEN");

    CHECK(fsm_dispatch(&fsm, EV_TIMER) == true, "GREEN+TIMER fires");
    CHECK(fsm_state(&fsm) == ST_YELLOW, "GREEN+TIMER -> YELLOW");
    CHECK(ctx.caution_calls == 1, "on_caution ran exactly once");

    CHECK(fsm_dispatch(&fsm, EV_TIMER) == true, "YELLOW+TIMER fires");
    CHECK(fsm_state(&fsm) == ST_RED, "YELLOW+TIMER -> RED (full cycle)");
}

static void test_any_state_wildcard(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;

    /* Fault fires from every state, including one never reached by TIMER
     * alone (drive to GREEN first, then fault from there). */
    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);
    CHECK(fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from RED");
    CHECK(fsm_state(&fsm) == ST_FAULT, "RED+FAULT -> FAULT");
    CHECK(ctx.fault_calls == 1, "on_fault ran");

    FSM_INIT(&fsm, light_transitions, ST_GREEN, &ctx);
    CHECK(fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from GREEN");
    CHECK(fsm_state(&fsm) == ST_FAULT, "GREEN+FAULT -> FAULT");

    FSM_INIT(&fsm, light_transitions, ST_YELLOW, &ctx);
    CHECK(fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from YELLOW");
    CHECK(fsm_state(&fsm) == ST_FAULT, "YELLOW+FAULT -> FAULT");
}

static void test_unhandled_event(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_FAULT, &ctx);

    /* No row matches (ST_FAULT, EV_TIMER): dispatch must report false and
     * leave state untouched. */
    CHECK(fsm_dispatch(&fsm, EV_TIMER) == false, "unhandled event returns false");
    CHECK(fsm_state(&fsm) == ST_FAULT, "unhandled event leaves state unchanged");
}

/* Guard fall-through: a turnstile where COIN unlocks only with enough
 * credit, otherwise buzzes and stays locked. */

enum { ST_LOCKED, ST_UNLOCKED };
enum { EV_COIN, EV_PUSH };

typedef struct {
    int credit_cents;
    int unlock_calls;
    int buzz_calls;
} turnstile_ctx_t;

#define TURNSTILE_FARE_CENTS 25

static bool has_sufficient_credit(void *ctx)
{
    turnstile_ctx_t *c = ctx;
    return c->credit_cents >= TURNSTILE_FARE_CENTS;
}

static void do_unlock(void *ctx)
{
    turnstile_ctx_t *c = ctx;
    c->credit_cents -= TURNSTILE_FARE_CENTS;
    c->unlock_calls++;
}

static void buzz_rejected(void *ctx)
{
    turnstile_ctx_t *c = ctx;
    c->buzz_calls++;
}

static const fsm_transition_t turnstile_transitions[] = {
    { ST_LOCKED,   EV_COIN, has_sufficient_credit, do_unlock,     ST_UNLOCKED },
    { ST_LOCKED,   EV_COIN, NULL,                  buzz_rejected, ST_LOCKED   },
    { ST_UNLOCKED, EV_PUSH, NULL,                  NULL,          ST_LOCKED   },
};

static void test_guard_fallthrough(void)
{
    turnstile_ctx_t ctx = { 0, 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, turnstile_transitions, ST_LOCKED, &ctx);

    /* Insufficient credit: first row's guard rejects, falls through to the
     * unconditional second row. */
    CHECK(fsm_dispatch(&fsm, EV_COIN) == true, "COIN with no credit still dispatches (buzz row)");
    CHECK(fsm_state(&fsm) == ST_LOCKED, "stays LOCKED with insufficient credit");
    CHECK(ctx.buzz_calls == 1, "buzz ran");
    CHECK(ctx.unlock_calls == 0, "unlock did not run");

    /* Enough credit: first row's guard passes, fires, second row never
     * evaluated. */
    ctx.credit_cents = TURNSTILE_FARE_CENTS;
    CHECK(fsm_dispatch(&fsm, EV_COIN) == true, "COIN with sufficient credit dispatches (unlock row)");
    CHECK(fsm_state(&fsm) == ST_UNLOCKED, "moves to UNLOCKED");
    CHECK(ctx.unlock_calls == 1, "unlock ran");
    CHECK(ctx.buzz_calls == 1, "buzz did not run again");
    CHECK(ctx.credit_cents == 0, "fare deducted");

    CHECK(fsm_dispatch(&fsm, EV_PUSH) == true, "PUSH fires from UNLOCKED");
    CHECK(fsm_state(&fsm) == ST_LOCKED, "PUSH -> LOCKED");
}

/* ------------------------------------------------------------------ */
/* Multiple instances sharing one table must never interfere.          */
/* ------------------------------------------------------------------ */

static void test_independent_instances(void)
{
    light_ctx_t ctx_a = { 0, 0 };
    light_ctx_t ctx_b = { 0, 0 };
    fsm_t fsm_a, fsm_b;

    FSM_INIT(&fsm_a, light_transitions, ST_RED, &ctx_a);
    FSM_INIT(&fsm_b, light_transitions, ST_RED, &ctx_b);

    fsm_dispatch(&fsm_a, EV_TIMER);
    CHECK(fsm_state(&fsm_a) == ST_GREEN, "instance A advanced");
    CHECK(fsm_state(&fsm_b) == ST_RED, "instance B untouched by A's dispatch");

    fsm_dispatch(&fsm_b, EV_FAULT);
    CHECK(fsm_state(&fsm_b) == ST_FAULT, "instance B faulted");
    CHECK(fsm_state(&fsm_a) == ST_GREEN, "instance A untouched by B's dispatch");
}

/* ------------------------------------------------------------------ */
/* FSM_INIT's sizeof/sizeof table_len computation.                     */
/* ------------------------------------------------------------------ */

static void test_fsm_init_table_len(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);

    CHECK(fsm.table_len == (sizeof(light_transitions) / sizeof(light_transitions[0])),
          "FSM_INIT computed table_len matches the array's real row count");
}

int main(void)
{
    test_basic_cycle();
    test_any_state_wildcard();
    test_unhandled_event();
    test_guard_fallthrough();
    test_independent_instances();
    test_fsm_init_table_len();

    if (g_failures == 0) {
        printf("PASS: all pic8-fsm unit tests\n");
        return 0;
    }
    printf("FAIL: %d assertion(s) failed\n", g_failures);
    return 1;
}
