/*
 * ECCP compare smoke: program ECCP1 in compare mode against Timer1,
 * verify the driver programs CCP1CON/CCPR1 correctly and Timer1 runs
 * as the time base. The compare-match flag (CCP1IF) is hardware; the
 * mdb gate proves it. Host sim verifies register programming.
 */

#include "pic18f1320.h"
#include "pic18f1320_sfr.h"
#include "peripherals/pic18f1320_eccp.h"
#include "peripherals/pic18f1320_timer1.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  5000UL

/* Event count, the ISR is the only writer (no match on host sim,
 * which does not model CCP compare; stays 0 here). */
static volatile uint32_t g_event_count = 0;

/** @brief  CCP1 event callback (fires on CCP1IF on target). */
static void on_ccp1_event(void)
{
    g_event_count++;
}

/**
 * @brief  Program ECCP1 compare mode + verify registers, run Timer1.
 *
 *          PASS if CCP1CON mode programs to compare-toggle, CCPR1 reads
 *          back the compare value, and Timer1 advances (time base runs).
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    uint8_t ok = 1U;

    /* Timer1 as the CCP time base: 16-bit, internal, 1:1, from 0. */
    TIMER1_HandleTypeDef th = TIMER1_HANDLE_DEFAULT;
    th.ClockSource = TIMER1_CLOCK_INTERNAL;
    th.Prescaler   = TIMER1_PRESCALER_1_1;
    th.ReloadValue = 0x0000U;
    EPIC_TIMER1_Init(&th);
    EPIC_TIMER1_Start(&th);

    /* ECCP1: compare-toggle on 0x1000, with event callback (arms CCP1IE). */
    CCP_HandleTypeDef ch;
    ch.Instance            = CCP_INSTANCE_1;
    ch.Mode                = CCP_MODE_COMPARE_TOGGLE;
    ch.CompareValue        = 0x1000U;
    ch.PWM.Period          = 0U;
    ch.PWM.Duty            = 0U;
    ch.PWMOutputMode       = CCP_PWM_OUTPUT_SINGLE;
    ch.DeadBand.Delay      = 0U;
    ch.DeadBand.AutoRestart = false;
    ch.AutoShutdown.Source = CCP_AUTOSHUTDOWN_DISABLED;
    ch.AutoShutdown.PinsAC = CCP_SHUTDOWN_DRIVE_0;
    ch.AutoShutdown.PinsBD = CCP_SHUTDOWN_DRIVE_0;
    ch.EventCallback       = on_ccp1_event;
    if (EPIC_CCP_Init(&ch) != EPIC_OK) ok = 0U;

    /* Verify CCP1CON mode field programmed to compare-toggle (0010). */
    uint8_t con = epic_sfr_read8(PIC_REG_CCP1CON);
    if ((con & PIC_CCP1_M_MASK) != CCP_MODE_COMPARE_TOGGLE) ok = 0U;

    /* Verify CCPR1 reads back the compare value. */
    if (EPIC_CCP_GetCapture(CCP_INSTANCE_1) != 0x1000U) ok = 0U;

    /* Verify CCP1IE armed (callback given). */
    if (!(epic_sfr_read8(PIC_REG_PIE1) & PIC_PIE1_CCP1IE)) ok = 0U;

    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    /* Timer1 (time base) must have advanced. */
    if (EPIC_TIMER1_ReadCounter() == 0U) ok = 0U;

    epic_harness_log("CCP1 compare programmed, Timer1=%u.\n",
                     (unsigned)EPIC_TIMER1_ReadCounter());
    return epic_harness_report(ok);
}
