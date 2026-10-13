/**
 * CLC functional-model host-sim probe. Host-only: configures cells through
 * EPIC_CLC_Init, drives Dx levels through pic16f1508_sim_drive_clc_input
 * (SEL codes are stored but not decoded, #384), steps the simulator, and
 * checks outputs, mirrors, sequential state, and edge interrupts.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

static volatile uint8_t g_clc1_calls;

/**
 * @brief CLC1 user callback: counts dispatches.
 */
static void on_clc1(void)
{
    g_clc1_calls++;
}

/**
 * @brief Configure one cell with single-true-term gates (Dx on Gx).
 * @param cell Cell to configure (CLC1..CLC4).
 * @param mode LCxMODE value 0..7.
 */
static void setup_passthrough(CLC_InstanceTypeDef cell, uint8_t mode)
{
    CLC_HandleTypeDef h;
    unsigned int i;

    h.Instance = cell;
    h.Mode = mode;
    h.Source[0] = 5U;
    h.Source[1] = 6U;
    h.Source[2] = 7U;
    h.Source[3] = 1U;
    for (i = 0U; i < 4U; i++)
    {
        h.Gls[i] = EPIC_CLC_GLSInput((uint8_t)(i + 1U), 0U, 1U);
    }
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    EPIC_CLC_Init(&h);
}

/**
 * @brief Drive all four Dx levels of a cell, then step once.
 * @param cell Cell index 0..3.
 * @param d Packed D1..D4 levels in bits 3:0.
 */
static void drive_and_step(uint8_t cell, uint8_t d)
{
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        pic16f1508_sim_drive_clc_input(cell, (uint8_t)(i + 1U), (uint8_t)((d >> i) & 1U));
    }
    epic_harness_tick();
}

/**
 * @brief Read back a cell output through the driver contract.
 * @param cell Cell to read.
 * @return 0 or 1.
 */
static uint8_t read_cell(CLC_InstanceTypeDef cell)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(cell, &level);
    return level;
}

/**
 * @brief CLC functional model checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef h;

    epic_harness_init(10000UL);

    /* AND-OR on CLC1: (D1&D2)|(D3&D4), with CLCDATA and CON.OUT mirrors. */
    setup_passthrough(CLC1, 0U);
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 1U);
    ok &= ((EPIC_REG8(PIC_REG_CLCDATA) & PIC_CLCDATA_MCLC1OUT) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_CLC1CON) & PIC_CLCCON_OUT) != 0U);
    drive_and_step(0U, 0x05U);
    ok &= (read_cell(CLC1) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_CLCDATA) & PIC_CLCDATA_MCLC1OUT) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_CLC1CON) & PIC_CLCCON_OUT) == 0U);
    drive_and_step(0U, 0x0CU);
    ok &= (read_cell(CLC1) == 1U);

    /* Gate polarity inverts G1; output polarity inverts the cell. */
    h.Instance = CLC1;
    h.Mode = 0U;
    h.Source[0] = 0U;
    h.Source[1] = 0U;
    h.Source[2] = 0U;
    h.Source[3] = 0U;
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 0U, 1U);
    h.Gls[2] = 0U;
    h.Gls[3] = 0U;
    h.GatePolarity = 0x01U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    h.Callback = NULL;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 0U);
    drive_and_step(0U, 0x02U);
    ok &= (read_cell(CLC1) == 1U);
    h.GatePolarity = 0U;
    h.OutputPolarity = 1U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 0U);
    drive_and_step(0U, 0x00U);
    ok &= (read_cell(CLC1) == 1U);

    /* OR-XOR: (D1|D2)^(D3|D4). 4-input AND: D1&D2&D3&D4. */
    setup_passthrough(CLC2, 1U);
    drive_and_step(1U, 0x01U);
    ok &= (read_cell(CLC2) == 1U);
    drive_and_step(1U, 0x05U);
    ok &= (read_cell(CLC2) == 0U);
    drive_and_step(1U, 0x0FU);
    ok &= (read_cell(CLC2) == 0U);
    setup_passthrough(CLC2, 2U);
    drive_and_step(1U, 0x0FU);
    ok &= (read_cell(CLC2) == 1U);
    drive_and_step(1U, 0x07U);
    ok &= (read_cell(CLC2) == 0U);
    ok &= (EPIC_CLC_DeInit(CLC2) == EPIC_OK);

    /* SR latch on CLC3: S=G1, R=G2; set, hold, reset, S dominates. */
    setup_passthrough(CLC3, 3U);
    drive_and_step(2U, 0x01U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x00U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x02U);
    ok &= (read_cell(CLC3) == 0U);
    drive_and_step(2U, 0x00U);
    ok &= (read_cell(CLC3) == 0U);
    drive_and_step(2U, 0x03U);
    ok &= (read_cell(CLC3) == 1U);
    ok &= (EPIC_CLC_DeInit(CLC3) == EPIC_OK);

    /* 1-input D flip-flop on CLC4: CLK=G1, D=G2, R=G3, S=G4. */
    setup_passthrough(CLC4, 4U);
    drive_and_step(3U, 0x02U);
    ok &= (read_cell(CLC4) == 0U);
    drive_and_step(3U, 0x03U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x00U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x04U);
    ok &= (read_cell(CLC4) == 0U);
    drive_and_step(3U, 0x03U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x08U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x00U);
    ok &= (read_cell(CLC4) == 1U);
    ok &= (EPIC_CLC_DeInit(CLC4) == EPIC_OK);

    /* 2-input D flip-flop on CLC2: CLK=G1, D=G2|G3, R=G4. */
    setup_passthrough(CLC2, 5U);
    drive_and_step(1U, 0x04U);
    ok &= (read_cell(CLC2) == 0U);
    drive_and_step(1U, 0x05U);
    ok &= (read_cell(CLC2) == 1U);
    drive_and_step(1U, 0x08U);
    ok &= (read_cell(CLC2) == 0U);
    drive_and_step(1U, 0x03U);
    ok &= (read_cell(CLC2) == 1U);
    ok &= (EPIC_CLC_DeInit(CLC2) == EPIC_OK);

    /* JK flip-flop on CLC3: CLK=G1, J=G2, K=G3, R=G4. */
    setup_passthrough(CLC3, 6U);
    drive_and_step(2U, 0x03U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x00U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x01U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x00U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x07U);
    ok &= (read_cell(CLC3) == 0U);
    drive_and_step(2U, 0x00U);
    ok &= (read_cell(CLC3) == 0U);
    drive_and_step(2U, 0x07U);
    ok &= (read_cell(CLC3) == 1U);
    drive_and_step(2U, 0x00U);
    drive_and_step(2U, 0x05U);
    ok &= (read_cell(CLC3) == 0U);
    drive_and_step(2U, 0x0BU);
    ok &= (read_cell(CLC3) == 0U);
    ok &= (EPIC_CLC_DeInit(CLC3) == EPIC_OK);

    /* Transparent latch on CLC4: E=G1, D=G2, R=G3, S=G4. */
    setup_passthrough(CLC4, 7U);
    drive_and_step(3U, 0x03U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x00U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x02U);
    ok &= (read_cell(CLC4) == 1U);
    drive_and_step(3U, 0x04U);
    ok &= (read_cell(CLC4) == 0U);
    drive_and_step(3U, 0x08U);
    ok &= (read_cell(CLC4) == 1U);
    ok &= (EPIC_CLC_DeInit(CLC4) == EPIC_OK);

    /* Edges: rising-only first, then falling-only, both, and neither, all through the handle. */
    h.Instance = CLC1;
    h.Mode = 0U;
    h.Source[0] = 5U;
    h.Source[1] = 6U;
    h.Source[2] = 7U;
    h.Source[3] = 1U;
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 0U, 1U);
    h.Gls[2] = 0U;
    h.Gls[3] = 0U;
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 1U;
    h.InterruptFalling = 0U;
    h.Callback = on_clc1;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC1);
    EPIC_IRQ_Restore(1U);
    drive_and_step(0U, 0x00U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    drive_and_step(0U, 0x03U);
    ok &= (g_clc1_calls == 1U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    drive_and_step(0U, 0x00U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    ok &= (g_clc1_calls == 1U);

    /* Falling only: the rising edge is silent, the falling edge dispatches. */
    h.InterruptRising = 0U;
    h.InterruptFalling = 1U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    ok &= (g_clc1_calls == 1U);
    drive_and_step(0U, 0x00U);
    ok &= (g_clc1_calls == 2U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);

    /* Both edges dispatch once each. */
    h.InterruptRising = 1U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (g_clc1_calls == 3U);
    drive_and_step(0U, 0x00U);
    ok &= (g_clc1_calls == 4U);

    /* Neither edge: the cell toggles with no flag and no dispatch. */
    h.InterruptRising = 0U;
    h.InterruptFalling = 0U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    drive_and_step(0U, 0x00U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    ok &= (g_clc1_calls == 4U);
    h.InterruptFalling = 1U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);

    /* Flag sets for polling with the enable clear; no dispatch without it. */
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC1);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_CLC1);
    drive_and_step(0U, 0x03U);
    drive_and_step(0U, 0x00U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) != 0U);
    ok &= (g_clc1_calls == 4U);
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_CLC1);

    /* Disabled cell: output forced to 0, no edge, no flag. */
    h.OutputPolarity = 0U;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 1U);
    EPIC_REG8(PIC_REG_CLC1CON) &= (uint8_t)~PIC_CLCCON_EN;
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    EPIC_REG8(PIC_REG_CLC1POL) |= PIC_CLCPOL_POL;
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);
    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    drive_and_step(0U, 0x03U);
    ok &= (read_cell(CLC1) == 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);

    epic_harness_log("clc/model: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
