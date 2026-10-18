/**
 * FVR host-sim probe. Host-only: the ready flag follows FVREN in the
 * analog model through epic_harness_tick, so it is not in the XC8 build
 * (the mdb gate probe is example_gpio_mdb.c). Covers handle validation,
 * the Init image, the ready flag and the DeInit POR state.
 *
 * Expected register image (default 2x/2x gains):
 *   FVRCON = 0x8A (FVREN, CDAFVR 2x, ADFVR 2x), ready after one tick
 *   (FVRRDY set: 0xCA).
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_fvr.h"

/**
 * @brief FVR checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    FVR_HandleTypeDef h = FVR_HANDLE_DEFAULT;

    epic_harness_init(100UL);

    /* POR: FVR off, not ready. */
    ok &= (EPIC_REG8(PIC_REG_FVRCON) == 0x00U);
    ok &= (EPIC_FVR_IsReady() == 0U);

    /* Validation: NULL rejected before anything is written. */
    ok &= (EPIC_FVR_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_FVRCON) == 0x00U);

    /* Default image: 2x on both buffers. */
    ok &= (EPIC_FVR_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_FVRCON) == 0x8AU);
    ok &= (EPIC_FVR_IsReady() == 0U);
    epic_harness_tick();
    ok &= (EPIC_FVR_IsReady() != 0U);
    ok &= (EPIC_REG8(PIC_REG_FVRCON) == 0xCAU);

    /* 1x ADC buffer, comparator buffer off. */
    h.AdcGain = FVR_GAIN_1X;
    h.CompDacGain = FVR_GAIN_OFF;
    ok &= (EPIC_FVR_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_FVRCON) & (uint8_t)~PIC_FVRCON_FVRRDY) == 0x81U);

    /* Buffers off stays enabled but gains read 0. */
    h.AdcGain = FVR_GAIN_OFF;
    ok &= (EPIC_FVR_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_FVRCON) & (uint8_t)~PIC_FVRCON_FVRRDY) == 0x80U);

    /* DeInit restores POR and drops ready. */
    ok &= (EPIC_FVR_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_FVRCON) == 0x00U);
    epic_harness_tick();
    ok &= (EPIC_FVR_IsReady() == 0U);

    epic_harness_log("fvr: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
