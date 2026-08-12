/**
 * Unit tests for epic-fsm; exercises the exact fsm.c that ships on-target
 * (no per-family variant, no test-framework dependency).
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

/** @brief Action: count a transition into YELLOW (caution). */
static void on_caution(void *ctx)
{
    light_ctx_t *c = ctx;
    c->caution_calls++;
}

/** @brief Action: count a transition into FAULT. */
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

/** @brief RED->GREEN->YELLOW->RED cycle with the action firing once. */
static void test_basic_cycle(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);

    CHECK(epic_fsm_state(&fsm) == ST_RED, "initial state is RED");

    CHECK(epic_fsm_dispatch(&fsm, EV_TIMER) == true, "RED+TIMER fires");
    CHECK(epic_fsm_state(&fsm) == ST_GREEN, "RED+TIMER -> GREEN");

    CHECK(epic_fsm_dispatch(&fsm, EV_TIMER) == true, "GREEN+TIMER fires");
    CHECK(epic_fsm_state(&fsm) == ST_YELLOW, "GREEN+TIMER -> YELLOW");
    CHECK(ctx.caution_calls == 1, "on_caution ran exactly once");

    CHECK(epic_fsm_dispatch(&fsm, EV_TIMER) == true, "YELLOW+TIMER fires");
    CHECK(epic_fsm_state(&fsm) == ST_RED, "YELLOW+TIMER -> RED (full cycle)");
}

/** @brief FSM_ANY_STATE row matches from any current state. */
static void test_any_state_wildcard(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;

    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);
    CHECK(epic_fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from RED");
    CHECK(epic_fsm_state(&fsm) == ST_FAULT, "RED+FAULT -> FAULT");
    CHECK(ctx.fault_calls == 1, "on_fault ran");

    FSM_INIT(&fsm, light_transitions, ST_GREEN, &ctx);
    CHECK(epic_fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from GREEN");
    CHECK(epic_fsm_state(&fsm) == ST_FAULT, "GREEN+FAULT -> FAULT");

    FSM_INIT(&fsm, light_transitions, ST_YELLOW, &ctx);
    CHECK(epic_fsm_dispatch(&fsm, EV_FAULT) == true, "FAULT fires from YELLOW");
    CHECK(epic_fsm_state(&fsm) == ST_FAULT, "YELLOW+FAULT -> FAULT");
}

/** @brief An event with no matching row reports false and leaves state. */
static void test_unhandled_event(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_FAULT, &ctx);

    /* No row matches (ST_FAULT, EV_TIMER): dispatch must report false and
     * leave state untouched. */
    CHECK(epic_fsm_dispatch(&fsm, EV_TIMER) == false, "unhandled event returns false");
    CHECK(epic_fsm_state(&fsm) == ST_FAULT, "unhandled event leaves state unchanged");
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

/** @brief Guard: credit must cover the fare. */
static bool has_sufficient_credit(void *ctx)
{
    turnstile_ctx_t *c = ctx;
    return c->credit_cents >= TURNSTILE_FARE_CENTS;
}

/** @brief Action: deduct the fare and count the unlock. */
static void do_unlock(void *ctx)
{
    turnstile_ctx_t *c = ctx;
    c->credit_cents -= TURNSTILE_FARE_CENTS;
    c->unlock_calls++;
}

/** @brief Action: count a rejected (buzzed) coin. */
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

/** @brief A rejected guard falls through to the next matching row. */
static void test_guard_fallthrough(void)
{
    turnstile_ctx_t ctx = { 0, 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, turnstile_transitions, ST_LOCKED, &ctx);

    /* Insufficient credit: first row's guard rejects, falls through to the
     * unconditional second row. */
    CHECK(epic_fsm_dispatch(&fsm, EV_COIN) == true, "COIN with no credit still dispatches (buzz row)");
    CHECK(epic_fsm_state(&fsm) == ST_LOCKED, "stays LOCKED with insufficient credit");
    CHECK(ctx.buzz_calls == 1, "buzz ran");
    CHECK(ctx.unlock_calls == 0, "unlock did not run");

    /* Enough credit: first row's guard passes, fires, second row never
     * evaluated. */
    ctx.credit_cents = TURNSTILE_FARE_CENTS;
    CHECK(epic_fsm_dispatch(&fsm, EV_COIN) == true, "COIN with sufficient credit dispatches (unlock row)");
    CHECK(epic_fsm_state(&fsm) == ST_UNLOCKED, "moves to UNLOCKED");
    CHECK(ctx.unlock_calls == 1, "unlock ran");
    CHECK(ctx.buzz_calls == 1, "buzz did not run again");
    CHECK(ctx.credit_cents == 0, "fare deducted");

    CHECK(epic_fsm_dispatch(&fsm, EV_PUSH) == true, "PUSH fires from UNLOCKED");
    CHECK(epic_fsm_state(&fsm) == ST_LOCKED, "PUSH -> LOCKED");
}

/** @brief Multiple instances sharing one table must never interfere. */

static void test_independent_instances(void)
{
    light_ctx_t ctx_a = { 0, 0 };
    light_ctx_t ctx_b = { 0, 0 };
    fsm_t fsm_a, fsm_b;

    FSM_INIT(&fsm_a, light_transitions, ST_RED, &ctx_a);
    FSM_INIT(&fsm_b, light_transitions, ST_RED, &ctx_b);

    epic_fsm_dispatch(&fsm_a, EV_TIMER);
    CHECK(epic_fsm_state(&fsm_a) == ST_GREEN, "instance A advanced");
    CHECK(epic_fsm_state(&fsm_b) == ST_RED, "instance B untouched by A's dispatch");

    epic_fsm_dispatch(&fsm_b, EV_FAULT);
    CHECK(epic_fsm_state(&fsm_b) == ST_FAULT, "instance B faulted");
    CHECK(epic_fsm_state(&fsm_a) == ST_GREEN, "instance A untouched by B's dispatch");
}

/** @brief FSM_INIT's sizeof/sizeof table_len computation. */

static void test_fsm_init_table_len(void)
{
    light_ctx_t ctx = { 0, 0 };
    fsm_t fsm;
    FSM_INIT(&fsm, light_transitions, ST_RED, &ctx);

    CHECK(fsm.table_len == (sizeof(light_transitions) / sizeof(light_transitions[0])),
          "FSM_INIT computed table_len matches the array's real row count");
}

/** @brief Run every epic-fsm unit test and report the failure count. */
int main(void)
{
    test_basic_cycle();
    test_any_state_wildcard();
    test_unhandled_event();
    test_guard_fallthrough();
    test_independent_instances();
    test_fsm_init_table_len();

    if (g_failures == 0) {
        printf("PASS: all epic-fsm unit tests\n");
        return 0;
    }
    printf("FAIL: %d assertion(s) failed\n", g_failures);
    return 1;
}
