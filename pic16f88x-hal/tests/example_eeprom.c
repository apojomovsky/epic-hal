/* Data EEPROM driver smoke test: read (EEADR then EECON1<RD>), the
 * 0x55/0xAA unlock sequence on write, simulated write completion, and
 * buffer round-trips. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_eeprom.h"
#include "core/pic16_irq.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/* Helper: read a register from a non-default bank. */
/**
 * @brief Read a register from a non-default bank.
 * @param bank the bank to select (0..3).
 * @param addr the register address in that bank.
 * @return the register value.
 */
static uint8_t b_read(uint8_t bank, uint16_t addr)
{
    uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(bank);
    uint8_t v = EPIC_REG8(addr);
    pic_select_bank(prev);
    return v;
}

/**
 * @brief Smoke-test EEPROM read, the write unlock sequence, simulated
 *        completion and buffer round-trips on the sim backend.
 */
int main(void)
{
    pic16f88x_sim_reset();

    /* 1. Read: load a byte at 0x42 first. */
    pic16f88x_sim_drive_eeprom_byte(0x42U, 0xA5U);
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);
    CHECK(r == 0xA5U, "Read 0x42 returned wrong value");
    CHECK(b_read(2, 0x0DU) == 0x42U, "EEADR != 0x42 after read");
    /* RD is set by the read and clears on the next instruction
     * cycle (DS40001291H §10.4); the sim backend does not model the
     * auto-clear, so the test only verifies the bit was set. */
    CHECK((b_read(3, 0x18CU) & 0x01U) == 0x01U, "RD not set after read");

    /* 2. Write: do the unlock sequence. */
    pic16f88x_sim_reset();
    EPIC_EEPROM_WriteByte(0x10U, 0xC3U);
    /* Verify EEDATA + EEADR. */
    CHECK(b_read(2, 0x0CU) == 0xC3U, "EEDATA not 0xC3 after write");
    CHECK(b_read(2, 0x0DU) == 0x10U, "EEADR not 0x10 after write");
    /* Verify unlock sequence: EECON2 was 0xAA last. */
    CHECK(b_read(3, 0x18DU) == 0xAAU, "EECON2 != 0xAA after unlock");

    /* 3. Sim completion. */
    pic16f88x_sim_drive_eeprom_done(0x10U, 0xC3U);
    CHECK(EPIC_EEPROM_IsWriteComplete() == 1U, "EEIF not set after done");
    EPIC_EEPROM_ClearITFlag();
    CHECK(EPIC_EEPROM_IsWriteComplete() == 0U, "EEIF not cleared");

    /* 4. Buffer write. */
    pic16f88x_sim_reset();
    uint8_t data[3] = { 0x11, 0x22, 0x33 };
    EPIC_StatusTypeDef st = EPIC_EEPROM_WriteBuffer(0x20U, data, 3);
    CHECK(st == EPIC_OK, "WriteBuffer returned error");
    /* Drive the write-completion sim helper for each byte. */
    for (uint8_t i = 0; i < 3; i++) {
        pic16f88x_sim_drive_eeprom_done((uint8_t)(0x20U + i), data[i]);
    }

    /* 5. Buffer read. */
    uint8_t buf[3] = { 0 };
    EPIC_EEPROM_ReadBuffer(0x20U, buf, 3);
    CHECK(buf[0] == 0x11U && buf[1] == 0x22U && buf[2] == 0x33U,
          "Buffer read did not return written values");

    printf("OK: EEPROM driver, read, write unlock sequence, buffer round-trip.\n");
    return 0;
}
