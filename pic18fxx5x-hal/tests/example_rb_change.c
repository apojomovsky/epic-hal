/*
 * Smoke test for the RB<7:4> change-interrupt hook
 * (EPIC_GPIO_RegisterChangeCallback / RB_IRQHandler). The host sim does
 * not assert RBIF on a PORTB mismatch (would require intercepting every
 * CPU read of PORTB), so the test sets RBIF directly in INTCON and
 * checks the handler's own read/clear/callback ordering.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"

#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* Observed callback state. */
static volatile int    g_cb_calls;
static volatile uint8_t g_cb_last;

/** @brief  RB change callback, records the call and the PORTB byte. */
static void on_rb_change(uint8_t portb_value)
{
    g_cb_calls++;
    g_cb_last = portb_value;
}

/** @brief  Reset the observed callback state to the sentinel value. */
static void reset_observed(void)
{
    g_cb_calls = 0;
    g_cb_last  = 0xFFU;   /* sentinel, no real PORTB read yields 0xFF */
}

/** @brief  Assert RBIF directly.
 *
 *          The documented test-only fallback: the host sim does not assert
 *          RBIF on a PORTB mismatch.
 */
static void assert_rbif(void)
{
    EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF;
}

/** @brief  Return whether the RBIF flag is pending in INTCON. */
static uint8_t rbif_pending(void)
{
    return (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_RBIF) ? 1U : 0U;
}

/** @brief  Verify the handler does nothing when RBIF is not pending. */
static void test_noop_when_not_pending(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    EPIC_REG8(PIC_REG_PORTB) = 0xA5U;

    RB_IRQHandler();   /* RBIF not pending: must do nothing. */

    CHECK(g_cb_calls == 0, "noop: callback not invoked when RBIF clear");
    CHECK(rbif_pending() == 0, "noop: flag still clear");
    CHECK(g_cb_last == 0xFFU, "noop: callback arg untouched");
}

/** @brief  Verify the handler invokes the callback once and clears RBIF. */
static void test_fires_when_pending(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    EPIC_REG8(PIC_REG_PORTB) = 0x3CU;   /* the byte the handler must read */
    assert_rbif();
    CHECK(rbif_pending() == 1, "fires: RBIF set before handler");

    RB_IRQHandler();

    CHECK(g_cb_calls == 1, "fires: callback invoked exactly once");
    CHECK(g_cb_last == 0x3CU, "fires: callback received the PORTB byte");
    CHECK(rbif_pending() == 0, "fires: RBIF cleared by handler");
}

/** @brief  Verify a NULL callback is safe and the flag is still cleared. */
static void test_null_callback_safe(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(NULL);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    EPIC_REG8(PIC_REG_PORTB) = 0x00U;
    assert_rbif();

    RB_IRQHandler();   /* must not crash, must still clear the flag */

    CHECK(g_cb_calls == 0, "null: no callback invocation");
    CHECK(rbif_pending() == 0, "null: flag still cleared");
}

/** @brief  Verify a full dispatch pass routes to RB_IRQHandler. */
static void test_dispatch_reaches_handler(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);

    EPIC_IRQ_ClearFlag(PIC18_IRQ_RB);
    EPIC_REG8(PIC_REG_PORTB) = 0x96U;
    assert_rbif();

    /* A full dispatch pass must route to RB_IRQHandler. */
    epic_dispatch_all_irqs();

    CHECK(g_cb_calls == 1, "dispatch: fan-out reached RB_IRQHandler");
    CHECK(g_cb_last == 0x96U, "dispatch: correct byte delivered");
    CHECK(rbif_pending() == 0, "dispatch: RBIF cleared");
}

/** @brief  RB<7:4> change-interrupt hook smoke test runner. */
int main(void)
{
    /* epic_harness_init resets the sim SFRs to POR and wires the family
     * dispatcher as the sim IRQ callback. */
    epic_harness_init(1024UL);

    test_noop_when_not_pending();
    test_fires_when_pending();
    test_null_callback_safe();
    test_dispatch_reaches_handler();

    printf("example_rb_change: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
