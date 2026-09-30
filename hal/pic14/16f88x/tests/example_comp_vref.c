/* Comparator + CVREF + SR-latch smoke test on the sim backend. C1 and
 * C2 are independent comparators with separate interrupt flags
 * (C1IF/C2IF in PIR2); the CVREF (VRCON) supplies a reference; the SR
 * latch (SRCON) routes latch Q to the comparator output pins. */

#include "pic16f88x.h"
#include "pic16f88x_sim.h"
#include "pic16f88x_sfr.h"
#include "peripherals/pic16f88x_comp.h"
#include "peripherals/pic16f88x_vref.h"
#include "peripherals/pic16f88x_srlatch.h"
#include "core/pic16_irq.h"
#include "core/epic_harness.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

static volatile uint32_t c1_changes = 0;
static volatile uint32_t c2_changes = 0;

/**
 * @brief Count C1 output changes.
 */
static void on_c1_change(void)
{
    c1_changes++;
}

/**
 * @brief Count C2 output changes.
 */
static void on_c2_change(void)
{
    c2_changes++;
}

/**
 * @brief Smoke-test the comparator, CVREF and SR-latch drivers.
 */
int main(void)
{
    pic16f88x_sim_reset();
    pic16f88x_sim_set_irq_callback(epic_dispatch_all_irqs);

    /* 1. C1: channel IN0, ref from CVREF, non-inverted, output internal. */
    COMP_HandleTypeDef c1 = COMP_HANDLE_DEFAULT;
    c1.Channel         = COMP_CHANNEL_IN0;
    c1.InputSource     = COMP_INPUT_REF;
    c1.RefSource       = COMP_REF_CVREF;
    c1.Inverted        = false;
    c1.ChangeCallback  = on_c1_change;
    CHECK(EPIC_COMP1_Init(&c1) == EPIC_OK, "COMP1_Init failed");

    /* 2. C2: channel IN0, ref from CVREF, output internal. */
    COMP_HandleTypeDef c2 = COMP_HANDLE_DEFAULT;
    c2.Channel         = COMP_CHANNEL_IN0;
    c2.InputSource     = COMP_INPUT_REF;
    c2.RefSource       = COMP_REF_CVREF;
    c2.ChangeCallback  = on_c2_change;
    CHECK(EPIC_COMP2_Init(&c2) == EPIC_OK, "COMP2_Init failed");

    /* 3. Verify the register image. CM1CON0: C1ON(bit7) | C1R(bit2) = 0x84.
     * CM2CON1: C1RSEL(bit5) | C2RSEL(bit4) = 0x30. */
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(2);
        uint8_t cm1 = EPIC_REG8(PIC_REG_CM1CON0);
        uint8_t cm2 = EPIC_REG8(PIC_REG_CM2CON1);
        pic_select_bank(prev);
        CHECK((cm1 & 0x84U) == 0x84U, "CM1CON0 not programmed for C1ON|C1R");
        CHECK((cm2 & 0x30U) == 0x30U, "CM2CON1 C1RSEL/C2RSEL not set");
    }

    /* 4. Sim a C1 output transition. The installed dispatcher routes the
     *    flag to COMP1_IRQHandler, which clears C1IF and fires the
     *    callback, so assert on the callback count and the live output. */
    EPIC_IRQ_Restore(1);
    pic16f88x_sim_drive_comparator(1U, 1U);
    CHECK(c1_changes >= 1U, "C1 change callback not fired");
    CHECK(EPIC_COMP_C1Out() == 1U, "C1Out not high after drive_comparator(1,1)");

    pic16f88x_sim_drive_comparator(2U, 1U);
    CHECK(c2_changes >= 1U, "C2 change callback not fired");
    CHECK(EPIC_COMP_C2Out() == 1U, "C2Out not high after drive_comparator(2,1)");

    /* 5. CVREF: high range (VRR=0), tap 8 → VDD/4 + 8/32*VDD = 0.5 VDD.
     *    VRCON = VREN(0x80) | tap 8 = 0x88. */
    VREF_HandleTypeDef vh = VREF_HANDLE_DEFAULT;
    vh.Range         = VREF_RANGE_HIGH;
    vh.Source        = VREF_SRC_VDD_VSS;
    vh.Value         = 8U;
    vh.OutputEnable  = false;
    vh.Enabled       = true;
    CHECK(EPIC_VREF_Init(&vh) == EPIC_OK, "VREF_Init failed");

    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        uint8_t vrcon = EPIC_REG8(PIC_REG_VRCON);
        pic_select_bank(prev);
        CHECK((vrcon & 0x88U) == 0x88U, "VRCON not programmed for VREN|tap8");
    }

    /* MilliVolts: 0.5 × 5000 = 2500 mV. */
    uint32_t mv = EPIC_VREF_MilliVolts(5000UL, VREF_RANGE_HIGH, 8U);
    CHECK(mv == 2500UL, "VREF_MilliVolts(5000, HIGH, 8) != 2500");

    /* 6. SR latch: route Q to C1OUT pin, set via PULSS. */
    SRLATCH_HandleTypeDef sh = SRLATCH_HANDLE_DEFAULT;
    sh.Output       = SRLATCH_OUT_C1_Q;
    sh.C1SetEnable  = true;
    CHECK(EPIC_SRLATCH_Init(&sh) == EPIC_OK, "SRLATCH_Init failed");
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK((srcon & 0x60U) == 0x60U, "SRCON not programmed for Q-out|C1SEN");
    }
    EPIC_SRLATCH_Set();
    {
        uint8_t prev = (EPIC_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(3);
        uint8_t srcon = EPIC_REG8(PIC_REG_SRCON);
        pic_select_bank(prev);
        CHECK((srcon & PIC_SRCON_PULSS) != 0U, "PULSS not set after SRLATCH_Set");
    }

    printf("OK: comparator C1/C2, CVREF, SR latch all pass.\n");
    return 0;
}
