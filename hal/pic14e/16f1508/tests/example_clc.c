/**
 * CLC register-contract host-sim probe. Host-only: uses the flat simulator SFR
 * array through EPIC_REG8, so it is not in the XC8 build. Covers the packed
 * SEL/POL/GLS/CON images, rejection of invalid handles without touching
 * registers, CLCDATA readout, and DeInit.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/epic_harness.h"

/**
 * @brief Run the CLC register-contract checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef h = {0};
    uint8_t level = 0xFFU;
    unsigned int i;
    uint8_t before[8];

    epic_harness_init(10000UL);

    /* GLS helper: input 2 negate is bit 2, input 2 true is bit 3, input 4 true is bit 7. */
    ok &= (EPIC_CLC_GLSInput(2U, 1U, 0U) == 0x04U);
    ok &= (EPIC_CLC_GLSInput(2U, 0U, 1U) == 0x08U);
    ok &= (EPIC_CLC_GLSInput(4U, 0U, 1U) == 0x80U);
    ok &= (EPIC_CLC_GLSInput(0U, 1U, 1U) == 0x00U);
    ok &= (EPIC_CLC_GLSInput(5U, 1U, 1U) == 0x00U);

    /* CLC2 full configuration: every packed register checked against its literal image. */
    h.Instance = CLC2;
    h.Mode = 3U;
    h.Source[0] = 1U;
    h.Source[1] = 2U;
    h.Source[2] = 3U;
    h.Source[3] = 4U;
    h.Gls[0] = (uint8_t)(EPIC_CLC_GLSInput(1U, 1U, 0U) | EPIC_CLC_GLSInput(3U, 0U, 1U));
    h.Gls[1] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    h.Gls[2] = EPIC_CLC_GLSInput(3U, 1U, 1U);
    h.Gls[3] = EPIC_CLC_GLSInput(2U, 1U, 1U);
    h.GatePolarity = 0x05U;
    h.OutputPolarity = 1U;
    h.OutputEnable = 1U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CLC2CON) == 0xC3U);
    ok &= (EPIC_REG8(PIC_REG_CLC2POL) == 0x85U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL0) == 0x21U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL1) == 0x43U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS0) == 0x21U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS1) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS2) == 0x30U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS3) == 0x0CU);

    /* Invalid handles return EPIC_INVALID and leave the CLC3 registers as the valid call above set them. */
    h.Instance = CLC3;
    h.Mode = 2U;
    h.Source[0] = 5U;
    h.Source[1] = 6U;
    h.Source[2] = 0U;
    h.Source[3] = 0U;
    h.GatePolarity = 0x00U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 1U, 0U);
    h.Gls[2] = EPIC_CLC_GLSInput(3U, 1U, 1U);
    h.Gls[3] = EPIC_CLC_GLSInput(4U, 1U, 0U);
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CLC3SEL0) == (uint8_t)(5U | (6U << 4U)));
    ok &= (EPIC_REG8(PIC_REG_CLC3CON) == 0x82U);
    for (i = 0U; i < 8U; i++)
    {
        before[i] = EPIC_REG8(PIC_REG_CLC3CON + i);
    }

    for (i = 0U; i < 4U; i++)
    {
        h.Gls[i] = 0xFFU;
    }
    h.Mode = 8U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_INVALID);
    h.Mode = 2U;
    h.Source[1] = 8U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_INVALID);
    h.Source[1] = 6U;
    h.GatePolarity = 0x10U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_INVALID);
    h.GatePolarity = 0x00U;
    h.Instance = (CLC_InstanceTypeDef)4;
    ok &= (EPIC_CLC_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_CLC_Init(NULL) == EPIC_INVALID);
    for (i = 0U; i < 8U; i++)
    {
        ok &= (EPIC_REG8(PIC_REG_CLC3CON + i) == before[i]);
    }

    /* CLCDATA readout: MCLC3OUT set reads back for CLC3 only. */
    EPIC_REG8(PIC_REG_CLCDATA) = PIC_CLCDATA_MCLC3OUT;
    ok &= (EPIC_CLC_ReadOutput(CLC3, &level) == EPIC_OK);
    ok &= (level == 1U);
    ok &= (EPIC_CLC_ReadOutput(CLC1, &level) == EPIC_OK);
    ok &= (level == 0U);
    ok &= (EPIC_CLC_ReadOutput((CLC_InstanceTypeDef)4, &level) == EPIC_INVALID);
    ok &= (EPIC_CLC_ReadOutput(CLC1, NULL) == EPIC_INVALID);

    /* DeInit clears every configuration register of the cell. */
    ok &= (EPIC_CLC_DeInit(CLC2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CLC3CON) == 0x82U);
    ok &= (EPIC_REG8(PIC_REG_CLC2CON) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2POL) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL0) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL1) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS0) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS3) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS1) == 0U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS2) == 0U);
    ok &= (EPIC_CLC_DeInit((CLC_InstanceTypeDef)4) == EPIC_INVALID);

    epic_harness_log("clc/register smoke: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
