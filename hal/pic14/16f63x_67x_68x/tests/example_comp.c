/* Comparator smoke test on the sim backend. C1 and C2 are independent
 * comparators with separate interrupt flags (C1IF/C2IF in PIR2); the
 * reference comes from VRCON<CxVREN> (CVREF) or the 0.6 V fixed
 * reference, selected per comparator.
 *
 * Expected register image: C1 with channel IN0, ref from CVREF,
 * non-inverted, internal output programs CM1CON0 = C1ON|C1R = 0x84
 * and sets VRCON<C1VREN> = 0x80. */

#include "pic16f63x_67x_68x_hal.h"
#include "pic16f63x_67x_68x_sim.h"
#include "pic16f63x_67x_68x_sfr.h"
#include "peripherals/pic16f63x_67x_68x_comp.h"
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
 * @brief Smoke-test the C1/C2 comparator drivers.
 */
int main(void)
{
    pic16f63x_67x_68x_sim_reset();
    pic16f63x_67x_68x_sim_set_irq_callback(epic_dispatch_all_irqs);

    /* 1. C1: channel IN0, ref from CVREF, non-inverted, output internal. */
    COMP_HandleTypeDef c1 = COMP_HANDLE_DEFAULT;
    c1.Channel         = COMP_CHANNEL_IN0;
    c1.InputSource     = COMP_INPUT_REF;
    c1.RefSource       = COMP_REF_CVREF;
    c1.Inverted        = false;
    c1.ChangeCallback  = on_c1_change;
    CHECK(EPIC_COMP1_Init(&c1) == EPIC_OK, "COMP1_Init failed");

    /* 2. C2: channel IN1, absolute reference, output internal. */
    COMP_HandleTypeDef c2 = COMP_HANDLE_DEFAULT;
    c2.Channel         = COMP_CHANNEL_IN1;
    c2.InputSource     = COMP_INPUT_REF;
    c2.RefSource       = COMP_REF_ABSOLUTE;
    c2.ChangeCallback  = on_c2_change;
    CHECK(EPIC_COMP2_Init(&c2) == EPIC_OK, "COMP2_Init failed");

    /* 3. Verify the register image. CM1CON0 = C1ON(bit7)|C1R(bit2) =
     * 0x84; VRCON = C1VREN(bit7) = 0x80 (C2VREN stays clear). */
    CHECK(EPIC_REG8(PIC_REG_CM1CON0) == 0x84U, "CM1CON0 != C1ON|C1R");
    CHECK(EPIC_REG8(PIC_REG_CM2CON0) == 0x85U, "CM2CON0 != C2ON|C2R|CH1");
    CHECK(EPIC_REG8(PIC_REG_VRCON) == 0x80U, "VRCON != C1VREN");

    /* 4. Sim a C1 output transition. The installed dispatcher routes the
     *    flag to COMP1_IRQHandler, which clears C1IF and fires the
     *    callback, so assert on the callback count and the live output. */
    EPIC_IRQ_Restore(1);
    pic16f63x_67x_68x_sim_drive_comparator(1U, 1U);
    CHECK(c1_changes >= 1U, "C1 change callback not fired");
    CHECK(EPIC_COMP_C1Out() == 1U, "C1Out not high after drive_comparator(1,1)");

    pic16f63x_67x_68x_sim_drive_comparator(2U, 1U);
    CHECK(c2_changes >= 1U, "C2 change callback not fired");
    CHECK(EPIC_COMP_C2Out() == 1U, "C2Out not high after drive_comparator(2,1)");

    printf("OK: comparator C1/C2 pass.\n");
    return 0;
}
