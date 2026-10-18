/**
 * Timer1 host-sim probe. Host-only: advances the TMR1 model through
 * epic_harness_tick, so it is not in the XC8 build. Covers ratio
 * lookups, Init/Start validation, the register images, the 16-bit
 * rollover flag, prescaler and FOSC clocking, the TMR0 overflow and
 * T1CKI clock rows, the overflow callback through the shared
 * dispatcher, the DeInit POR state, and the gate: polarity, GE, GVAL,
 * toggle and single-pulse against the Timer2 postscaled pulse.
 *
 * Expected register image (FOSC/4, 1:1, synchronized, gate off, after
 * Init, before Start):
 *   T1CLK  = 0x01   (CS=0001 FOSC/4)
 *   T1GATE = 0x00   (GSS=T1GPPS)
 *   T1GCON = 0x00   (GE, GPOL, GTM, GSPM all clear, GVAL clear)
 *   T1CON  = 0x02   (RD16 set, CKPS=00, SYNC=0, ON=0)
 *   PIE4   = 0x00   (TMR1IE clear without a callback)
 * After Start: T1CON = 0x03.
 * Flag timing at 1:1: from 0xFFFE the 2nd tick rolls 0xFFFF to 0x0000
 * and sets TMR1IF.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer1 overflow callback that records its call.
 */
static void t1_cb(void)
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
 * @brief Init and start Timer1 gated by the TMR2 postscaled pulse, then
 * start Timer2 free-running with a 10 tick period (PR2 9, 1:1, 1:1).
 * @param toggle Gate toggle mode.
 * @param single_pulse Gate single-pulse mode, armed before Timer2 starts.
 */
static void start_tmr2_gate(uint8_t toggle, uint8_t single_pulse)
{
    TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

    t1.Gate.Enable = 1U;
    t1.Gate.ActiveHigh = 1U;
    t1.Gate.Toggle = toggle;
    t1.Gate.SinglePulse = single_pulse;
    t1.Gate.Source = TIMER1_GATE_TMR2_POST;
    t2.Period = 9U;
    (void)EPIC_TIMER1_Init(&t1);
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER1_Start(&t1);
    if (single_pulse != 0U)
    {
        (void)EPIC_TIMER1_GateArm();
    }
    (void)EPIC_TIMER2_Start(&t2);
}

/**
 * @brief Timer1 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;

    epic_harness_init(10000UL);

    /* The sim vectors timer callbacks only with GIE set, as on silicon. */
    EPIC_IRQ_Restore(1U);

    /* Ratio lookups, including the out-of-range fallback. */
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_1) == 1U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_2) == 2U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_4) == 4U);
    ok &= (EPIC_TIMER1_PrescalerToRatio(TIMER1_PRESCALER_1_8) == 8U);
    ok &= (EPIC_TIMER1_PrescalerToRatio((TIMER1_PrescalerTypeDef)4) == 1U);

    /* Validation: NULL, reserved and out-of-range codes are rejected
     * unwritten. */
    ok &= (EPIC_TIMER1_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER1_Start(NULL) == EPIC_INVALID);
    h.ClockSource = (TIMER1_ClockSourceTypeDef)9;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.ClockSource = (TIMER1_ClockSourceTypeDef)16;
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_INVALID);
    h.ClockSource = TIMER1_CS_FOSC4;
    h.Prescaler = (TIMER1_PrescalerTypeDef)4;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.Prescaler = TIMER1_PRESCALER_1_1;
    h.Gate.Source = (TIMER1_GateSourceTypeDef)2;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.Gate.Source = (TIMER1_GateSourceTypeDef)25;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_INVALID);
    h.Gate.Source = TIMER1_GATE_T1GPPS;
    ok &= ((EPIC_REG8(PIC_REG_T1CON) & PIC_T1CON_ON) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0U);

    /* Init stages the control image without starting: ON clear,
     * interrupt disabled without a callback. */
    h.ReloadValue = 0xFFFEU;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1GCON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x02U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFFEU);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);

    /* Start sets ON in one T1CON write. Two ticks roll the counter over
     * and flag it; Stop freezes the count. */
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x03U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0xFFFFU);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0x0000U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_TMR1) == 1U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0x0000U);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1GCON) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U);

    /* Prescaler 1:4 counts every 4th tick; Async sets SYNC in the image. */
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.Prescaler = TIMER1_PRESCALER_1_4;
    h.Async = 1U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x27U);
    step(8U);
    ok &= (EPIC_TIMER1_ReadCounter() == 2U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* FOSC clocks the counter four times per instruction tick. */
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.ClockSource = TIMER1_CS_FOSC;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x02U);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 12U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Timer0 overflow clocks the counter once per Timer0 flag: period 3
     * at FOSC/4 flags every 4 ticks, so 16 ticks count 4. */
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;

        t0.Period = 3U;
        h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
        h.ClockSource = TIMER1_CS_TMR0_OVF;
        ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x08U);
        ok &= (EPIC_TIMER0_Init(&t0) == EPIC_OK);
        ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
        ok &= (EPIC_TIMER0_Start(&t0) == EPIC_OK);
        step(16U);
        ok &= (EPIC_TIMER1_ReadCounter() == 4U);
        ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
        ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    }

    /* T1CKI counts rising edges of the driven input only. */
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.ClockSource = TIMER1_CS_T1CKIPPS;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    pic16f188xx_sim_drive_input('C', 0U, 1U);
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 1U);
    pic16f188xx_sim_drive_input('C', 0U, 0U);
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 1U);
    pic16f188xx_sim_drive_input('C', 0U, 1U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 2U);
    pic16f188xx_sim_drive_input('C', 0U, 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Rows without a stimulus model (LC1) freeze the counter. */
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.ClockSource = TIMER1_CS_LC1OUT;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(8U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Overflow callback: Init enables TMR1IE and PEIE, the dispatcher
     * clears TMR1IF and runs the slot once per rollover. */
    s_calls = 0U;
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.ReloadValue = 0xFFFEU;
    h.OverflowCallback = t1_cb;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(2U);
    ok &= (s_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);
    step(8U);
    ok &= (s_calls == 1U);

    /* Gate on the T1G pin: GE with active-high holds while the pin is
     * low and counts while it is high; GVAL follows the gate. */
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.Gate.Enable = 1U;
    h.Gate.ActiveHigh = 1U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1GCON) == 0xC0U);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(6U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    pic16f188xx_sim_drive_input('B', 5U, 1U);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 5U);
    ok &= (EPIC_TIMER1_GateValue() == 1U);
    pic16f188xx_sim_drive_input('B', 5U, 0U);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 5U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Active-low is the mirror image. */
    h.Gate.ActiveHigh = 0U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1GCON) == 0x80U);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (EPIC_TIMER1_ReadCounter() == 4U);
    ok &= (EPIC_TIMER1_GateValue() == 1U);
    pic16f188xx_sim_drive_input('B', 5U, 1U);
    step(4U);
    ok &= (EPIC_TIMER1_ReadCounter() == 4U);
    pic16f188xx_sim_drive_input('B', 5U, 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Gate disabled: GVAL still reports the gate, counting ignores it. */
    h.Gate.Enable = 0U;
    h.Gate.ActiveHigh = 1U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (EPIC_TIMER1_ReadCounter() == 4U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* TMR2 postscaled as the gate source, level mode: each pulse lets
     * one tick count, and Timer2 pulses at ticks 10 and 20. */
    start_tmr2_gate(0U, 0U);
    step(25U);
    ok &= (EPIC_TIMER1_ReadCounter() == 2U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Toggle mode measures the full Timer2 period: the gate opens on
     * the first pulse and closes on the second, counting 10 ticks. */
    start_tmr2_gate(1U, 0U);
    step(10U);
    ok &= (EPIC_TIMER1_GateValue() == 1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 1U);
    step(9U);
    ok &= (EPIC_TIMER1_ReadCounter() == 10U);
    step(1U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 10U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* Single pulse with toggle: GGO/DONE stays set until the trailing
     * edge, one cycle is captured, and later cycles are ignored. */
    start_tmr2_gate(1U, 1U);
    ok &= (EPIC_TIMER1_GateBusy() == 1U);
    step(15U);
    ok &= (EPIC_TIMER1_GateBusy() == 1U);
    step(5U);
    ok &= (EPIC_TIMER1_GateBusy() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 10U);
    step(60U);
    ok &= (EPIC_TIMER1_ReadCounter() == 10U);
    ok &= (EPIC_TIMER1_GateBusy() == 0U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    epic_harness_log("timer1: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
