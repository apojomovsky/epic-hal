/* Data EEPROM driver smoke test: read (EEADR then EECON1<RD>), the
 * 0x55/0xAA unlock sequence on write, simulated write completion, and
 * buffer round-trips. The data pair sits in Bank 2 (EEDATA 0x10C,
 * EEADR 0x10D) and the control pair in Bank 3 (EECON1 0x18C, EECON2
 * 0x18D, DS39598F Register 3-1), so a write of 0xC3 to 0x10 must leave
 * EEDATA = 0xC3, EEADR = 0x10, EECON2 = 0xAA and EECON1 = 0x06
 * (WREN|WR), with completion on PIR2<EEIF> (Register 2-7). EEPROM size
 * is 128 B on the 16F818 and 256 B on the 16F819 (Table 1-1). */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_eeprom.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* The host register file is indexed by the full datasheet address, bank
 * bits included (pic16f818_819_sim_sfr[0x200]), so the EPIC_BANK2_* and
 * EPIC_BANK3_* writes the driver performs land on PIC_REG_EEDATA and
 * friends with no bank select here. */
/**
 * @brief Smoke-test EEPROM read, the write unlock sequence, simulated
 *        completion and buffer round-trips on the sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();

    /* 1. Read: load a byte at 0x42 first. */
    pic16f818_819_sim_drive_eeprom_byte(0x42U, 0xA5U);
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);
    CHECK(r == 0xA5U, "Read 0x42 returned wrong value");
    CHECK(EPIC_REG8(PIC_REG_EEADR) == 0x42U, "EEADR (0x10D) != 0x42 after read");
    /* RD is set by the read and clears on the next instruction cycle
     * (DS39598F §3.3); the sim backend does not model the auto-clear,
     * so the test only verifies the bit was set. */
    CHECK((EPIC_REG8(PIC_REG_EECON1) & PIC_EECON1_RD) == PIC_EECON1_RD,
          "EECON1<RD> (0x18C) not set after read");

    /* 2. Write: do the unlock sequence. */
    pic16f818_819_sim_reset();
    EPIC_EEPROM_WriteByte(0x10U, 0xC3U);
    CHECK(EPIC_REG8(PIC_REG_EEDATA) == 0xC3U, "EEDATA (0x10C) not 0xC3 after write");
    CHECK(EPIC_REG8(PIC_REG_EEADR) == 0x10U, "EEADR (0x10D) not 0x10 after write");
    CHECK(EPIC_REG8(PIC_REG_EECON2) == 0xAAU, "EECON2 (0x18D) != 0xAA after unlock");
    CHECK(EPIC_REG8(PIC_REG_EECON1) == (PIC_EECON1_WREN | PIC_EECON1_WR),
          "EECON1 (0x18C) != WREN|WR after write start");

    /* 3. Sim completion. */
    pic16f818_819_sim_drive_eeprom_done(0x10U, 0xC3U);
    CHECK(EPIC_EEPROM_IsWriteComplete() == 1U, "EEIF not set after done");
    EPIC_EEPROM_ClearITFlag();
    CHECK(EPIC_EEPROM_IsWriteComplete() == 0U, "EEIF not cleared");

    /* 4. Buffer write. */
    pic16f818_819_sim_reset();
    uint8_t data[3] = { 0x11, 0x22, 0x33 };
    EPIC_StatusTypeDef st = EPIC_EEPROM_WriteBuffer(0x20U, data, 3);
    CHECK(st == EPIC_OK, "WriteBuffer returned error");
    /* Drive the write-completion sim helper for each byte. */
    for (uint8_t i = 0; i < 3; i++)
    {
        pic16f818_819_sim_drive_eeprom_done((uint8_t)(0x20U + i), data[i]);
    }

    /* 5. Buffer read. */
    uint8_t buf[3] = { 0 };
    EPIC_EEPROM_ReadBuffer(0x20U, buf, 3);
    CHECK(buf[0] == 0x11U && buf[1] == 0x22U && buf[2] == 0x33U,
          "Buffer read did not return written values");

    printf("example_eeprom: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
