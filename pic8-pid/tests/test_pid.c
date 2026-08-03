/**
 * @file    test_pid.c
 * @brief   Host tests for the fixed-point PID controller.
 *
 * @details
 *   Tests the exact pid.c that ships in the PIC16/PIC18 cross-compiles;
 *   no HAL, timebase, or per-family split involved. Expected values are
 *   computed independently in the test (plain `(int32_t)a * b`), not via
 *   `pic_math_mul_s16`, so a pid.c bug is what these tests catch, not a
 *   pic_math one.
 */

#include "pid.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } \
                          else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* ---- helpers: Q8.8 multiply by hand (independent of pic_math) ---- */
static int32_t mul_s16(int16_t a, int16_t b)
{
    /* Independent oracle: the test must not call pic_math_mul_s16 here,
     * since the bug we're trying to catch could be in pid.c's wiring
     * of that call. Compute the product in the wider host type. */
    return (int32_t)a * (int32_t)b;
}

/* ---- helper: Q8.8 multiply for a Q8.8-scale int16_t gain against
 *      a raw int16_t measurement/error. Returns the int32_t Q8.8 result,
 *      identical to what pid.c computes for P and D terms. */
static int32_t p_or_d_term(int16_t gain_q8, int16_t raw)
{
    return mul_s16(gain_q8, raw);
}

/* ================================================================ */
/* 1. Pure P: ki_q8 = kd_q8 = 0. A constant error should produce
 *      output == clamp(p_mul_s16(kp_q8, error), out_min, out_max). */
static void test_pure_p(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)256, (int16_t)0, (int16_t)0, (int16_t)-1000, (int16_t)1000);

    /* kp_q8 = 256 means Kp = 1.0 in fixed point. error=100 -> P_q8 = 25600. */
    int16_t error = 100;
    int16_t out = pid_update(&pid, error, (int16_t)0);
    int32_t p_q8 = p_or_d_term(256, error);            /* 25600 */
    int16_t exp  = (int16_t)(p_q8 >> 8);              /* 100 */
    CHECK(out == exp, "pure P: error=100 -> output 100");

    /* error = -200 -> P_q8 = -51200, output = clamp(-200, ...) = -200. */
    out = pid_update(&pid, (int16_t)(-200), (int16_t)0);
    p_q8 = p_or_d_term(256, (int16_t)(-200));          /* -51200 */
    exp  = (int16_t)(p_q8 / 256);                      /* -200, exact multiple, no rounding */
    CHECK(out == exp, "pure P: error=-200 -> output -200");

    /* Now choose a gain * error that would exceed the clamp. kp=256,
     * error=500 -> P_q8 = 128000, (128000>>8)=500, but clamp is 1000,
     * so this lands exactly at 500, no clamp. */
    out = pid_update(&pid, (int16_t)500, (int16_t)0);
    CHECK(out == 500, "pure P: no clamp yet, 500 in range");

    /* Push past the clamp: error=2000 -> P_q8 = 512000, >>8 = 2000, but
     * clamp[max]=1000. */
    out = pid_update(&pid, (int16_t)2000, (int16_t)0);
    CHECK(out == 1000, "pure P: clamped to out_max on huge P");
}

/* ================================================================ */
/* 2. Pure I: kp_q8 = kd_q8 = 0. Repeated calls with a constant
 *      sub-saturating error accumulate integrator_q8 linearly. */
static void test_pure_i_accumulates(void)
{
    pid_t pid;
    /* ki_q8 = 256 means Ki*Ts = 1.0 in fixed point. Sub-saturating
     * error = 10, clamp wide so no anti-windup. */
    pid_init(&pid, (int16_t)0, (int16_t)256, (int16_t)0,
             (int16_t)-10000, (int16_t)10000);

    int32_t expected_integrator = 0;
    for (int n = 0; n < 4; n++) {
        (void)pid_update(&pid, (int16_t)10, (int16_t)0);
        expected_integrator += mul_s16(256, 10);  /* 2560 per step */
        CHECK(pid.integrator_q8 == expected_integrator,
              "pure I: integrator tracks running sum");
    }
    /* Step 5 (after 4 tracked steps): sum_q8 = 0 + 12800 + 0 = 12800,
     * output = 50. */
    int16_t out = pid_update(&pid, (int16_t)10, (int16_t)0);
    CHECK(pid.integrator_q8 == 12800, "pure I: integrator at 12800 after 5 calls");
    CHECK(out == 50, "pure I: 5th call -> output 50");
    /* Step 6 increments to 15360, output = 60. */
    out = pid_update(&pid, (int16_t)10, (int16_t)0);
    CHECK(pid.integrator_q8 == 15360, "pure I: integrator at 15360 after 6 calls");
    CHECK(out == 60, "pure I: 6th call -> output 60");
}

/* ================================================================ */
/* 3. Anti-windup: tight clamp + sustained large error. The integrator
 *      must never exceed the Q8.8 clamp rails, and the output must
 *      saturate at the rails exactly, never beyond. */
static void test_anti_windup_clamps(void)
{
    pid_t pid;
    /* kp=0, ki=256 (1.0/step), kd=0. Tight clamp [-50, 50] so the
     * Q8.8 rails are [-12800, 12800]. Sustained error=10 normally
     * accumulates to (10 * 256) = 2560 per step -> saturates in
     * about 5 steps. */
    pid_init(&pid, (int16_t)0, (int16_t)256, (int16_t)0,
             (int16_t)-50, (int16_t)50);

    int32_t out_max_q8 = 50 * 256;  /* 12800 */
    int32_t out_min_q8 = -50 * 256; /* -12800 */

    /* Drive until saturation: 5 steps of error=+10 (2560/step) takes the
     * integrator from 0 to 12800, exactly the rail. */
    int16_t last_out = 0;
    for (int n = 0; n < 5; n++) {
        last_out = pid_update(&pid, (int16_t)10, (int16_t)0);
    }
    CHECK(last_out == 50, "anti-windup: just reached the rail");
    CHECK(pid.integrator_q8 == out_max_q8,
          "anti-windup: integrator exactly at out_max_q8");

    /* Continue for another 20 steps: every step's increment would
     * overshoot, so the integrator must stay clamped at out_max_q8 and
     * the output must stay at out_max exactly. */
    for (int n = 0; n < 20; n++) {
        last_out = pid_update(&pid, (int16_t)10, (int16_t)0);
        CHECK(pid.integrator_q8 <= out_max_q8,
              "anti-windup: integrator never above out_max_q8");
        CHECK(pid.integrator_q8 >= out_min_q8,
              "anti-windup: integrator never below out_min_q8");
        CHECK(last_out == 50, "anti-windup: output saturates at out_max exactly");
    }
    CHECK(last_out == 50, "anti-windup: still saturated after 20 more steps");
    CHECK(pid.integrator_q8 == out_max_q8,
          "anti-windup: integrator still equals the rail exactly");
}

/* ================================================================ */
/* 4. Windup recovery: continuing #3, flip the sign of the error. The
 *      integrator was capped rather than allowed to overshoot, so the
 *      very next call should move the output off the saturation rail. */
static void test_windup_recovery_immediate(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)0, (int16_t)256, (int16_t)0,
             (int16_t)-50, (int16_t)50);

    /* Drive to saturation: 20 steps of error=+10. */
    for (int n = 0; n < 20; n++) {
        (void)pid_update(&pid, (int16_t)10, (int16_t)0);
    }
    int32_t out_max_q8 = 50 * 256;
    CHECK(pid.integrator_q8 == out_max_q8, "recovery: pre-flip saturated");
    CHECK(pid_update(&pid, (int16_t)10, (int16_t)0) == 50,
          "recovery: still saturated at flip point");

    /* Flip the error: now measurement is far above setpoint (error=-10).
     * The integrator must immediately start decreasing. After one step:
     * integrator_q8 = 12800 - 2560 = 10240; output = 10240 >> 8 = 40.
     * Critically, this happens on the VERY FIRST flipped call, not after
     * unwinding through 5 calls worth of overshoot. */
    int16_t out = pid_update(&pid, (int16_t)0, (int16_t)10);
    CHECK(pid.integrator_q8 == out_max_q8 - 2560,
          "recovery: integrator decreased on first flipped call");
    CHECK(out == 40, "recovery: output off the rail on the first flipped call");
}

/* ================================================================ */
/* 5. No derivative kick on the first call. With kp=ki=0, kd=256,
 *      a setpoint step from 0 to 1000 right after init must produce
 *      exactly 0 output (D=0 because no prev measurement, P=0 and
 *      I=0 because kp=ki=0). */
static void test_no_derivative_kick_first_call(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)0, (int16_t)0, (int16_t)256,
             (int16_t)-1000, (int16_t)1000);

    /* First call: setpoint=1000, measurement=0 -> error=1000, but
     * kp=ki=0, so P=I=0, and the D term is also 0 because no
     * previous measurement. */
    int16_t out = pid_update(&pid, (int16_t)1000, (int16_t)0);
    CHECK(out == 0, "no-deriv-kick: first call returns 0 with all-zero gains except kd");

    /* Second call with a moving measurement must now produce a real
     * D term. Same setpoint, measurement changes to 100: dmeas = 100. */
    out = pid_update(&pid, (int16_t)1000, (int16_t)100);
    int32_t d_q8 = -mul_s16(256, (int16_t)100);  /* derivative-on-measurement, negated */
    int16_t exp  = (int16_t)(d_q8 / 256);        /* -100, exact multiple, no rounding */
    CHECK(out == exp, "no-deriv-kick: D term appears on second call only");
}

/* Same property after pid_reset: a reset must zero the D-term
 * history so the very next call behaves like the first call. */
static void test_no_derivative_kick_after_reset(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)0, (int16_t)0, (int16_t)256,
             (int16_t)-1000, (int16_t)1000);

    /* Build up D history over a few calls. */
    (void)pid_update(&pid, (int16_t)0, (int16_t)0);
    (void)pid_update(&pid, (int16_t)0, (int16_t)50);
    (void)pid_update(&pid, (int16_t)0, (int16_t)100);
    /* Now have_prev_measurement = true and prev = 100. */

    pid_reset(&pid);

    /* After reset, the next call should produce D=0 (no
     * have_prev_measurement). All gains zero here, so total = 0. */
    int16_t out = pid_update(&pid, (int16_t)1000, (int16_t)0);
    CHECK(out == 0, "no-deriv-kick-after-reset: D zeroed on first call after reset");
    /* gains and clamp were not reset: kd=256 should still apply on
     * the second call. measurement 0 -> 10 with dmeas=10 -> D_q8 =
     * -2560, output = -10. */
    out = pid_update(&pid, (int16_t)1000, (int16_t)10);
    CHECK(out == -10, "no-deriv-kick-after-reset: gains still apply on second call");
}

/* ================================================================ */
/* 6. Derivative sign. With kp=ki=0, kd=256 (D-on-measurement,
 *      negated): a measurement that rises between two calls must
 *      produce a NEGATIVE output (damping); a falling measurement a
 *      POSITIVE output. Assert the sign, not just nonzero. */
static void test_derivative_sign(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)0, (int16_t)0, (int16_t)256,
             (int16_t)-1000, (int16_t)1000);

    /* Build D history: first call with measurement=0, then 50. */
    (void)pid_update(&pid, (int16_t)0, (int16_t)0);
    /* Now prev_measurement=0. */

    /* Rising: measurement 0 -> 50, dmeas=+50, D_q8=-256*50=-12800,
     * output=-50 (damping). */
    int16_t out = pid_update(&pid, (int16_t)0, (int16_t)50);
    CHECK(out == -50, "derivative sign: rising measurement -> negative (damping)");

    /* Falling: measurement 50 -> 20, dmeas=-30, D_q8=-256*(-30)=+7680,
     * output=+30 (anti-damping / push back). */
    out = pid_update(&pid, (int16_t)0, (int16_t)20);
    CHECK(out == 30, "derivative sign: falling measurement -> positive");
}

/* ================================================================ */
/* 7. Final output always clamps. Extreme P, I, or D values chosen so
 *      sum_q8 >> 8 would land outside [out_min, out_max]. Assert
 *      the return is clamped regardless of which term caused the
 *      excess. */
static void test_final_output_clamps_p(void)
{
    pid_t pid;
    /* kp huge, ki=kd=0, tight clamp. */
    pid_init(&pid, (int16_t)32000, (int16_t)0, (int16_t)0,
             (int16_t)-100, (int16_t)100);
    int16_t out = pid_update(&pid, (int16_t)200, (int16_t)0);
    /* P_q8 = 32000*200 = 6_400_000, >>8 = 25000, clamp to 100. */
    CHECK(out == 100, "clamp: P pushes past out_max, clamped to out_max");
    out = pid_update(&pid, (int16_t)(-200), (int16_t)0);
    /* P_q8 = 32000*(-200) = -6_400_000, >>8 = -25000, clamp to -100. */
    CHECK(out == -100, "clamp: P pushes past out_min, clamped to out_min");
}

static void test_final_output_clamps_d(void)
{
    pid_t pid;
    /* kd huge, kp=ki=0. */
    pid_init(&pid, (int16_t)0, (int16_t)0, (int16_t)32000,
             (int16_t)-100, (int16_t)100);
    (void)pid_update(&pid, (int16_t)0, (int16_t)0);  /* build D history */
    /* dmeas=200 -> D_q8 = -32000*200 = -6_400_000, >>8 = -25000, clamp -100. */
    int16_t out = pid_update(&pid, (int16_t)0, (int16_t)200);
    CHECK(out == -100, "clamp: D pushes past out_min, clamped to out_min");
}

static void test_final_output_clamps_i(void)
{
    pid_t pid;
    /* I-only, gain huge. Clamp is symmetric, so we get pinned to
     * out_max, then check the rail is respected even with
     * accumulated integrator overflow potential. */
    pid_init(&pid, (int16_t)0, (int16_t)32000, (int16_t)0,
             (int16_t)-100, (int16_t)100);
    for (int n = 0; n < 50; n++) {
        int16_t out = pid_update(&pid, (int16_t)10, (int16_t)0);
        CHECK(out >= -100 && out <= 100,
              "clamp: I extreme, every step's output is in range");
    }
}

/* ================================================================ */
/* 8. Bumpless transfer. Run several AUTO cycles, flip to MANUAL
 *      with a new output target, then back to AUTO. The output
 *      should not jump at the AUTO->MANUAL->AUTO transitions. */
static void test_bumpless_transfer(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)256, (int16_t)10, (int16_t)0,
             (int16_t)-1000, (int16_t)1000);

    /* Drive in AUTO for a few steps so the integrator is
     * non-trivial. setpoint=100, measurement=0 -> steady-state
     * P = 256*100 = 25600 -> 100, plus a small I. */
    int16_t last_auto_out = 0;
    for (int n = 0; n < 10; n++) {
        last_auto_out = pid_update(&pid, (int16_t)100, (int16_t)0);
    }
    /* P=100 exactly, I contributing a small amount, D=0. Output is
     * near 100. */

    /* Switch to MANUAL, set manual_output=300. First MANUAL call
     * returns 300 (clamped). */
    pid_set_mode(&pid, PID_MODE_MANUAL);
    pid_set_manual_output(&pid, (int16_t)300);
    int16_t manual_out = pid_update(&pid, (int16_t)100, (int16_t)0);
    CHECK(manual_out == 300, "bumpless: MANUAL output is the requested target");

    /* Now switch back to AUTO, same setpoint/measurement. The
     * integrator was back-calculated during MANUAL so this call's
     * AUTO output should equal the MANUAL output (continuous, no
     * jump) before the loop is free to evolve. */
    pid_set_mode(&pid, PID_MODE_AUTO);
    int16_t first_auto_after = pid_update(&pid, (int16_t)100, (int16_t)0);
    CHECK(first_auto_after == manual_out,
          "bumpless: first AUTO call after MANUAL returns the same value (no jump)");

    /* A second AUTO call must move off manual_out as the integrator
     * accumulates; exact value not pinned, just that it changed. */
    int16_t second_auto_after = pid_update(&pid, (int16_t)100, (int16_t)0);
    CHECK(second_auto_after != manual_out,
          "bumpless: integrator resumes evolving after AUTO resumes");
}

/* ================================================================ */
/* 9. pid_reset behavior. After several AUTO cycles, call pid_reset
 *      and assert the next pid_update behaves like the first call
 *      on a freshly-pid_init'd instance. The gains/clamp/mode
 *      persist. */
static void test_pid_reset_clears_state_keeps_gains(void)
{
    pid_t pid;
    pid_init(&pid, (int16_t)256, (int16_t)128, (int16_t)256,
             (int16_t)-1000, (int16_t)1000);

    /* Build up a non-zero integrator and prev_measurement. */
    (void)pid_update(&pid, (int16_t)100, (int16_t)0);
    (void)pid_update(&pid, (int16_t)100, (int16_t)20);
    (void)pid_update(&pid, (int16_t)100, (int16_t)40);
    CHECK(pid.integrator_q8 != 0, "reset: pre-state non-zero integrator");
    CHECK(pid.have_prev_measurement == true, "reset: pre-state have_prev_measurement true");

    pid_reset(&pid);
    CHECK(pid.integrator_q8 == 0, "reset: integrator zeroed");
    CHECK(pid.have_prev_measurement == false, "reset: have_prev_measurement cleared");
    /* Gains and clamp persist: */
    CHECK(pid.kp_q8 == 256 && pid.ki_q8 == 128 && pid.kd_q8 == 256,
          "reset: gains preserved");
    CHECK(pid.out_min == -1000 && pid.out_max == 1000,
          "reset: clamp range preserved");
    CHECK(pid.mode == PID_MODE_AUTO, "reset: mode preserved");

    /* First call after reset: D=0 (no prev measurement). */
    int16_t out = pid_update(&pid, (int16_t)100, (int16_t)0);
    int32_t p_q8 = mul_s16(256, 100);  /* 25600 */
    int32_t i_inc = mul_s16(128, 100); /* 12800, the increment */
    /* First call: integrator starts at 0 (reset), so post-call value
     * is just the increment 12800. sum = P + I + D = 25600 + 12800 + 0
     * = 38400, >>8 = 150. */
    int16_t exp = (int16_t)((p_q8 + i_inc + 0) >> 8);
    CHECK(out == exp, "reset: first call is P+I only (D gated off)");

    /* Second call: prev_measurement=0 now, so dmeas=0-0=0; D still 0.
     * Integrator now 12800, second call adds another 12800 to 25600. */
    out = pid_update(&pid, (int16_t)100, (int16_t)0);
    int16_t exp2 = 200;
    CHECK(out == exp2, "reset: second call adds I increment, P unchanged, D=0");
}

/* ================================================================ */
/* 10. Two independent instances never affect each other. */
static void test_two_independent_instances(void)
{
    pid_t a, b;
    pid_init(&a, (int16_t)256, (int16_t)0, (int16_t)0,
             (int16_t)-1000, (int16_t)1000);
    pid_init(&b, (int16_t)512, (int16_t)0, (int16_t)0,
             (int16_t)-1000, (int16_t)1000);

    /* A and B with different gains, driven by the same setpoint
     * interleaved. Their outputs should follow their own gains,
     * not leak. */
    int16_t a_out = 0, b_out = 0;
    for (int n = 0; n < 5; n++) {
        a_out = pid_update(&a, (int16_t)100, (int16_t)0);
        b_out = pid_update(&b, (int16_t)100, (int16_t)0);
    }
    /* A: P = 256*100 = 25600, >>8 = 100. */
    CHECK(a_out == 100, "indep: A output reflects A's gain (256 -> 100)");
    /* B: P = 512*100 = 51200, >>8 = 200. */
    CHECK(b_out == 200, "indep: B output reflects B's gain (512 -> 200)");

    /* Now drive only B; A should be unaffected. */
    for (int n = 0; n < 5; n++) {
        b_out = pid_update(&b, (int16_t)100, (int16_t)0);
    }
    /* B with kp=512 and 10 steps of error=100: P=51200, >>8=200 still. */
    CHECK(b_out == 200, "indep: B still tracks its own gain");
    /* A's integrator is still 0 (it was P-only with ki=0). */
    CHECK(a.integrator_q8 == 0, "indep: A's integrator untouched by B's activity");
    /* A still produces 100 on the next call. */
    CHECK(pid_update(&a, (int16_t)100, (int16_t)0) == 100,
          "indep: A still produces its own output");
}

/* ================================================================ */

int main(void)
{
    test_pure_p();
    test_pure_i_accumulates();
    test_anti_windup_clamps();
    test_windup_recovery_immediate();
    test_no_derivative_kick_first_call();
    test_no_derivative_kick_after_reset();
    test_derivative_sign();
    test_final_output_clamps_p();
    test_final_output_clamps_d();
    test_final_output_clamps_i();
    test_bumpless_transfer();
    test_pid_reset_clears_state_keeps_gains();
    test_two_independent_instances();

    printf("test_pid: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
