/* Smoke test for the RB<7:4> change-interrupt hook
 * (EPIC_GPIO_RegisterChangeCallback / RB_IRQHandler) on the sim. The
 * host sim does not model RABIF-on-mismatch, so the test asserts RABIF
 * directly and checks the handler's read/clear/callback ordering. */

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include "peripherals/pic16f63x_67x_68x_gpio.h"
#include "core/pic16_irq.h"
#include "core/epic_harness.h"

#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* Observed callback state. */
static volatile int    g_cb_calls;
static volatile uint8_t g_cb_last;

/**
 * @brief Record the callback invocation and its PORTB argument.
 * @param portb_value the PORTB byte passed by the handler.
 */
static void on_rb_change(uint8_t portb_value)
{
    g_cb_calls++;
    g_cb_last = portb_value;
}

/**
 * @brief Reset the observed callback state to its sentinel values.
 */
static void reset_observed(void)
{
    g_cb_calls = 0;
    g_cb_last  = 0xFFU;   /* sentinel, no real PORTB read yields 0xFF */
}

/**
 * @brief Assert RABIF directly (the documented test-only fallback).
 */
static void assert_rabif(void)
{
    EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_RABIF;
}

/**
 * @brief Report whether RABIF is pending.
 * @return 1 if RABIF is set, 0 otherwise.
 */
static uint8_t rabif_pending(void)
{
    return (EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_RABIF) ? 1U : 0U;
}

/**
 * @brief Verify the handler is a no-op when RABIF is clear.
 */
static void test_noop_when_not_pending(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);

    /* Ensure RABIF is clear, PORTB is some known value. */
    EPIC_IRQ_ClearFlag(PIC16_IRQ_RB);
    EPIC_REG8(PIC_REG_PORTB) = 0xA0U;

    RB_IRQHandler();   /* RABIF not pending: must do nothing. */
    CHECK(g_cb_calls == 0, "callback fired with RABIF clear");
    CHECK(rabif_pending() == 0U, "RABIF set after no-op handler");
}

/**
 * @brief Verify the handler reads PORTB, clears RABIF, and forwards
 *        the byte to the callback.
 */
static void test_read_clear_forward(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(on_rb_change);

    EPIC_REG8(PIC_REG_PORTB) = 0xB0U;
    assert_rabif();

    RB_IRQHandler();
    CHECK(g_cb_calls == 1, "callback not fired exactly once");
    CHECK(g_cb_last == 0xB0U, "callback got wrong PORTB byte");
    CHECK(rabif_pending() == 0U, "RABIF not cleared by handler");
}

/**
 * @brief Verify IOC gating: with no callback registered the handler
 *        still clears RABIF and never traps.
 */
static void test_no_callback_registered(void)
{
    reset_observed();
    EPIC_GPIO_RegisterChangeCallback(0);

    EPIC_REG8(PIC_REG_PORTB) = 0xC0U;
    assert_rabif();

    RB_IRQHandler();
    CHECK(g_cb_calls == 0, "callback fired with NULL registered");
    CHECK(rabif_pending() == 0U, "RABIF not cleared with NULL callback");
}

/**
 * @brief Smoke-test the RB-change hook on the sim backend.
 */
int main(void)
{
    pic16f63x_67x_68x_sim_reset();

    /* RB4..RB7 are inputs with IOC; drive RB5 high from the rig. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4 | GPIO_PIN_5 |
                   GPIO_PIN_6 | GPIO_PIN_7, GPIO_MODE_INPUT);
    EPIC_GPIO_SetPinIOC(5U, 1U);
    CHECK((EPIC_REG8(PIC_REG_IOCB) & 0x20U) != 0U, "IOCB5 not set");
    pic16f63x_67x_68x_sim_drive_input('B', 5U, 1U);
    CHECK(EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET,
          "RB5 reads low after drive high");

    test_noop_when_not_pending();
    test_read_clear_forward();
    test_no_callback_registered();

    if (g_fail == 0) {
        printf("OK: RB change hook passes (%d checks).\n", g_pass);
        return 0;
    }
    printf("FAIL: RB change hook (%d failures).\n", g_fail);
    return 1;
}
