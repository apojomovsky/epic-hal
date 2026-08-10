/**
 * @file    sim_settings.c
 * @brief   Bounded, self-reporting HARNESS=sim build: epic-settings'
 *          first-ever real `mdb` gate. Saves a known settings blob
 *          through the real compiled epic_settings.c, reads it back
 *          and verifies both the payload and the CRC-16 validation
 *          result, corrupts one stored byte via a direct HAL EEPROM
 *          write, and verifies the next load detects the corruption
 *          and falls back to (and persists) defaults. Reports
 *          PASS/FAIL over the target's real hardware USART the same
 *          way every other family's own `.sim` variant does (see
 *          pic18fxx5x-hal/src/core/pic18_harness_sim_target.c).
 *
 * @details
 *   The module is stateless (a blob is just (eeprom_addr, size, data)
 *   passed explicitly, see epic_settings.h), so "reset" is a fresh
 *   read of the stored region: after the save, every subsequent load
 *   re-reads EEPROM from scratch, exactly the path a power-cycle
 *   would take.
 *
 *   EEPROM is real device state under MPLAB SIM (the 18F4550's 256
 *   bytes are part of the simulated die), so the write/readback here
 *   exercises the actual EEPROM hardware path, not a host array:
 *   `epic_settings_save` sequences byte writes through
 *   EPIC_EEPROM_WriteByte and waits on EEIF, and the corruption is a
 *   direct EPIC_EEPROM_WriteByte through the HAL's literal-token
 *   path. No MPLAB SIM RX injection needed (same constraint
 *   documented in epic-swuart's sim build): the gate is entirely
 *   TX-of-record / internal-state.
 *
 *   One MPLAB SIM limitation this gate works around, verified by hand
 *   against both PIC18F4550 and PIC16F877A (MPLAB X 6.35): the
 *   simulator never completes a CPU-executed data-EEPROM write cycle.
 *   After the firmware sets EECON1<WR>, the simulator holds WR set
 *   forever and never raises EEIF, so any firmware that blocks on
 *   EEIF hangs (the EEPROM *read* path, RD strobe into EEDATA, works,
 *   and mdb `write` commands do complete writes). The gate runner
 *   (scripts/sim-mdb-run.sh, EEPROM_WRITES=24) compensates by halting
 *   the firmware at each stalled write, clearing WR, replaying the
 *   EECON2 unlock, re-asserting WR, and resuming; the simulator then
 *   completes the write from the firmware's own EEDATA/EEADR. This
 *   scenario performs exactly 19 EEPROM writes (6 payload+CRC bytes of
 *   the first save, 1 corruption byte, 6 of the persisted-default
 *   save, 6 of the blank-region default save); keep EEPROM_WRITES >=
 *   19 if the scenario below changes.
 *
 *   Distinct from `tests/test_settings.c` (the host unit tests over
 *   the pic18fxx5x sim backend) and `mcu/target_sizecheck.c` (the
 *   real-target footprint build, an unbounded loop with no harness
 *   dependency).
 */
#include "epic_settings.h"
#include "core/epic_harness.h"
#include "epic_hal.h"

#include <string.h>

/** Loop-iteration bound, not a real time unit (see core/epic_harness.h).
 *  On target `epic_harness_tick` is a no-op, so this loop only paces
 *  the run; 1000 iterations is a rounding error next to the UART
 *  report, comfortably inside the 5000 ms wait_ms budget. */
#define SIM_ITERATIONS 1000UL

/* 4-byte payload + 2-byte CRC-16 trailer = 6 bytes of EEPROM. All
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

    /* NEVER return: on a real target the firmware runs forever, and
     * under MPLAB SIM a return would fall through to the reset vector
     * and RE-RUN the sequence. The checks are deliberately stateful
     * (pass 1 corrupts a stored byte), so a re-run would report the
     * corruption as a failure. An idle loop keeps the gate single-pass
     * no matter how long the runner lets the target run. */
    for (;;) {
        epic_harness_tick();
    }
}
