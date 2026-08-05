/**
 * @file    example_ccp_pwm.c
 * @brief   End-to-end smoke test: ECCP1 half-bridge PWM with dead-band.
 *
 * @details
 *   Exercises Enhanced CCP (DS39632E §16.4): half-bridge output (P1M),
 *   dead-band (ECCP1DEL), auto-restart (PRSEN). The host sim doesn't
 *   toggle PWM pins, so the test checks the programmed register image
 *   (PR2=99, 50% duty, dead-band=12 -> `CCPR1L=0x0C`, `CCP1CON=0xAC`,
 *   `ECCP1DEL=0x8C`) and counts Timer2 overflows as the PWM period marker.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include <stdio.h>

#define EXPECTED_PERIOD_CYCLES  100UL
#define EXPECTED_OVERFLOWS      5U
#define SIM_CYCLES              ((EXPECTED_PERIOD_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t g_overflows   = 0;
static volatile uint32_t g_first_cycle = 0;
static uint32_t g_cycle = 0;

static void on_t2_overflow(void)
{
    if (g_overflows == 0U) g_first_cycle = g_cycle;
    g_overflows++;
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    /* 1. Timer2 as the PWM time base (DS39632E §16.4.3 step 4). */
    TIMER2_HandleTypeDef th = TIMER2_HANDLE_DEFAULT;
    th.Prescaler       = TIMER2_PRESCALER_1_1;
    th.Postscaler      = TIMER2_POSTSCALER_1_1;
    th.Period          = 99U;            /* PR2 = 99 -> 100 ticks/period. */
    th.OverflowCallback = on_t2_overflow;
    EPIC_TIMER2_Init(&th);

    /* 2. ECCP1 in half-bridge PWM, 50% duty, dead-band=12, auto-restart. */
    CCP_HandleTypeDef ch = { 0 };
    ch.Instance        = CCP_INSTANCE_1;
    ch.Mode            = CCP_MODE_PWM;
    ch.PWM.Period      = 99U;
    ch.PWM.Duty        = 50U;            /* 50 of 100 -> 50%. */
    ch.PWMOutputMode   = CCP_PWM_OUTPUT_HALF_BRIDGE;
    ch.DeadBand.Delay  = 12U;
    ch.DeadBand.AutoRestart = true;
    ch.AutoShutdown.Source  = CCP_AUTOSHUTDOWN_DISABLED;
    ch.EventCallback   = NULL;
    EPIC_CCP_Init(&ch);

    /* 3. Start Timer2 (writes PR2 + T2CON); PWM begins on the next period. */
    EPIC_TIMER2_Start(&th);
    EPIC_IRQ_Restore(1);

    /* 4. Verify the register image. */
    uint8_t cprl = pic8_sfr_read8(PIC_REG_CCPR1L);
    uint8_t con  = pic8_sfr_read8(PIC_REG_CCP1CON);
    uint8_t del  = pic8_sfr_read8(PIC_REG_ECCP1DEL);
    if (cprl != 12U) {
        pic8_harness_log("FAIL: CCPR1L=0x%02X, expected 0x0C\n", (unsigned)cprl);
        return pic8_harness_report(0);
    }
    if (con != 0xACU) {
        pic8_harness_log("FAIL: CCP1CON=0x%02X, expected 0xAC\n", (unsigned)con);
        return pic8_harness_report(0);
    }
    if (del != 0x8CU) {
        pic8_harness_log("FAIL: ECCP1DEL=0x%02X, expected 0x8C\n", (unsigned)del);
        return pic8_harness_report(0);
    }

    /* 5. Run the sim and count TMR2 overflows (one per PWM period). */
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        g_cycle = i + 1;
        pic8_harness_tick();
        if (g_overflows >= EXPECTED_OVERFLOWS) break;
    }

    int32_t delta = (int32_t)g_first_cycle - (int32_t)EXPECTED_PERIOD_CYCLES;
    if (delta < 0) delta = -delta;

    pic8_harness_log("ECCP1 half-bridge PWM: %u periods, first at cycle %u "
                     "(expected ~%u)\n", (unsigned)g_overflows,
                     (unsigned)g_first_cycle, (unsigned)EXPECTED_PERIOD_CYCLES);
    return pic8_harness_report(g_overflows >= EXPECTED_OVERFLOWS && delta <= 4);
}
