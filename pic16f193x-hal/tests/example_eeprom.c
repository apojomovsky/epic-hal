/**
 * @file    example_eeprom.c
 * @brief   EEPROM smoke test: set WREN on EECON1 (bank 3), read back,
 *          verify the banked read-modify-write landed correctly. This
 *          is the specific risk the brief flagged (same class as the
 *          PIE1/2/3 Finding 2 bug). The §4 gate payload.
 *
 * @details
 *   Expected register image (after WREN set):
 *     EECON1 = 0x04   (WREN=1 at bit 2, rest 0)
 *
 *   This test does NOT do a full EEPROM write/read cycle (which would
 *   require the while(WR) spin that deadlocks the host sim's polled
 *   step model). It only tests the banked RMW of EECON1, which is the
 *   actual codegen risk. If the §4 gate shows WREN not landing, apply
 *   the __at(0x70) scratch + inline-asm movlb fix per ARCHITECTURE.md
 *   Finding 2.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_eeprom.h"
#include "core/epic_harness.h"

extern void pic16f193x_harness_halt(void);

int main(void)
{
    epic_harness_init(1UL);

    EPIC_EEPROM_Init();
    /* Test the banked RMW: set WREN on EECON1 (bank 3) and verify. */
    EPIC_BIT_SET(EPIC_REG8(PIC_REG_EECON1), PIC_EECON1_WREN);

    uint8_t econ1 = EPIC_REG8(PIC_REG_EECON1);

    epic_harness_log("EECON1=0x%02X\n", econ1);
    int pass = ((econ1 & PIC_EECON1_WREN) != 0U);
    int rc = epic_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
