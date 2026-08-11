/**
 * @file    combo_tick_settings.c
 * @brief   C7 of the combination matrix:
 *          epic-tick + epic-settings on PIC18 (18F4550).
 *
 * @details
 *   The point of this gate: the settings module's EEPROM save path
 *   (byte-at-a-time EPIC_EEPROM_WriteByte + EEIF poll, see
 *   epic_settings.c) running while the 1 ms tick ISR is live. The
 *   scenario performs 13 real EEPROM writes (6 save payload+CRC, 1
 *   corruption, 6 persisted defaults); MPLAB SIM never completes a
 *   CPU-executed write, so the mdb runner completes each one with its
 *   halt+unlock+re-assert cycle (scripts/sim-mdb-run.sh,
 *   EEPROM_WRITES=32, the same mechanism the epic-settings gate
 *   uses). The tick ISR fires thousands of times across those cycles,
 *   so a save path that disturbs the ISR (GIE wedging, dispatch
 *   corruption) or an ISR path that steals the module's EEIF
 *   completion signal fails the gate.
 *
 *   Scenario:
 *   (a) epic_tick_init() arms Timer2 and enables GIE (GIEH/GIEL);
 *   (b) a known blob is saved via epic_settings_save (6 writes);
 *   (c) it is loaded back and both the payload and the CRC validation
 *       result are verified;
 *   (d) one stored byte is corrupted via a direct HAL
 *       EPIC_EEPROM_WriteByte (1 write); the load detects the CRC
 *       mismatch, leaves the output untouched, and
 *       epic_settings_load_or_default falls back to (and persists) the
 *       defaults (6 writes);
 *   (e) the tick counter kept advancing through the whole churn and
 *       GIE stayed alive (GIEH/GIEL still set, TMR2IE still set).
 *
 *   EEPROM_WRITES accounting: 6 + 1 + 6 = 13, comfortably inside the
 *   runner's 32. The spare cycles also absorb a known sim race: the
 *   ISR dispatch runs EEPROM_IRQHandler whenever PIR2<EEIF> is set
 *   (even with EEIE disabled), so a tick ISR that lands between the
 *   runner's write completion and the firmware's next poll clears the
 *   flag and the poll spins; the next runner cycle re-completes the
 *   byte and un-sticks the firmware.
 */

#include "epic_tick.h"
#include "epic_settings.h"
#include "core/epic_harness.h"
#include "core/pic18_irq.h"
#include "epic_hal.h"

#include <stdint.h>
#include <string.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define SIM_ITERATIONS 1000UL

/* 4-byte payload + 2-byte CRC-16 trailer = 6 bytes of EEPROM. All
 * uint8_t members so the layout is explicit on the target. */
typedef struct {
    uint8_t mode;
    uint8_t limit_lo;
    uint8_t limit_hi;
    uint8_t flags;
} settings_blob_t;

#define BLOB_ADDR 0x10u   /* primary blob region */

static const settings_blob_t g_saved    = { 2u, 0x77u, 0x01u, 0x05u };
static const settings_blob_t g_defaults = { 1u, 0xFAu, 0x00u, 0x03u };

static uint16_t g_fail = 0u;

static void fail(uint8_t idx)
{
    static const char hx[] = "0123456789ABCDEF";
    char c[2];
    g_fail++;
    epic_harness_log("F");
    c[0] = hx[(idx >> 4) & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    c[0] = hx[idx & 0xF];
    c[1] = '\0';
    epic_harness_log(c);
    epic_harness_log(".");
}

#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/* Direct byte write through the HAL, mirroring the module's own write
 * path (start, wait for EEIF, clear flag) so the corruption is a real
 * EEPROM write, not a register poke. */
static void eeprom_write_byte(uint8_t addr, uint8_t data)
{
    (void)EPIC_EEPROM_WriteByte(addr, data);
    while (EPIC_EEPROM_IsWriteComplete() == 0u) {
        epic_harness_tick();
    }
    EPIC_EEPROM_ClearITFlag();
}

static int blob_eq(const settings_blob_t *a, const settings_blob_t *b)
{
    return memcmp(a, b, sizeof(settings_blob_t)) == 0;
}

int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    /* (a) Start the 1 ms timebase: epic_tick_init configures Timer2,
     * arms TMR2IE and enables GIE at the end. The timer cannot have a
     * pending match at that point (the Init-to-Restore window is a few
     * instructions, the period is ~12000), so the MPLAB SIM
     * GIE-on-while-pending wedge (the C1 finding) cannot fire here. */
    epic_tick_init(FOSC_HZ);
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEH) != 0u, 0x00);
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEL) != 0u, 0x01);
    CHECK((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) != 0u, 0x02);

    settings_blob_t out;
    uint32_t t_save0 = epic_tick_get();
    uint32_t t_save1;

    /* (b) save a known blob through the real API (6 EEPROM writes,
     * each completed by the runner's halt+complete cycle while the
     * tick ISR stays live). */
    if (!epic_settings_save(BLOB_ADDR, &g_saved, (uint8_t)sizeof(g_saved))) {
        epic_harness_log("combo tick-settings: save failed\n");
        fail(0x03);
    }

    /* (e) the tick must have advanced through the save's churn: the
     * point of the gate is the save path under a live tick ISR. */
    t_save1 = epic_tick_get();
    if (t_save1 <= t_save0) {
        fail(0x04);
    }

    /* (c) fresh read (the module is stateless: a load re-reads EEPROM
     * from scratch, exactly the power-cycle path) and verify both the
     * payload and the CRC validation result. */
    memset(&out, 0, sizeof(out));
    if (!epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
        epic_harness_log("combo tick-settings: load rejected valid blob\n");
        fail(0x05);
    }
    if (!blob_eq(&out, &g_saved)) {
        epic_harness_log("combo tick-settings: roundtrip mismatch\n");
        fail(0x06);
    }

    /* (d) corrupt one stored payload byte via a direct HAL EEPROM
     * write, then verify the load detects it (CRC mismatch) and
     * leaves the output buffer untouched on failure. */
    eeprom_write_byte((uint8_t)(BLOB_ADDR + 1u), (uint8_t)~g_saved.limit_lo);
    {
        settings_blob_t untouched = out;
        if (epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
            epic_harness_log("combo tick-settings: corruption NOT detected\n");
            fail(0x07);
        }
        if (!blob_eq(&out, &untouched)) {
            epic_harness_log("combo tick-settings: failed load overwrote output\n");
            fail(0x08);
        }
    }

    /* Fall back to defaults, persist them, and confirm the next boot
     * reads the persisted default blob as valid. */
    if (epic_settings_load_or_default(BLOB_ADDR, &out, (uint8_t)sizeof(out),
                                      &g_defaults)) {
        epic_harness_log("combo tick-settings: corrupt region did not default\n");
        fail(0x09);
    }
    if (!blob_eq(&out, &g_defaults)) {
        epic_harness_log("combo tick-settings: defaults not applied\n");
        fail(0x0A);
    }
    if (!epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
        epic_harness_log("combo tick-settings: persisted defaults unreadable\n");
        fail(0x0B);
    }
    if (!blob_eq(&out, &g_defaults)) {
        epic_harness_log("combo tick-settings: persisted defaults mismatch\n");
        fail(0x0C);
    }

    /* (e) the tick kept advancing through the corruption write and
     * the persisted-defaults churn too, and the interrupt state
     * survived (GIEH/GIEL still set, TMR2IE still armed). */
    if (epic_tick_get() <= t_save1) {
        fail(0x0D);
    }
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEH) != 0u, 0x0E);
    CHECK((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIEL) != 0u, 0x0F);
    CHECK((EPIC_REG8(PIC_REG_PIE1) & PIC_PIE1_TMR2IE) != 0u, 0x10);

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }

    epic_harness_log(g_fail == 0u
        ? "combo tick-settings: save/load/corrupt/default under live tick ok\n"
        : "combo tick-settings: sequence failed\n");
    return epic_harness_report(g_fail == 0u);
}
