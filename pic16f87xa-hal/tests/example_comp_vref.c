/**
 * @file    example_comp_vref.c
 * @brief   Comparator + Vref driver smoke test.
 *
 *   Verifies:
 *     1. EPIC_COMP_Init() programs CMCON for two independent comparators
 *        with C1 inverted.
 *     2. EPIC_VREF_Init() programs CVRCON for 0.25..0.75 VDD range,
 *        tap 8, output enabled.
 *     3. EPIC_VREF_MilliVolts() computes correct values:
 *        - low range,  tap 0  → 0 mV
 *        - low range,  tap 12 → Vdd/2 = 2500 mV at 5 V
 *        - high range, tap 0  → Vdd/4 = 1250 mV at 5 V
 *        - high range, tap 8  → Vdd/4 + Vdd/4 = Vdd/2 = 2500 mV at 5 V
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_comp.h"
#include "peripherals/pic16f87xa_vref.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    /* --- Comparator --- */
    pic16f87xa_sim_reset();

    COMP_HandleTypeDef ch = COMP_HANDLE_DEFAULT;
    ch.Mode = COMP_MODE_TWO_INDEP;
    ch.C1Inverted = true;
    EPIC_COMP_Init(&ch);

    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t cmcon = EPIC_REG8(0x9CU);
        pic_select_bank(prev);
        /* Expected: CM2:CM0 = 010, C1INV = 1 → 0x12. */
        CHECK(cmcon == 0x12U, "CMCON not programmed for two indep with C1 inverted");
    }

    EPIC_COMP_DeInit();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t cmcon = EPIC_REG8(0x9CU);
        pic_select_bank(prev);
        CHECK(cmcon == 0x07U, "CMCON not 0x07 (off) after DeInit");
    }

    /* --- Vref --- */
    pic16f87xa_sim_reset();

    VREF_HandleTypeDef vh = VREF_HANDLE_DEFAULT;
    vh.Range = VREF_RANGE_HIGH;
    vh.Value = 8;
    vh.OutputEnable = true;
    vh.Enabled = true;
    EPIC_VREF_Init(&vh);

    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t cvr = EPIC_REG8(0x9DU);
        pic_select_bank(prev);
        /* Expected: CVR3:0 = 1000 (0x08), CVRR = 1, CVROE = 1, CVREN = 1
         *          → 0x08 | 0x20 | 0x40 | 0x80 = 0xE8. */
        CHECK(cvr == 0xE8U, "CVRCON not programmed correctly");
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
