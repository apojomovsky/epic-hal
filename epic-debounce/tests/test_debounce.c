/**
 * Host tests for the debounce engine: scripted bounce sequences through
 * a mock `debounce_read_fn`, with real simulated time advanced 1 ms per
 * poll via epic_tick_init + epic_harness_tick.
 */

#include "debounce.h"
#include "epic_tick.h"
#include "core/epic_harness.h"

#include <stdio.h>

#define FOSC_HZ      20000000UL
#define DEBOUNCE_MS  5u

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

static bool g_script[512];
static int  g_idx;
/** @brief  Mock pin-read: replay the next scripted level. */
static bool mock_read(void *ctx) { (void)ctx; return g_script[g_idx++]; }

/**
 * @brief  Advance simulated time by exactly one millisecond.
 */
static void advance_one_tick(void)
{
    uint32_t t0 = epic_tick_get();
    while (epic_tick_get() == t0) { epic_harness_tick(); }
}
/** @brief  Advance simulated time by `ms` milliseconds. */
static void advance_ms(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) advance_one_tick(); }

/**
 * @brief  Advance one millisecond and poll the debounce instance.
 */
static debounce_event_t step(debounce_t *db)
{
    advance_ms(1);
    return debounce_poll(db);
}

/**
 * @brief  Clean single press and release commits exactly one edge each.
 */
static void test_clean_transition(void)
{
    /* Script: false for calls 0..14, true for 15..29, false for 30+.
     * init reads call 0 (false). Polls read calls 1, 2, 3, ... */
    g_idx = 0;
    for (int i = 0; i < 512; i++) g_script[i] = (i >= 15 && i < 30);

    debounce_t db;
    debounce_init(&db, mock_read, NULL, DEBOUNCE_MS);  /* reads g_script[0]=false */
    CHECK(!debounce_is_active(&db), "clean: init inactive");

    int presses = 0, releases = 0;
    for (int i = 0; i < 40; i++) {
        debounce_event_t ev = step(&db);
        if (ev == DEBOUNCE_EVENT_PRESSED)  presses++;
        if (ev == DEBOUNCE_EVENT_RELEASED) releases++;
    }
    /* The raw goes true at call 15 (poll i=14). Candidate set, timer starts.
     * 5 ms later (poll i=19): PRESSED. Then raw goes false at call 30
     * (poll i=29). 5 ms later (poll i=34): RELEASED. */
    CHECK(presses == 1, "clean: exactly 1 PRESSED");
    CHECK(releases == 1, "clean: exactly 1 RELEASED");
    CHECK(!debounce_is_active(&db), "clean: inactive after release");
}

/**
 * @brief  Bouncy input commits one PRESSED, not one per flip.
 */
static void test_bouncy_transition(void)
{
    /* Script: false for calls 0..9, then bouncy: true,false,true,false,
     * true,false,true,true,true,... (flips at calls 10,11,12,13,14,15,
     * then stable true from 16). Then false from 30. */
    g_idx = 0;
    for (int i = 0; i < 512; i++) {
        if (i < 10) g_script[i] = false;
        else if (i < 16) g_script[i] = (i % 2 == 0);  /* 10=T,11=F,12=T,13=F,14=T,15=F */
        else g_script[i] = true;                       /* stable from 16 onward */
    }
    debounce_t db;
    debounce_init(&db, mock_read, NULL, DEBOUNCE_MS);  /* reads g_script[0]=false */

    int presses = 0;
    for (int i = 0; i < 40; i++) {
        debounce_event_t ev = step(&db);
        if (ev == DEBOUNCE_EVENT_PRESSED) presses++;
    }
    /* The bouncy flips (calls 10..15) keep resetting the candidate timer.
     * From call 16 (poll i=15) the raw is stably true. 5 ms later
     * (poll i=20): PRESSED. Exactly 1, not one per flip. */
    CHECK(presses == 1, "bouncy: exactly 1 PRESSED despite flips");
    CHECK(debounce_is_active(&db), "bouncy: active after settle");
}

/**
 * @brief  A candidate that reverses before the window never commits.
 */
static void test_reversed_before_window(void)
{
    /* Script: false for 0..9, true for 10..12, false for 13+. The true
     * candidate starts at call 10 (poll i=9) but reverses to false at
     * call 13 (poll i=12), only 3 ms later, before the 5 ms window. */
    g_idx = 0;
    for (int i = 0; i < 512; i++) g_script[i] = (i >= 10 && i < 13);

    debounce_t db;
    debounce_init(&db, mock_read, NULL, DEBOUNCE_MS);

    int presses = 0, releases = 0;
    for (int i = 0; i < 30; i++) {
        debounce_event_t ev = step(&db);
        if (ev == DEBOUNCE_EVENT_PRESSED)  presses++;
        if (ev == DEBOUNCE_EVENT_RELEASED) releases++;
    }
    CHECK(presses == 0, "reversed: no PRESSED (true didn't hold)");
    CHECK(releases == 0, "reversed: no RELEASED (false was already stable)");
    CHECK(!debounce_is_active(&db), "reversed: stayed inactive");
}

/**
 * @brief  An input already active at init never fires a spurious PRESSED.
 */
static void test_init_already_active(void)
{
    /* Script: true throughout. Init reads true, sets stable=candidate=true.
     * After the window elapses with no change, no spurious PRESSED. */
    g_idx = 0;
    for (int i = 0; i < 512; i++) g_script[i] = true;

    debounce_t db;
    debounce_init(&db, mock_read, NULL, DEBOUNCE_MS);
    CHECK(debounce_is_active(&db), "init-active: is_active at init");

    int presses = 0;
    for (int i = 0; i < 20; i++) {
        debounce_event_t ev = step(&db);
        if (ev == DEBOUNCE_EVENT_PRESSED) presses++;
    }
    CHECK(presses == 0, "init-active: no spurious PRESSED after window");
    CHECK(debounce_is_active(&db), "init-active: still active");
}

/**
 * @brief  Instances run sequentially do not share state.
 */
static void test_two_independent_instances(void)
{
    /* Run A fully, then B fully with a fresh script, confirming the
     * debounce_t structs are independent (B's result is unaffected by
     * A's prior run). test_concurrent_independence below covers true
     * interleaved polling. */
    g_idx = 0;
    for (int i = 0; i < 512; i++) g_script[i] = (i >= 10 && i < 25);
    debounce_t dbA;
    debounce_init(&dbA, mock_read, NULL, DEBOUNCE_MS);
    int pressA = 0;
    for (int i = 0; i < 35; i++) {
        if (step(&dbA) == DEBOUNCE_EVENT_PRESSED) pressA++;
    }
    CHECK(pressA == 1, "indep: A has 1 press");

    /* Run B: never presses (all false). */
    g_idx = 0;
    for (int i = 0; i < 512; i++) g_script[i] = false;
    debounce_t dbB;
    debounce_init(&dbB, mock_read, NULL, DEBOUNCE_MS);
    int pressB = 0;
    for (int i = 0; i < 35; i++) {
        if (step(&dbB) == DEBOUNCE_EVENT_PRESSED) pressB++;
    }
    CHECK(pressB == 0, "indep: B has 0 presses");
    CHECK(debounce_is_active(&dbA) == false, "indep: A unaffected by B");
    CHECK(debounce_is_active(&dbB) == false, "indep: B stays inactive");
}

static bool g_script2[512];
static int  g_idx2;
/** @brief  Mock pin-read for a second instance: replay g_script2. */
static bool mock_read2(void *ctx) { (void)ctx; return g_script2[g_idx2++]; }

/**
 * @brief  Two instances polled interleaved do not interfere.
 */
static void test_concurrent_independence(void)
{
    /* Two instances polled interleaved, each with its own script + read fn,
     * verifying they don't interfere. */
    g_idx = 0;  g_idx2 = 0;
    for (int i = 0; i < 512; i++) {
        g_script[i]  = (i >= 10 && i < 25);  /* A: press at 10, release at 25 */
        g_script2[i] = (i >= 20);            /* B: press at 20, stays active   */
    }
    debounce_t dbA, dbB;
    debounce_init(&dbA, mock_read,  NULL, DEBOUNCE_MS);
    debounce_init(&dbB, mock_read2, NULL, DEBOUNCE_MS);

    int pA = 0, rA = 0, pB = 0, rB = 0;
    for (int i = 0; i < 35; i++) {
        advance_ms(1);
        debounce_event_t eA = debounce_poll(&dbA);
        debounce_event_t eB = debounce_poll(&dbB);
        if (eA == DEBOUNCE_EVENT_PRESSED)  pA++;
        if (eA == DEBOUNCE_EVENT_RELEASED) rA++;
        if (eB == DEBOUNCE_EVENT_PRESSED)  pB++;
        if (eB == DEBOUNCE_EVENT_RELEASED) rB++;
    }
    CHECK(pA == 1 && rA == 1, "concurrent: A has 1 press + 1 release");
    CHECK(pB == 1 && rB == 0, "concurrent: B has 1 press, 0 releases");
    CHECK(!debounce_is_active(&dbA), "concurrent: A released");
    CHECK(debounce_is_active(&dbB),  "concurrent: B still active");
}

/**
 * @brief  Run all debounce scenarios and report pass/fail counts.
 */
int main(void)
{
    epic_harness_init(2000000UL);
    epic_tick_init(FOSC_HZ);

    test_clean_transition();
    test_bouncy_transition();
    test_reversed_before_window();
    test_init_already_active();
    test_two_independent_instances();
    test_concurrent_independence();

    printf("test_debounce: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
