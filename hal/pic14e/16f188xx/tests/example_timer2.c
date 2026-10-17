/**
 * Timer2 host-sim probe. Host-only: advances the TMR2 model through
 * epic_harness_tick, so it is not in the XC8 build. Covers ratio
 * lookups, Init/Start validation, the register images, the PR2 match
 * period and flag, prescaler and postscaler division, FOSC clocking,
 * the match callback through the shared dispatcher, the DeInit POR
 * state, and the T2HLT modes: hardware gate, edge and level reset,
 * software one-shot, and the frozen edge-start rows.
 *
 * Expected register image (FOSC/4, 1:1, 1:1, free-running software
 * mode, period 7, no callback, after Init, before Start):
 *   T2CLKCON = 0x01   (CS=0001 FOSC/4)
 *   T2RST    = 0x00   (RSEL=T2INPPS)
 *   T2HLT    = 0x00   (MODE=0, CKPOL, CKSYNC, PSYNC clear)
 *   T2CON    = 0x00   (ON=0, CKPS=000, OUTPS=0000)
 *   PR2      = 0x07
 *   PIE4     = 0x00   (TMR2IE clear without a callback)
 * After Start: T2CON = 0x80, TMR2 = 0.
 * Flag timing at 1:1/1:1: 7 ticks reach TMR2 7 without flagging, the
 * 8th clears TMR2 with TMR2IF. TMR2 never holds PR2+1.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_timer2.h"

/* Callback call count, read by the checks in main(). */
static unsigned int s_calls;

/**
 * @brief Timer2 match callback that records its call.
 */
static void t2_cb(void)
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
 * @brief Report whether TMR2IF is set.
 * @return Non-zero while the flag is set.
 */
static unsigned int t2_flag(void)
{
    return (EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U;
}

/**
 * @brief Init and start a Timer2 on the T2IN reset input in one mode.
 * @param mode T2HLT mode to program.
 * @param period PR2 value.
 */
static void start_mode(TIMER2_ModeTypeDef mode, uint8_t period)
{
    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;

    h.Mode = mode;
    h.Period = period;
    (void)EPIC_TIMER2_Init(&h);
    (void)EPIC_TIMER2_Start(&h);
}

/**
 * @brief Stop Timer2, restore POR and release the T2IN input.
 * @return 1 when both calls succeeded.
 */
static int end_mode(void)
{
    int ok = (EPIC_TIMER2_Stop() == EPIC_OK);

    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    return ok;
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
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_16) == 16U);
    ok &= (EPIC_TIMER2_PrescalerToRatio(TIMER2_PRESCALER_1_128) == 128U);
    ok &= (EPIC_TIMER2_PrescalerToRatio((TIMER2_PrescalerTypeDef)8) == 1U);
    ok &= (EPIC_TIMER2_PostscalerToRatio(TIMER2_POSTSCALER_1_1) == 1U);
    ok &= (EPIC_TIMER2_PostscalerToRatio(TIMER2_POSTSCALER_1_16) == 16U);
    ok &= (EPIC_TIMER2_PostscalerToRatio((TIMER2_PostscalerTypeDef)16) == 1U);

    /* Validation: NULL, reserved and out-of-range codes are rejected
     * unwritten. */
    ok &= (EPIC_TIMER2_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_TIMER2_Start(NULL) == EPIC_INVALID);
    h.ClockSource = (TIMER2_ClockSourceTypeDef)14;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.ClockSource = TIMER2_CS_FOSC4;
    h.Prescaler = (TIMER2_PrescalerTypeDef)8;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.Prescaler = TIMER2_PRESCALER_1_1;
    h.Postscaler = (TIMER2_PostscalerTypeDef)16;
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_INVALID);
    h.Postscaler = TIMER2_POSTSCALER_1_1;
    h.Mode = (TIMER2_ModeTypeDef)0x10;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.Mode = (TIMER2_ModeTypeDef)0x14;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.Mode = (TIMER2_ModeTypeDef)0x18;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.Mode = TIMER2_MODE_FREE_SOFTWARE;
    h.ResetSource = (TIMER2_ResetSourceTypeDef)1;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.ResetSource = (TIMER2_ResetSourceTypeDef)18;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_INVALID);
    h.ResetSource = TIMER2_RST_T2INPPS;
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_ON) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0U);

    /* Init stages the control image without starting: ON clear,
     * interrupt disabled without a callback. */
    h.Period = 7U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 7U);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);

    /* Start sets ON in one T2CON write with TMR2 zeroed. */
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x80U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);

    /* Match period: 7 ticks reach TMR2 7 without flagging, the 8th
     * clears TMR2 with TMR2IF. */
    step(7U);
    ok &= (EPIC_TIMER2_ReadCounter() == 7U);
    ok &= (t2_flag() == 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (t2_flag() != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_TMR2) == 1U);

    /* Stop freezes the counter; DeInit restores the POR state. */
    step(3U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    step(5U);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 0xFFU);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (t2_flag() == 0U);

    /* Prescaler 1:4 and postscaler 1:2 at PR2 1: a match every 8
     * ticks, a flag every 16. */
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.Prescaler = TIMER2_PRESCALER_1_4;
    h.Postscaler = TIMER2_POSTSCALER_1_2;
    h.Period = 1U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0xA1U);
    step(15U);
    ok &= (t2_flag() == 0U);
    step(1U);
    ok &= (t2_flag() != 0U);
    ok &= (end_mode() != 0);

    /* Prescaler 1:128 at PR2 0 matches on every clock: flag per 128 ticks. */
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.Prescaler = TIMER2_PRESCALER_1_128;
    h.Period = 0U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0xF0U);
    step(127U);
    ok &= (t2_flag() == 0U);
    step(1U);
    ok &= (t2_flag() != 0U);
    ok &= (end_mode() != 0);

    /* FOSC clocks four times per tick: PR2 7 matches every 2 ticks. */
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.ClockSource = TIMER2_CS_FOSC;
    h.Period = 7U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x02U);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(1U);
    ok &= (t2_flag() == 0U);
    step(1U);
    ok &= (t2_flag() != 0U);
    ok &= (end_mode() != 0);

    /* The T2HLT image carries MODE plus the three option bits. */
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.Mode = TIMER2_MODE_FREE_GATE_HIGH;
    h.ResetSource = TIMER2_RST_LC4OUT;
    h.FallingEdge = 1U;
    h.SyncOn = 1U;
    h.SyncPrescaler = 1U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0xE1U);
    ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x11U);
    ok &= (end_mode() != 0);

    /* Match callback: Init enables TMR2IE and PEIE, the dispatcher
     * clears TMR2IF and runs the slot once per match. */
    s_calls = 0U;
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.Period = 3U;
    h.OverflowCallback = t2_cb;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (s_calls == 1U);
    ok &= (t2_flag() == 0U);
    step(4U);
    ok &= (s_calls == 2U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);
    step(8U);
    ok &= (s_calls == 2U);

    /* Hardware gate, active high: TMR2 holds while T2IN is low, counts
     * while it is high and holds (not resets) when it falls again. */
    start_mode(TIMER2_MODE_FREE_GATE_HIGH, 100U);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x01U);
    step(10U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(3U);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    step(10U);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    ok &= (end_mode() != 0);

    /* Hardware gate, active low. */
    start_mode(TIMER2_MODE_FREE_GATE_LOW, 100U);
    step(4U);
    ok &= (EPIC_TIMER2_ReadCounter() == 4U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(10U);
    ok &= (EPIC_TIMER2_ReadCounter() == 4U);
    ok &= (end_mode() != 0);

    /* Rising-edge reset restarts the count; a falling edge does not. */
    start_mode(TIMER2_MODE_FREE_RESET_RISING, 100U);
    step(20U);
    ok &= (EPIC_TIMER2_ReadCounter() == 20U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 1U);
    step(9U);
    ok &= (EPIC_TIMER2_ReadCounter() >= 9U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() >= 10U);
    ok &= (end_mode() != 0);

    /* Falling-edge reset acts on the fall only. */
    start_mode(TIMER2_MODE_FREE_RESET_FALLING, 100U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(20U);
    ok &= (EPIC_TIMER2_ReadCounter() == 20U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 1U);
    ok &= (end_mode() != 0);

    /* Any-edge reset acts on both. */
    start_mode(TIMER2_MODE_FREE_RESET_ANY_EDGE, 100U);
    step(15U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 1U);
    step(14U);
    ok &= (EPIC_TIMER2_ReadCounter() >= 14U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 1U);
    ok &= (end_mode() != 0);

    /* Low-level reset: held at zero while T2IN is low, runs while high,
     * returns to zero the moment it falls. */
    start_mode(TIMER2_MODE_FREE_RESET_LOW, 100U);
    step(10U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(6U);
    ok &= (EPIC_TIMER2_ReadCounter() == 6U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    step(1U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (end_mode() != 0);

    /* High-level reset is the mirror image. */
    start_mode(TIMER2_MODE_FREE_RESET_HIGH, 100U);
    step(6U);
    ok &= (EPIC_TIMER2_ReadCounter() == 6U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(4U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (end_mode() != 0);

    /* Software one-shot: the first match flags, clears ON and stops TMR2
     * at 0; it stays stopped until Start runs again. */
    start_mode(TIMER2_MODE_ONESHOT_SOFTWARE, 3U);
    step(3U);
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_ON) != 0U);
    ok &= (t2_flag() == 0U);
    step(1U);
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_ON) == 0U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (t2_flag() != 0U);
    EPIC_REG8(PIC_REG_PIR4) = 0U;
    step(20U);
    ok &= (t2_flag() == 0U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
    h.Mode = TIMER2_MODE_ONESHOT_SOFTWARE;
    h.Period = 3U;
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    step(4U);
    ok &= (t2_flag() != 0U);
    ok &= ((EPIC_REG8(PIC_REG_T2CON) & PIC_T2CON_ON) == 0U);
    ok &= (end_mode() != 0);

    /* Edge-start modes are not modelled: the counter stays frozen. */
    start_mode(TIMER2_MODE_ONESHOT_RISING, 3U);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(10U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= (t2_flag() == 0U);
    ok &= (end_mode() != 0);

    epic_harness_log("timer2: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
