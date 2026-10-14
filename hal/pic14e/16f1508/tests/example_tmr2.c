/**
 * Timer2 host-sim probe. Host-only: advances the TMR2 model through
 * epic_harness_tick, so it is not in the XC8 build. Covers ratio lookups,
 * Init/Start validation, PR2 match period and flag, postscaler and prescaler
 * division, the match callback through the shared dispatcher, Stop, and the
 * DeInit POR state.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f1508_timer2.h"
#include "peripherals/pic16f1508_clc_src.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer2 match callback that records its call.
 */
static void tmr2_cb(void)
{
    s_calls++;
}

/**
 * @brief Step the simulator n instruction cycles.
 * @param n Cycle count.
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
 * @brief Timer2 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;

    epic_harness_init(10000UL);

    /* The sim vectors timer callbacks only with GIE set, as on silicon. */
    EPIC_IRQ_Restore(1U);
    /* Ratio lookups, including the out-of-range fallbacks. */
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_1) == 1U);
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_4) == 4U);
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_16) == 16U);
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_64) == 64U);
    ok &= (EPIC_TIMER2_PrescalerToRatio((TIMER2_PrescalerTypeDef)4) == 1U);
    ok &= (EPIC_TIMER2_PostscalerToRatio(TIMER2_POSTSCALER_1_1) == 1U);
    ok &= (EPIC_TIMER2_PostscalerToRatio(TIMER2_POSTSCALER_1_16) == 16U);
    ok &= (EPIC_TIMER2_PostscalerToRatio((TIMER2_PostscalerTypeDef)16) == 1U);

    /* Validation: NULL and out-of-range ratios are rejected unwritten. */
    ok &= (EPIC_TIMER2_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER2_Start(NULL) == EPIC_INVALID);
    h.Prescaler = (TIMER2_PrescalerTypeDef)4;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_INVALID);
    h.Prescaler = TIMER2_PRESCALER_1_1;
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_TMR2ON) == 0U);

    /* Init stages PR2 without starting: ON clear, interrupt disabled. */
    h.Period = 7U;
    h.OverflowCallback = NULL;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_TMR2ON) == 0U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 7U);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) == 0U);

    /* Start programs T2CON in one write and zeroes the counter. */
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON)
        == (uint8_t)(PIC_T2CON_TMR2ON | (uint8_t)TIMER2_PRESCALER_1_1));
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);

    /* PR2 match period: 7 ticks reach PR2 without flagging, the 8th wraps
     * to 0 with TMR2IF. TMR2 never holds PR2+1. */
    step(7U);
    ok &= (EPIC_TIMER2_ReadCounter() == 7U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_TMR2) == 1U);

    /* Stop freezes the counter; DeInit restores the POR state. */
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_TMR2ON) == 0U);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR2);
    step(20U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 0xFFU);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_TMR2) == 0U);

    /* Postscaler 1:2 with PR2 1: a match every 2 ticks, a flag every 2nd. */
    h.Prescaler = TIMER2_PRESCALER_1_1;
    h.Postscaler = TIMER2_POSTSCALER_1_2;
    h.Period = 1U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(3U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);

    /* A counter write clears a partial prescaler: 6 ticks in with prescaler
     * 1:4 and PR2 3, WriteCounter(0) restarts the count, so the match needs
     * 16 more ticks, not 10. */
    h.Prescaler = TIMER2_PRESCALER_1_4;
    h.Postscaler = TIMER2_POSTSCALER_1_1;
    h.Period = 3U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(6U);
    EPIC_TIMER2_WriteCounter(0U);
    step(14U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(2U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);

    /* Prescaler 1:4 with PR2 0: a match every tick, a flag every 4th. */
    h.Prescaler = TIMER2_PRESCALER_1_4;
    h.Postscaler = TIMER2_POSTSCALER_1_1;
    h.Period = 0U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(3U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);

    /* Match callback: Init enables TMR2IE and PEIE, the dispatcher clears
     * the flag and runs the slot exactly once per match. */
    s_calls = 0U;
    h.Prescaler = TIMER2_PRESCALER_1_1;
    h.Postscaler = TIMER2_POSTSCALER_1_1;
    h.Period = 3U;
    h.OverflowCallback = tmr2_cb;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (s_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(4U);
    ok &= (s_calls == 2U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) == 0U);
    step(8U);
    ok &= (s_calls == 2U);

    /* T2_match reaches the CLC data multiplexers: the debouncer slow clock. */
    ok &= (EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH) == 7U);
    ok &= (EPIC_CLC_SourceCode(2U, CLC_IN_T2_MATCH) == 3U);

    epic_harness_log("tmr2: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
