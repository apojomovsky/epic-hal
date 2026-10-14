/**
 * CLC switch-debouncer probe, AN2805 2-CLC shape on 2-input D cells.
 * Host-only: the slow clock and switch arrive through the Dx hook (SEL codes
 * stored, not decoded), and the probe closes the unrouted inter-cell wire by
 * re-driving STAGE2 D2 from the live STAGE1 output each half clock. STAGE1
 * samples the switch, STAGE2 samples switch AND STAGE1, so a glitch must
 * persist across two slow edges to propagate.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

/** @brief Read back the STAGE1 (CLC2) output through the driver contract.
 * @return 0 or 1. */
static uint8_t read_stage1(void)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(CLC2, &level);
    return level;
}

/** @brief Read back the STAGE2 (CLC1) output through the driver contract.
 * @return 0 or 1. */
static uint8_t read_stage2(void)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(CLC1, &level);
    return level;
}

/**
 * @brief Drive one half clock into both stages, then step once.
 * @param clk Slow-clock level for D1 of both cells.
 * @param sw Switch level for D4 of both cells.
 * @param wire STAGE1 output re-driven into STAGE2 D2 (the inter-cell wire).
 */
static void step_half(uint8_t clk, uint8_t sw, uint8_t wire)
{
    pic16f1508_sim_drive_clc_input(1U, 1U, clk);
    pic16f1508_sim_drive_clc_input(1U, 4U, sw);
    pic16f1508_sim_drive_clc_input(0U, 1U, clk);
    pic16f1508_sim_drive_clc_input(0U, 2U, wire);
    pic16f1508_sim_drive_clc_input(0U, 4U, sw);
    epic_harness_tick();
}

/**
 * @brief Run one slow-clock rising edge with the switch held at sw.
 * @param sw Switch level sampled by this edge.
 */
static void clock_edge(uint8_t sw)
{
    uint8_t wire = read_stage1();

    step_half(0U, sw, wire);
    step_half(1U, sw, wire);
}

/**
 * @brief Debouncer checks: images, glitch blocked, press passes, release.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef f;
    CLC_HandleTypeDef o;

    epic_harness_init(10000UL);

    f.Instance = CLC2;
    f.Mode = 5U;
    f.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH);
    f.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_FOSC);
    f.Source[2] = EPIC_CLC_SourceCode(3U, CLC_IN_LC1_OUT);
    f.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN0);
    f.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    f.Gls[1] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    f.Gls[2] = 0U;
    f.Gls[3] = 0U;
    f.GatePolarity = 0U;
    f.OutputPolarity = 0U;
    f.OutputEnable = 0U;
    f.InterruptRising = 0U;
    f.InterruptFalling = 0U;
    f.Callback = NULL;
    ok &= (EPIC_CLC_Init(&f) == EPIC_OK);

    /* Packed select images: D1S T2_MATCH, D4S CLCIN0 on the filter. */
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL0) == 0x07U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL1) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS0) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS1) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS2) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS3) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC2CON) == 0x85U);

    o.Instance = CLC1;
    o.Mode = 5U;
    o.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH);
    o.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_LC2_OUT);
    o.Source[2] = EPIC_CLC_SourceCode(3U, CLC_IN_LC1_OUT);
    o.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN0);
    o.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    o.Gls[1] = (uint8_t)(EPIC_CLC_GLSInput(2U, 1U, 0U) | EPIC_CLC_GLSInput(4U, 1U, 0U));
    o.Gls[2] = 0U;
    o.Gls[3] = 0U;
    o.GatePolarity = 0x02U;
    o.OutputPolarity = 0U;
    o.OutputEnable = 0U;
    o.InterruptRising = 1U;
    o.InterruptFalling = 1U;
    o.Callback = NULL;
    ok &= (EPIC_CLC_Init(&o) == EPIC_OK);

    /* Packed select images: D1S T2_MATCH, D2S LC2_OUT, D4S CLCIN0. */
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL0) == 0x57U);
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL1) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS0) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS1) == 0x44U);
    ok &= (EPIC_REG8(PIC_REG_CLC1CON) == 0x9DU);
    ok &= (EPIC_REG8(PIC_REG_CLC1POL) == 0x02U);

    /* Idle: both stages low, no edge flag. */
    clock_edge(0U);
    clock_edge(0U);
    ok &= (read_stage1() == 0U);
    ok &= (read_stage2() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* One-clock glitch latches STAGE1 but never reaches the output. */
    clock_edge(1U);
    ok &= (read_stage1() == 1U);
    ok &= (read_stage2() == 0U);
    clock_edge(0U);
    ok &= (read_stage1() == 0U);
    ok &= (read_stage2() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* Sustained press sets STAGE1 first, then the output with an edge flag. */
    clock_edge(1U);
    ok &= (read_stage1() == 1U);
    ok &= (read_stage2() == 0U);
    clock_edge(1U);
    ok &= (read_stage2() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) != 0U);

    /* Held press stays high with no new edge flag. */
    EPIC_REG8(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_CLC1IF;
    clock_edge(1U);
    ok &= (read_stage2() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* Release clears both stages with a falling-edge flag. */
    clock_edge(0U);
    ok &= (read_stage1() == 0U);
    ok &= (read_stage2() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) != 0U);

    /* Disabled cells read 0 even with the switch driven. */
    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    ok &= (EPIC_CLC_DeInit(CLC2) == EPIC_OK);
    clock_edge(1U);
    ok &= (read_stage1() == 0U);
    ok &= (read_stage2() == 0U);

    return epic_harness_report(ok);
}
