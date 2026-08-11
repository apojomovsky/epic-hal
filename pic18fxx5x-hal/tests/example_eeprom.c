/*
 * Data EEPROM driver smoke test on the PIC18 host sim: read (EEADR + RD
 * strobe), write (the 0x55/0xAA unlock sequence, DS39632E §7.0),
 * sim-driven write completion via EEIF, and a buffer round-trip.
 */

#include "epic_hal.h"
#include "core/epic_harness.h"
#include "pic18fxx5x_sim.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { epic_harness_log("FAIL: %s\n", msg); return epic_harness_report(0); } \
} while (0)

/** @brief  Data EEPROM driver smoke test.
 *
 *          Verifies read (EEADR + RD strobe), the 0x55/0xAA write unlock,
 *          sim-driven write completion via EEIF, and a buffer round-trip.
 */
int main(void)
{
    epic_harness_init(16U);
    pic18_sim_set_irq_callback(NULL);

    /* 1. Read: preload a byte at 0x42, then read it back. */
    pic18_sim_drive_eeprom_byte(0x42U, 0xA5U);
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);
    CHECK(r == 0xA5U, "Read 0x42 returned wrong value");
    CHECK(epic_sfr_read8(PIC_REG_EEADR) == 0x42U, "EEADR != 0x42 after read");
    CHECK((epic_sfr_read8(PIC_REG_EECON1) & PIC_EECON1_RD) != 0U,
          "RD not set after read");
    CHECK(epic_sfr_read8(PIC_REG_EEDATA) == 0xA5U, "EEDATA != 0xA5 after read");

    /* 2. Write: do the unlock sequence. */
    EPIC_EEPROM_WriteByte(0x10U, 0xC3U);
    CHECK(epic_sfr_read8(PIC_REG_EEDATA) == 0xC3U, "EEDATA != 0xC3 after write");
    CHECK(epic_sfr_read8(PIC_REG_EEADR) == 0x10U, "EEADR != 0x10 after write");
    /* The unlock writes 0x55 then 0xAA to EECON2; the last is 0xAA. */
    CHECK(epic_sfr_read8(PIC_REG_EECON2) == 0xAAU, "EECON2 != 0xAA after unlock");

    /* 3. Sim completion -> EEIF set, then clear. */
    pic18_sim_drive_eeprom_done(0x10U, 0xC3U);
    CHECK(EPIC_EEPROM_IsWriteComplete() == 1U, "EEIF not set after done");
    EPIC_EEPROM_ClearITFlag();
    CHECK(EPIC_EEPROM_IsWriteComplete() == 0U, "EEIF not cleared");

    /* 4. Buffer round-trip. */
    static uint8_t data[3] = { 0x11U, 0x22U, 0x33U };
    EPIC_StatusTypeDef st = EPIC_EEPROM_WriteBuffer(0x20U, data, 3);
    CHECK(st == EPIC_OK, "WriteBuffer returned error");
    for (uint8_t i = 0; i < 3; i++) {
        pic18_sim_drive_eeprom_done((uint8_t)(0x20U + i), data[i]);
    }
    uint8_t buf[3] = { 0 };
    EPIC_EEPROM_ReadBuffer(0x20U, buf, 3);
    CHECK(buf[0] == 0x11U && buf[1] == 0x22U && buf[2] == 0x33U,
          "Buffer read did not return written values");

    epic_harness_log("OK: EEPROM driver, read, write unlock, completion, buffer round-trip.\n");
    return epic_harness_report(1);
}
