/*
 * LVD smoke, the §4 gate's host-sim and mdb half for the LVD driver. The
 * shared half checks the LVDCON power-on image and the LVDL/LVDEN
 * readback through Init/Start/Stop: a real trip point cannot be
 * stimulated headless, so the mdb gate asserts control-register readback
 * only. The host half (not built for XC8) drives the modeled supply
 * through IRVST settling and the LVDIF callback.
 */

#include "pic18f6520_hal.h"
#include "pic18f6520_sfr.h"
#include "peripherals/pic18f6520_lvd.h"
#include "core/pic18_irq.h"
#include "core/epic_harness.h"
#ifndef __XC8
#include "pic18f6520_sim.h"
#endif

/** @brief Family-local harness extension: no-op on the host build,
 *  infinite loop on the mdb build (see example_irq.c). */
extern void pic18f6520_harness_halt(void);

/** Simulated run length (host only). */
#define SIM_CYCLES  2000UL

/** Trip code programmed by the test: not the 0x05 POR code, so the write
 * is visible on readback. */
#define TEST_TRIP   0x0CU

/** LVDCON bits the driver owns; IRVST is hardware-owned and excluded. */
#define LVDCON_DRIVER_MASK  (PIC_LVDCON_LVDL_MASK | PIC_LVDCON_LVDEN)

#ifndef __XC8
/* Trip count, the callback is the only writer. */
static volatile uint32_t g_trips = 0;

/** @brief  LVD trip callback: counts the event. */
static void on_trip(void)
{
    g_trips++;
}

/**
 * @brief  Advance simulated time by `n` instruction cycles.
 * @param n the number of cycles to pump.
 */
static void pump(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) epic_harness_tick();
}

/**
 * @brief  Exercise IRVST settling and the trip callback on the host sim.
 * @return 1 if every host-only expectation holds, else 0.
 */
static uint8_t host_trip_checks(void)
{
    uint8_t ok = 1U;

    /* Stable only after the settling time; arming is refused before. */
    if (EPIC_LVD_IsStable()) ok = 0U;
    if (EPIC_LVD_EnableInterrupt() != EPIC_BUSY) ok = 0U;
    pump(200U);
    if (!EPIC_LVD_IsStable()) ok = 0U;
    if (EPIC_LVD_EnableInterrupt() != EPIC_OK) ok = 0U;
    if (!(epic_sfr_read8(PIC_REG_PIE2) & PIC_PIE2_LVDIE)) ok = 0U;

    EPIC_IRQ_SetPriority(PIC18_IRQ_LVD, EPIC_IRQ_PRIORITY_HIGH);
    EPIC_IRQ_Restore(1);

    /* Supply above the trip point: no event. */
    pump(50U);
    if (g_trips != 0U) ok = 0U;

    /* Falling below fires once, and not again while it stays low. */
    pic18_sim_drive_supply_low(1U);
    pump(50U);
    if (g_trips != 1U) ok = 0U;
    if (EPIC_IRQ_GetFlag(PIC18_IRQ_LVD)) ok = 0U;

    /* Recovering and falling again is a second event. */
    pic18_sim_drive_supply_low(0U);
    pump(50U);
    pic18_sim_drive_supply_low(1U);
    pump(50U);
    if (g_trips != 2U) ok = 0U;

    /* Stop drops IRVST and silences the detector. */
    (void)EPIC_LVD_Stop();
    pump(5U);
    if (EPIC_LVD_IsStable()) ok = 0U;
    pic18_sim_drive_supply_low(0U);
    pump(5U);
    pic18_sim_drive_supply_low(1U);
    pump(200U);
    if (g_trips != 2U) ok = 0U;

    return ok;
}
#endif

/**
 * @brief  Check the LVDCON image through Init/Start/Stop, then (host
 *         only) the trip behavior, and report.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);

    /* Power-on image first: Init overwrites it. DS39609B Register 22-1,
     * LVDL = 0101, LVDEN = 0, IRVST = 0. */
    uint8_t por_ok =
        (epic_sfr_read8(PIC_REG_LVDCON) == PIC_LVDCON_POR_VALUE) ? 1U : 0U;

    LVD_HandleTypeDef h = LVD_HANDLE_DEFAULT;
    h.TripLevel = TEST_TRIP;
#ifndef __XC8
    h.TripCallback = on_trip;
#endif

    uint8_t init_ok = 0U, start_ok = 0U, stop_ok = 0U;
    if (EPIC_LVD_Init(&h) == EPIC_OK)
    {
        init_ok = ((epic_sfr_read8(PIC_REG_LVDCON) & LVDCON_DRIVER_MASK)
                   == TEST_TRIP) ? 1U : 0U;
    }
    if (EPIC_LVD_Start(&h) == EPIC_OK)
    {
        start_ok = ((epic_sfr_read8(PIC_REG_LVDCON) & LVDCON_DRIVER_MASK)
                    == (TEST_TRIP | PIC_LVDCON_LVDEN)) ? 1U : 0U;
    }

    uint8_t host_ok = 1U;
#ifndef __XC8
    host_ok = host_trip_checks();
#endif

    /* Stop clears LVDEN and keeps the trip code. */
    if (EPIC_LVD_Stop() == EPIC_OK)
    {
        stop_ok = ((epic_sfr_read8(PIC_REG_LVDCON) & LVDCON_DRIVER_MASK)
                   == TEST_TRIP) ? 1U : 0U;
    }

    epic_harness_log("LVD: por=%u init=%u start=%u stop=%u host=%u\n",
                     (unsigned)por_ok, (unsigned)init_ok,
                     (unsigned)start_ok, (unsigned)stop_ok,
                     (unsigned)host_ok);
    int rc = epic_harness_report(por_ok && init_ok && start_ok && stop_ok &&
                                 host_ok);

    /* Freeze so RA0 keeps the report value across the mdb readback. */
    pic18f6520_harness_halt();
    return rc;
}
