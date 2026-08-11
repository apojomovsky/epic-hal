/**
 * Bounded, self-reporting HARNESS=sim build (epic-settings' `mdb` gate):
 * saves a known blob through the real compiled epic_settings.c, verifies
 * payload and CRC on load, corrupts one stored byte via a direct HAL EEPROM
 * write, verifies the next load detects it and falls back to (and persists)
 * defaults. Reports PASS/FAIL over the harness USART (see
 * pic18_harness_sim_target.c).
 *
 * EEPROM is real device state under MPLAB SIM, so the actual hardware path
 * is exercised. Simulator limitation: a CPU-executed EEPROM write never
 * completes (WR held set, EEIF never raised), so scripts/sim-mdb-run.sh
 * (EEPROM_WRITES=24) halts at each stalled write, replays the EECON2
 * unlock, and resumes. This scenario makes exactly 19 writes; keep
 * EEPROM_WRITES >= 19 if the scenario changes.
 *
 * Distinct from tests/test_settings.c (host unit tests over the sim
 * backend) and mcu/target_sizecheck.c (real-target footprint build).
 */
#include "epic_settings.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#include <string.h>

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h);
 *  on target `epic_harness_tick` is a no-op, so this only paces the run. */
#define SIM_ITERATIONS 1000UL

/* 4-byte payload + 2-byte CRC-16 trailer = 6 bytes of EEPROM; all
 * uint8_t members so the layout is explicit on the target. */
typedef struct {
    uint8_t mode;
    uint8_t limit_lo;
    uint8_t limit_hi;
    uint8_t flags;
} settings_blob_t;

#define BLOB_ADDR    0x10u   /* primary blob region */
#define DEFAULT_ADDR 0x40u   /* blank-region fallback check */

static const settings_blob_t g_saved    = { 2u, 0x77u, 0x01u, 0x05u };
static const settings_blob_t g_defaults = { 1u, 0xFAu, 0x00u, 0x03u };

/**
 * @brief Direct byte write through the HAL.
 *
 * Mirrors the module's own write path (start, wait for EEIF, clear flag)
 * so the corruption is a real EEPROM write, not a register poke.
 */
static void eeprom_write_byte(uint8_t addr, uint8_t data)
{
    (void)EPIC_EEPROM_WriteByte(addr, data);
    while (EPIC_EEPROM_IsWriteComplete() == 0u) {
        epic_harness_tick();
    }
    EPIC_EEPROM_ClearITFlag();
}

/**
 * @brief Compare two settings blobs byte-for-byte.
 */
static int blob_eq(const settings_blob_t *a, const settings_blob_t *b)
{
    return memcmp(a, b, sizeof(settings_blob_t)) == 0;
}

/**
 * @brief Run the sim: save, load, corrupt, and fall back to defaults,
 *        reporting PASS/FAIL over the harness USART.
 */
int main(void)
{
    epic_harness_init(SIM_ITERATIONS);

    settings_blob_t out;
    int ok = 1;

    /* (a) write a known blob through the real API. */
    if (!epic_settings_save(BLOB_ADDR, &g_saved, (uint8_t)sizeof(g_saved))) {
        epic_harness_log("settings sim: save failed\n");
        ok = 0;
    }

    /* (b)+(c) reset (fresh read) and verify both the payload and the
     * CRC validation result (the load return value). */
    memset(&out, 0, sizeof(out));
    if (!epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
        epic_harness_log("settings sim: load rejected valid blob\n");
        ok = 0;
    }
    if (!blob_eq(&out, &g_saved)) {
        epic_harness_log("settings sim: roundtrip mismatch\n");
        ok = 0;
    }

    /* (d) corrupt one stored payload byte via a direct HAL EEPROM
     * write, then verify the load detects it (CRC mismatch) and
     * leaves the output buffer untouched on failure. */
    eeprom_write_byte((uint8_t)(BLOB_ADDR + 1u), (uint8_t)~g_saved.limit_lo);
    {
        settings_blob_t untouched = out;
        if (epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
            epic_harness_log("settings sim: corruption NOT detected\n");
            ok = 0;
        }
        if (!blob_eq(&out, &untouched)) {
            epic_harness_log("settings sim: failed load overwrote output\n");
            ok = 0;
        }
    }

    /* Fall back to defaults, persist them, and confirm the next boot
     * reads the persisted default blob as valid. */
    if (epic_settings_load_or_default(BLOB_ADDR, &out, (uint8_t)sizeof(out),
                                      &g_defaults)) {
        epic_harness_log("settings sim: corrupt region did not default\n");
        ok = 0;
    }
    if (!blob_eq(&out, &g_defaults)) {
        epic_harness_log("settings sim: defaults not applied\n");
        ok = 0;
    }
    if (!epic_settings_load(BLOB_ADDR, &out, (uint8_t)sizeof(out))) {
        epic_harness_log("settings sim: persisted defaults unreadable\n");
        ok = 0;
    }
    if (!blob_eq(&out, &g_defaults)) {
        epic_harness_log("settings sim: persisted defaults mismatch\n");
        ok = 0;
    }

    /* A never-written region still reports invalid and yields the
     * caller's defaults (blank and corrupt are both "apply
     * defaults", see epic_settings.h). */
    if (epic_settings_load_or_default(DEFAULT_ADDR, &out,
                                      (uint8_t)sizeof(out), &g_defaults)) {
        epic_harness_log("settings sim: blank region reported valid\n");
        ok = 0;
    }

    for (uint32_t i = 0; epic_harness_running(i); i++) {
        epic_harness_tick();
    }

    epic_harness_log(ok ? "settings sim: save/load/corrupt/default sequence ok\n"
                        : "settings sim: sequence failed\n");
    (void)epic_harness_report(ok);

    /* NEVER return: under MPLAB SIM a return re-runs main, and pass 1
     * corrupts a stored byte, so a re-run would report a false failure.
     * Idle instead, keeping the gate single-pass. */
    for (;;) {
        epic_harness_tick();
    }
}
