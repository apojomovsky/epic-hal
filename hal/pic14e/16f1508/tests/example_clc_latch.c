/**
 * CLC S-R latch demo probe, TB3273 LED-dimmer front half. Host-only: drives
 * Dx levels through pic16f1508_sim_drive_clc_input (driven levels win over
 * the SEL decode), so the Dx levels driven below stand in for the real
 * timer and PWM sources. Wiring mirrors the MCC clc1.c
 * beat pair: S on D1+D2, R on D3+D4, S-R latch mode, no polarity.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

static uint8_t s_calls;

/** @brief CLC1 user callback: counts dispatches. */
static void on_clc1(void)
{
    s_calls++;
}

/**
 * @brief Drive all four Dx levels of CLC1, then step once.
 * @param d Packed D1..D4 levels in bits 3:0.
 */
static void drive_and_step(uint8_t d)
{
    uint8_t i;

    for (i = 0U; i < 4U; i++)
    {
        pic16f1508_sim_drive_clc_input(0U, (uint8_t)(i + 1U), (uint8_t)((d >> i) & 1U));
    }
    epic_harness_tick();
}

/**
 * @brief Read back the CLC1 output through the driver contract.
 * @return 0 or 1.
 */
static uint8_t read_latch(void)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(CLC1, &level);
    return level;
}

/**
 * @brief S-R latch demo checks: set, hold, reset, S priority, edge flags.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef h;

    epic_harness_init(10000UL);

    h.Instance = CLC1;
    h.Mode = 3U;
    h.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_T2_MATCH);
    h.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_T2_MATCH);
    h.Source[2] = EPIC_CLC_SourceCode(3U, CLC1_IN_PWM3_OUT);
    h.Source[3] = EPIC_CLC_SourceCode(4U, CLC1_IN_PWM3_OUT);
    h.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    h.Gls[1] = EPIC_CLC_GLSInput(2U, 0U, 1U);
    h.Gls[2] = EPIC_CLC_GLSInput(3U, 0U, 1U);
    h.Gls[3] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    h.GatePolarity = 0U;
    h.OutputPolarity = 0U;
    h.OutputEnable = 0U;
    h.InterruptRising = 1U;
    h.InterruptFalling = 1U;
    h.Callback = on_clc1;
    ok &= (EPIC_CLC_Init(&h) == EPIC_OK);

    /* Packed select images: D1S 7 and D2S 3 in SEL0, D3S 6 and D4S 2 in SEL1. */
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL0) == 0x37U);
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL1) == 0x26U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS0) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS1) == 0x08U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS2) == 0x20U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS3) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC1CON) == 0x9BU);

    /* Idle: no set, no reset, output low, no edge flag. */
    drive_and_step(0x00U);
    ok &= (read_latch() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* S pulse sets the latch and raises the rising-edge flag. */
    drive_and_step(0x03U);
    ok &= (read_latch() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) != 0U);

    /* Released S holds: output stays high with no new edge flag. */
    EPIC_REG8(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_CLC1IF;
    drive_and_step(0x00U);
    ok &= (read_latch() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* R pulse resets the latch and raises the falling-edge flag. */
    drive_and_step(0x0CU);
    ok &= (read_latch() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) != 0U);

    /* Released R holds low with no new flag. */
    EPIC_REG8(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_CLC1IF;
    drive_and_step(0x00U);
    ok &= (read_latch() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);

    /* S and R together: S wins, output goes high again. */
    drive_and_step(0x0FU);
    ok &= (read_latch() == 1U);
    EPIC_REG8(PIC_REG_PIR3) &= (uint8_t)~PIC_PIR3_CLC1IF;
    drive_and_step(0x00U);
    ok &= (read_latch() == 1U);

    /* Enabled interrupt dispatches the callback and clears the flag. */
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC1);
    EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_GIE;
    drive_and_step(0x0CU);
    ok &= (read_latch() == 0U);
    ok &= (s_calls == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_CLC1IF) == 0U);
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC1);

    /* Disabled cell reads 0 even with S driven. */
    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    drive_and_step(0x03U);
    ok &= (read_latch() == 0U);

    return epic_harness_report(ok);
}
