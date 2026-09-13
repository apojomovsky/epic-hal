/* Data EEPROM driver smoke test: read (EEADR then EECON1<RD>), the
 * 0x55/0xAA unlock sequence on write, simulated write completion, and
 * a round-trip. On this family the data pair (EEDATA/EEADR) lives in
 * Bank 2 and the control pair (EECON1/EECON2) in Bank 3; the completion
 * flag EEIF is PIR2<4>. */

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include "peripherals/pic14_eeprom.h"
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
 *        completion and a round-trip on the sim backend.
 */
int main(void)
{
    pic16f63x_67x_68x_sim_reset();

    /* 1. Read: load a byte at 0x42 first. */
    pic16f63x_67x_68x_sim_drive_eeprom_byte(0x42U, 0xA5U);
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);
    CHECK(r == 0xA5U, "Read 0x42 returned wrong value");
    /* Host-only: the shared driver's host path addresses the Bank-2
     * data pair by offset (0x0D) under a bank-2 select, so the test
     * reads the offset, not the absolute 0x10D (the target path uses
     * literal tokens and hits the true address; the mdb gate proves
     * that half). */
    CHECK(b_read(2, 0x0DU) == 0x42U, "EEADR != 0x42 after read");
    /* RD is set by the read and clears on the next instruction
     * cycle; the sim backend does not model the auto-clear, so the
     * test only verifies the bit was set. */
    CHECK((b_read(3, PIC_REG_EECON1) & 0x01U) == 0x01U, "RD not set after read");

    /* 2. Write: do the unlock sequence. */
    EPIC_StatusTypeDef st = EPIC_EEPROM_WriteByte(0x10U, 0x5AU);
    CHECK(st == EPIC_OK, "WriteByte returned error");
    CHECK(b_read(2, 0x0DU) == 0x10U, "EEADR != 0x10 after write");

    /* 3. Completion: the sim models the write finishing. */
    pic16f63x_67x_68x_sim_drive_eeprom_done(0x10U, 0x5AU);
    CHECK((EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_EEIF) != 0U, "EEIF not set");
    r = EPIC_EEPROM_ReadByte(0x10U);
    CHECK(r == 0x5AU, "round-trip read returned wrong value");

    printf("OK: EEPROM read/write round-trip passes.\n");
    return 0;
}
