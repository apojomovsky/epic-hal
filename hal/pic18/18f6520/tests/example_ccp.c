/*
 * CCP compare smoke: program CCP1 and CCP3 in compare mode against
 * Timer1, verify the driver programs CCPxCON/CCPRx correctly and
 * Timer1 runs as the time base. The compare-match flag (CCPxIF) is
 * hardware; the mdb gate proves it. Host sim verifies register
 * programming.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_ccp.h"
#include "peripherals/pic18f6520_timer1.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  5000UL

/**
 * @brief  Program CCP1/CCP3 compare mode + verify registers, run Timer1.
 *
 *          PASS if CCPxCON mode programs to compare-toggle, CCPRx reads
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

    /* CCP1: compare-toggle on 0x1000, with event callback (arms CCP1IE). */
    CCP_HandleTypeDef ch = CCP_HANDLE_DEFAULT;
    ch.Instance     = CCP_INSTANCE_1;
    ch.Mode         = CCP_MODE_COMPARE_TOGGLE;
    ch.CompareValue = 0x1000U;
    if (EPIC_CCP_Init(&ch) != EPIC_OK) ok = 0U;

    /* CCP3: compare-set on 0x0300 (instance 3 of the five plain CCPs). */
    CCP_HandleTypeDef ch3 = CCP_HANDLE_DEFAULT;
    ch3.Instance     = CCP_INSTANCE_3;
    ch3.Mode         = CCP_MODE_COMPARE_SET;
    ch3.CompareValue = 0x0300U;
    if (EPIC_CCP_Init(&ch3) != EPIC_OK) ok = 0U;

    /* Verify CCP1CON mode field programmed to compare-toggle (0010). */
    uint8_t con = epic_sfr_read8(PIC_REG_CCP1CON);
    if ((con & PIC_CCPxCON_CCPxM_MASK) != CCP_MODE_COMPARE_TOGGLE) ok = 0U;

    /* Verify CCPR1 reads back the compare value. */
    if (EPIC_CCP_GetCapture(CCP_INSTANCE_1) != 0x1000U) ok = 0U;

    /* Verify CCP3CON mode + CCPR3 (compare-set, 0x0300). */
    uint8_t con3 = epic_sfr_read8(PIC_REG_CCP3CON);
    if ((con3 & PIC_CCPxCON_CCPxM_MASK) != CCP_MODE_COMPARE_SET) ok = 0U;
    if (EPIC_CCP_GetCapture(CCP_INSTANCE_3) != 0x0300U) ok = 0U;

    EPIC_IRQ_Restore(1);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    /* Timer1 (time base) must have advanced. */
    if (EPIC_TIMER1_ReadCounter() == 0U) ok = 0U;

    epic_harness_log("CCP1/CCP3 compare programmed, Timer1=%u.\n",
                     (unsigned)EPIC_TIMER1_ReadCounter());
    return epic_harness_report(ok);
}
