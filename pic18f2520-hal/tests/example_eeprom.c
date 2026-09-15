/*
 * Data EEPROM smoke: write a byte and read it back. Host sim models the
 * EEPROM cell array; mdb gate proves unlock + WR + EEIF on hardware.
 */

#include "pic18f2520_hal.h"
#include "pic18f2520_sfr.h"
#include "peripherals/pic18f2520_eeprom.h"
#include "pic18f2520_sim.h"
#include "core/epic_harness.h"

/** Simulated run length (host only). */
#define SIM_CYCLES  1000UL

/**
 * @brief  Write 0xA5 to address 0x42, read back, compare.
 *
 *          PASS if the readback matches.
 */
int main(void)
{
    epic_harness_init(SIM_CYCLES);
    EPIC_EEPROM_Init(NULL);
    EPIC_EEPROM_WriteByte(0x42U, 0xA5U);
    /* Simulate hardware write completion on host (stores cell + raises
     * EEIF). On target, hardware completes the write automatically. */
#ifndef __XC8
    pic18_sim_drive_eeprom_done(0x42U, 0xA5U);
#endif
    uint8_t r = EPIC_EEPROM_ReadByte(0x42U);

    for (uint32_t i = 0; epic_harness_running(i); i++)
    {
        epic_harness_tick();
    }

    epic_harness_log("EEPROM[0x42]=0x%02X.\n", (unsigned)r);
    return epic_harness_report(r == 0xA5U);
}
