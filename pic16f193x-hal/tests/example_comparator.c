/**
 * @file    example_comparator.c
 * @brief   Comparator smoke test: init both instances, confirm
 *          control-register state. The §4 gate payload.
 *
 * @details
 *   Expected register image (after init):
 *     CM1CON0 = 0x80   (C1ON=1, C1OUT read-only at bit 6 masked)
 *     CM2CON0 = 0x80   (C2ON=1, C2OUT read-only at bit 6 masked)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_comp.h"
#include "core/pic8_harness.h"

extern void pic16f193x_harness_halt(void);

int main(void)
{
    pic8_harness_init(1UL);

    COMP_HandleTypeDef c1 = COMP_HANDLE_DEFAULT;
    c1.Instance = COMP_INSTANCE_1;
    HAL_COMP_Init(&c1);

    COMP_HandleTypeDef c2 = COMP_HANDLE_DEFAULT;
    c2.Instance = COMP_INSTANCE_2;
    HAL_COMP_Init(&c2);

    uint8_t cm1con0 = PIC8_REG8(PIC_REG_CM1CON0);
    uint8_t cm2con0 = PIC8_REG8(PIC_REG_CM2CON0);

    pic8_harness_log("CM1CON0=0x%02X CM2CON0=0x%02X\n", cm1con0, cm2con0);
    /* C1OUT/C2OUT (bit 6) are read-only hardware status bits; mask
     * them out so the check is on the writable control bits only. */
    int pass = ((cm1con0 & 0xBFU) == 0x80U) && ((cm2con0 & 0xBFU) == 0x80U);
    int rc = pic8_harness_report(pass);
    pic16f193x_harness_halt();
    return rc;
}
