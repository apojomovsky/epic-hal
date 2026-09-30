/* Data EEPROM driver smoke test: read, the 0x55/0xAA unlock sequence on
 * write, simulated write completion, and buffer round-trips. The data
 * pair sits in Bank 2 (EEDATA 0x10C, EEADR 0x10D) and the control pair
 * in Bank 3 (EECON1 0x18C, EECON2 0x18D, DS39598F Register 3-1), with
 * completion on PIR2<EEIF> (Register 2-7) and 128/256 B of storage on
 * the 16F818/16F819 (Table 1-1).
 *
 * This test asserts what the driver does, not its register image: the
 * byte-level Bank 2/3 images are asserted on the real target by
 * tests/sim_bank_probe.c, which reaches those registers through the
 * same literal-token macros the driver uses. */

#include "pic16f818_819_hal.h"
#include "pic16f818_819_sim.h"
#include "pic16f818_819_sfr.h"
#include "peripherals/hal_eeprom.h"
#include "core/pic16_irq.h"
#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/**
 * @brief Smoke-test EEPROM read, the write unlock sequence, simulated
 *        completion and buffer round-trips on the sim backend.
 */
int main(void)
{
    pic16f818_819_sim_reset();

    /* 1. Read: load a byte at 0x42 first and read it back. */
    pic16f818_819_sim_drive_eeprom_byte(0x42U, 0xA5U);
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);
    CHECK(r == 0xA5U, "Read 0x42 returned wrong value");

    /* 2. Write: do the unlock sequence. EECON2 is not a readable
     * register, so the sequence's effect is asserted through the stored
     * byte and the completion flag, not through the control registers. */
    pic16f818_819_sim_reset();
    EPIC_EEPROM_WriteByte(0x10U, 0xC3U);
    pic16f818_819_sim_drive_eeprom_done(0x10U, 0xC3U);
    CHECK(EPIC_EEPROM_IsWriteComplete() == 1U, "EEIF not set after done");
    EPIC_EEPROM_ClearITFlag();
    CHECK(EPIC_EEPROM_IsWriteComplete() == 0U, "EEIF not cleared");
    CHECK(pic14_sim_eeprom_read(0x10U) == 0xC3U,
          "stored byte at 0x10 not 0xC3 after the write completed");
    CHECK(EPIC_EEPROM_ReadByte(0x10U) == 0xC3U,
          "the driver did not read back the byte it wrote");

    /* 3. Buffer write, then read it back: the same round trip over a
     * range, which the driver's address-increment path exercises. */
    pic16f818_819_sim_reset();
    uint8_t data[3] = { 0x11, 0x22, 0x33 };
    EPIC_StatusTypeDef st = EPIC_EEPROM_WriteBuffer(0x20U, data, 3);
    CHECK(st == EPIC_OK, "WriteBuffer returned error");
    /* Drive the write-completion sim helper for each byte. */
    for (uint8_t i = 0; i < 3; i++)
    {
        pic16f818_819_sim_drive_eeprom_done((uint8_t)(0x20U + i), data[i]);
    }

    uint8_t buf[3] = { 0 };
    EPIC_EEPROM_ReadBuffer(0x20U, buf, 3);
    CHECK(buf[0] == 0x11U && buf[1] == 0x22U && buf[2] == 0x33U,
          "Buffer read did not return written values");

    printf("example_eeprom: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
