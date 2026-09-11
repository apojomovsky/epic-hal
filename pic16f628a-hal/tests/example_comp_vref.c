/* Comparator + Vref driver smoke test: COMP Init programs CMCON (Bank 0
 * on this part) for two independent comparators with C1 inverted, VREF
 * Init programs VRCON (0.25..0.75 VDD range, tap 8, output enabled),
 * and EPIC_VREF_MilliVolts computes the expected mV values. */

#include "pic16f628a_hal.h"
#include "pic16f628a_sim.h"
#include "pic16f628a_sfr.h"
#include "peripherals/pic14_comp.h"
#include "peripherals/pic14_vref.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

/**
 * @brief Smoke-test the comparator and voltage-reference drivers on the
 *        sim backend.
 */
int main(void)
{
    /* --- Comparator --- */
    pic16f628a_sim_reset();

    COMP_HandleTypeDef ch = COMP_HANDLE_DEFAULT;
    ch.Mode = COMP_MODE_TWO_INDEP;
    ch.C1Inverted = true;
    EPIC_COMP_Init(&ch);

    {
        /* CMCON lives in Bank 0 (0x1F) on this part. */
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        uint8_t cmcon = EPIC_REG8(PIC_REG_CMCON);
        pic_select_bank(prev);
        /* Expected: CM2:CM0 = 010, C1INV = 1 → 0x12. */
        CHECK(cmcon == 0x12U, "CMCON not programmed for two indep with C1 inverted");
    }

    EPIC_COMP_DeInit();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(0);
        uint8_t cmcon = EPIC_REG8(PIC_REG_CMCON);
        pic_select_bank(prev);
        CHECK(cmcon == 0x07U, "CMCON not 0x07 (off) after DeInit");
    }

    /* --- Vref --- */
    pic16f628a_sim_reset();

    VREF_HandleTypeDef vh = VREF_HANDLE_DEFAULT;
    vh.Range = VREF_RANGE_HIGH;
    vh.Value = 8;
    vh.OutputEnable = true;
    vh.Enabled = true;
    EPIC_VREF_Init(&vh);

    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t vr = EPIC_REG8(PIC_REG_VRCON);
        pic_select_bank(prev);
        /* Expected: VR3:0 = 1000 (0x08), VRR = 0 (high range on VRCON
         * parts), VROE = 1, VREN = 1 → 0x08 | 0x40 | 0x80 = 0xC8.
         * The VRR polarity is verified against silicon by the mdb
         * gate; see MANUAL.md. */
        CHECK(vr == 0xC8U, "VRCON not programmed correctly");
    }

    /* MilliVolts helper. */
    CHECK(EPIC_VREF_MilliVolts(5000U, VREF_RANGE_LOW,  0U)  == 0U,
          "Vref low tap 0 != 0 mV");
    CHECK(EPIC_VREF_MilliVolts(5000U, VREF_RANGE_LOW, 12U)  == 2500U,
          "Vref low tap 12 != 2500 mV at 5V");
    CHECK(EPIC_VREF_MilliVolts(5000U, VREF_RANGE_HIGH, 0U)  == 1250U,
          "Vref high tap 0 != 1250 mV at 5V");
    CHECK(EPIC_VREF_MilliVolts(5000U, VREF_RANGE_HIGH, 8U)  == 2500U,
          "Vref high tap 8 != 2500 mV at 5V");

    printf("OK: Comparator + Vref drivers, mode, inverted, milliVolts all pass.\n");
    return 0;
}
