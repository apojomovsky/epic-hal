/**
 * Timer0 host-sim probe. Host-only: advances the TMR0 model through
 * epic_harness_tick, so it is not in the XC8 build. Covers ratio
 * lookups, Init/Start validation, the 8-bit match period and flag, the
 * 16-bit rollover with Start preload, prescaler and postscaler division,
 * the T0OUT toggle, the T0ASYNC source policy, the frozen external
 * rows, the overflow callback through the shared dispatcher, Stop, and
 * the DeInit POR state.
 *
 * Expected register image (FOSC/4, 1:1, 1:1, 8-bit, period 7, no
 * callback, after Init, before Start):
 *   T0CON1 = 0x40   (T0CS=010 FOSC/4, T0ASYNC=0, T0CKPS=0000)
 *   T0CON0 = 0x00   (T0EN=0, T016BIT=0, T0OUTPS=0000)
 *   TMR0H  = 0x07   (match value)
 *   TMR0L  = 0x00
 *   PIE0   = 0x00   (TMR0IE clear without a callback)
 * After Start: T0CON0 = 0x80 (T0EN set, rest unchanged).
 * Flag timing at 1:1/1:1: 7 ticks reach TMR0L 7 without flagging, the
 * 8th resets TMR0L to 0 with TMR0IF. TMR0L never holds period+1.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_timer0.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer0 overflow callback that records its call.
 */
static void t0_cb(void)
{
    s_calls++;
}

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Timer0 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;

    epic_harness_init(10000UL);

    /* The sim vectors timer callbacks only with GIE set, as on silicon. */
    EPIC_IRQ_Restore(1U);
    /* Ratio lookups, including the out-of-range fallbacks. */
    ok &= (EPIC_TIMER0_PrescalerToRatio(TIMER0_PRESCALER_1_1) == 1U);
    ok &= (EPIC_TIMER0_PrescalerToRatio(TIMER0_PRESCALER_1_2) == 2U);
    ok &= (EPIC_TIMER0_PrescalerToRatio(TIMER0_PRESCALER_1_8) == 8U);
    ok &= (EPIC_TIMER0_PrescalerToRatio(TIMER0_PRESCALER_1_256) == 256U);
    ok &= (EPIC_TIMER0_PrescalerToRatio(TIMER0_PRESCALER_1_32768) == 32768U);
    ok &= (EPIC_TIMER0_PrescalerToRatio((TIMER0_PrescalerTypeDef)16) == 1U);
    ok &= (EPIC_TIMER0_PostscalerToRatio(TIMER0_POSTSCALER_1_1) == 1U);
    ok &= (EPIC_TIMER0_PostscalerToRatio(TIMER0_POSTSCALER_1_2) == 2U);
    ok &= (EPIC_TIMER0_PostscalerToRatio(TIMER0_POSTSCALER_1_16) == 16U);
    ok &= (EPIC_TIMER0_PostscalerToRatio((TIMER0_PostscalerTypeDef)16) == 1U);

    /* Validation: NULL and out-of-range fields are rejected unwritten. */
    ok &= (EPIC_TIMER0_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER0_Start(NULL) == EPIC_INVALID);
    h.ClockSource = (TIMER0_ClockSourceTypeDef)7;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_INVALID);
    h.ClockSource = TIMER0_CS_FOSC4;
    h.Prescaler = (TIMER0_PrescalerTypeDef)16;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_INVALID);
    h.Prescaler = TIMER0_PRESCALER_1_1;
    h.Postscaler = (TIMER0_PostscalerTypeDef)16;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_INVALID);
    h.Postscaler = TIMER0_POSTSCALER_1_1;
    ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0EN) == 0U);

    /* Init stages the control image without starting: T0EN clear,
     * interrupt disabled without a callback. */
    h.Period = 7U;
    h.OverflowCallback = NULL;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_T0CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_TMR0H) == 7U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0700U);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) == 0U);

    /* Start sets T0EN in one T0CON0 write with the counter at zero. */
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON0) == PIC_T0CON0_T0EN);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0700U);

    /* Match period: 7 ticks reach TMR0L 7 without flagging, the 8th
     * wraps to 0 with TMR0IF and toggles T0OUT. */
    step(7U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0707U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0700U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_TMR0) == 1U);
    ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0OUT) != 0U);

    /* Stop freezes the counter; DeInit restores the POR state. */
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0EN) == 0U);
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_TMR0);
    step(20U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0700U);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON0) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0xFF00U);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_TMR0) == 0U);

    /* Prescaler 1:4 with period 0: a match every tick, a flag every 4th. */
    h.Prescaler = TIMER0_PRESCALER_1_4;
    h.Postscaler = TIMER0_POSTSCALER_1_1;
    h.Period = 0U;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    step(3U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0000U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* Postscaler 1:2 with period 1: a match every 2 ticks, a flag
     * every 2nd match. */
    h.Prescaler = TIMER0_PRESCALER_1_1;
    h.Postscaler = TIMER0_POSTSCALER_1_2;
    h.Period = 1U;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    step(3U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* 16-bit mode preloads Period on Start: 0xFFF0 overflows after 16
     * ticks, and a counter write round-trips through TMR0H:TMR0L. */
    h.Prescaler = TIMER0_PRESCALER_1_1;
    h.Postscaler = TIMER0_POSTSCALER_1_1;
    h.Is16Bit = 1U;
    h.Period = 0xFFF0U;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON0) == PIC_T0CON0_T016BIT);
    ok &= (EPIC_TIMER0_ReadCounter() == 0xFFF0U);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON0) ==
        (uint8_t)(PIC_T0CON0_T016BIT | PIC_T0CON0_T0EN));
    step(15U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0xFFFFU);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0000U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    EPIC_TIMER0_WriteCounter(0x1234U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x1234U);
    h.Is16Bit = 0U;
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* 8-bit WriteCounter keeps the TMR0H match value: starting above it
     * matches on the very next tick. */
    h.Period = 0x3AU;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    EPIC_TIMER0_WriteCounter(0x125AU);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x3A5AU);
    step(1U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x3A00U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* T0ASYNC follows the source: set for LFINTOSC and LC1OUT, clear
     * for HFINTOSC. */
    h.ClockSource = TIMER0_CS_LFINTOSC;
    h.Period = 0xFFU;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0x90U);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    h.ClockSource = TIMER0_CS_LC1OUT;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0xD0U);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    h.ClockSource = TIMER0_CS_HFINTOSC;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0x60U);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* An external row with an idle pin never ticks: 50 ticks move nothing
     * (edge counting on a mapped pin is covered by example_pps). */
    h.ClockSource = TIMER0_CS_T0CKIPPS_INV;
    h.Period = 7U;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    step(50U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0700U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* Overflow callback: Init enables TMR0IE and PEIE, the dispatcher
     * clears the flag and runs the slot exactly once per match, and
     * T0OUT toggles once per flag. */
    s_calls = 0U;
    h.ClockSource = TIMER0_CS_FOSC4;
    h.Period = 3U;
    h.OverflowCallback = t0_cb;
    ok &= (EPIC_TIMER0_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER0_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (s_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0OUT) != 0U);
    step(4U);
    ok &= (s_calls == 2U);
    ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0OUT) == 0U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) == 0U);
    step(8U);
    ok &= (s_calls == 2U);

    epic_harness_log("timer0: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
