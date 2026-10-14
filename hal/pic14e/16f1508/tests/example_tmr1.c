/**
 * Timer1 host-sim probe. Host-only: advances the TMR1 model through
 * epic_harness_tick and the external counter path through the sim pulse hook,
 * so it is not in the XC8 build. Covers ratio lookups, Init/Start validation,
 * the 16-bit overflow period and flag, prescaler division, the external T1CKI
 * counter the quadrature assist feeds, the overflow callback through the
 * shared dispatcher, Stop, the DeInit POR state, and the 256:255 beat ratio
 * of TMR2 against a preloaded TMR1.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f1508_timer1.h"
#include "peripherals/pic16f1508_timer2.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "pic16f1508_sim.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer1 overflow callback that records its call.
 */
static void tmr1_cb(void)
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
 * @brief Timer1 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    TIMER2_HandleTypeDef h2 = TIMER2_HANDLE_DEFAULT;

    epic_harness_init(10000UL);

    /* Ratio lookups, including the out-of-range fallbacks. */
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_1) == 1U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_2) == 2U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_4) == 4U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_8) == 8U);
    ok &= (EPIC_TIMER1_PrescalerToRatio((TIMER1_PrescalerTypeDef)4) == 1U);

    /* Validation: NULL, non-internal/external clocks and bad prescalers
     * are rejected unwritten. */
    ok &= (EPIC_TIMER1_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER1_Start(NULL) == EPIC_INVALID);
    h.ClockSource = (TIMER1_ClockSourceTypeDef)1;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_INVALID);
    h.ClockSource = (TIMER1_ClockSourceTypeDef)3;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.ClockSource = TIMER1_CLOCK_INTERNAL;
    h.Prescaler = (TIMER1_PrescalerTypeDef)4;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.Prescaler = TIMER1_PRESCALER_1_1;
    ok &= ((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) == 0U);

    /* Init stages the reload without starting: ON clear, interrupt off. */
    h.ReloadValue = 0xFFF0U;
    h.OverflowCallback = NULL;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFF0U);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) == 0U);

    /* Start programs T1CON in one write: prescaler 1:1, FOSC/4, ON. */
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == PIC_T1CON_TMR1ON);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFF0U);

    /* Overflow period: 16 counts wrap to 0 with TMR1IF. */
    step(15U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFFFU);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_TMR1) == 1U);

    /* Stop freezes the counter; DeInit restores the POR state. */
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_TMR1ON) == 0U);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR1);
    step(20U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) == 0U);

    /* Prescaler 1:2 from 0xFFFE: two counts per tick, overflow on the 4th. */
    h.Prescaler = TIMER1_PRESCALER_1_2;
    h.ReloadValue = 0xFFFEU;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON)
        == (uint8_t)(((uint8_t)TIMER1_PRESCALER_1_2 << PIC_T1CON_T1CKPS_SHIFT) | PIC_T1CON_TMR1ON));
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFFFU);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) != 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Overflow callback: Init enables TMR1IE and PEIE, the dispatcher clears
     * the flag and runs the slot exactly once per overflow. */
    s_calls = 0U;
    h.Prescaler = TIMER1_PRESCALER_1_1;
    h.ReloadValue = 0xFFFEU;
    h.OverflowCallback = tmr1_cb;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(2U);
    ok &= (s_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) == 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR1IE) == 0U);
    EPIC_TIMER1_WriteCounter(0xFFFEU);
    EPIC_REG8(PIC_REG_T1CON) = PIC_T1CON_TMR1ON;
    step(2U);
    ok &= (s_calls == 1U);
    EPIC_REG8(PIC_REG_T1CON) = 0U;
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_TMR1);

    /* External counter: instruction ticks never advance it, T1CKI edges do. */
    h.ClockSource = TIMER1_CLOCK_EXTERNAL;
    h.Prescaler = TIMER1_PRESCALER_1_1;
    h.ReloadValue = 0U;
    h.OverflowCallback = NULL;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON)
        == (uint8_t)((uint8_t)((uint8_t)TIMER1_CLOCK_EXTERNAL << PIC_T1CON_TMR1CS_SHIFT)
            | PIC_T1CON_T1SYNC | PIC_T1CON_TMR1ON));
    step(100U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    ok &= (EPIC_TIMER1_ReadCounter() == 3U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* External prescaler 1:2: five edges count two. */
    h.ClockSource = TIMER1_CLOCK_EXTERNAL;
    h.Prescaler = TIMER1_PRESCALER_1_2;
    h.ReloadValue = 0U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    pic16f1508_sim_timer1_external_pulse();
    ok &= (EPIC_TIMER1_ReadCounter() == 2U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Counter round trip through the driver accessors. */
    EPIC_TIMER1_WriteCounter(0x1234U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0x1234U);
    EPIC_TIMER1_WriteCounter(0U);

    /* Beat ratio: TMR2 matches every 256 ticks (PR2 255) while a TMR1
     * preloaded to 0xFF01 overflows every 255. The R flag leads by one. */
    h2.Prescaler = TIMER2_PRESCALER_1_1;
    h2.Postscaler = TIMER2_POSTSCALER_1_1;
    h2.Period = 0xFFU;
    h2.OverflowCallback = NULL;
    ok &= (EPIC_TIMER2_Init(&h2) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h2) == EPIC_OK);
    h.ClockSource = TIMER1_CLOCK_INTERNAL;
    h.Prescaler = TIMER1_PRESCALER_1_1;
    h.ReloadValue = 0xFF01U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(255U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR1IF) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF) != 0U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* T1_overflow reaches the CLC data multiplexers: the beat R input. */
    ok &= (EPIC_CLC_SourceCode(1U, CLC_IN_T1_OVERFLOW) == 6U);
    ok &= (EPIC_CLC_SourceCode(2U, CLC_IN_T1_OVERFLOW) == 2U);

    epic_harness_log("tmr1: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
