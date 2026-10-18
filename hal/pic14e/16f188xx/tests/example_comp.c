/**
 * Comparator host-sim probe. Host-only: outputs and edges come from the
 * analog model through epic_harness_tick, so it is not in the XC8 build
 * (the mdb gate probe is example_gpio_mdb.c). Covers handle validation,
 * the Init register images, pin-driven output on C1, a DAC-vs-FVR
 * comparison on C2 with inversion, the edge interrupt through the shared
 * dispatcher with the instance callback, and the DeInit POR state.
 *
 * Expected register image (C1, RA2 vs RA0, no hysteresis, no inversion,
 * no interrupt): CM1NSEL = 0x00, CM1PSEL = 0x00, CM1CON1 = 0x00,
 * CM1CON0 = 0x80. With RA2 driven high the model sets CxOUT and
 * MC1OUT within one tick.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_comp.h"
#include "peripherals/pic16f188xx_dac.h"
#include "peripherals/pic16f188xx_fvr.h"

static unsigned int s_calls;

/**
 * @brief Event callback that records its call.
 */
static void comp_cb(void)
{
    s_calls++;
}

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Comparator checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    COMP_HandleTypeDef h = COMP_HANDLE_DEFAULT;
    DAC_HandleTypeDef dac = DAC_HANDLE_DEFAULT;
    FVR_HandleTypeDef fvr = FVR_HANDLE_DEFAULT;

    epic_harness_init(10000UL);
    EPIC_IRQ_Restore(1U);

    /* POR: both comparators off. */
    ok &= (EPIC_REG8(PIC_REG_CM1CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM2CON0) == 0x00U);

    /* Validation: NULL, bad instance and unconnected codes rejected. */
    ok &= (EPIC_COMP_Init(NULL) == EPIC_INVALID);
    h.Instance = (COMP_InstanceTypeDef)3;
    ok &= (EPIC_COMP_Init(&h) == EPIC_INVALID);
    h.Instance = COMP_INSTANCE_1;
    h.PosChannel = (COMP_PosChannelTypeDef)2;
    ok &= (EPIC_COMP_Init(&h) == EPIC_INVALID);
    h.PosChannel = COMP_POS_PIN0;
    h.NegChannel = (COMP_NegChannelTypeDef)4;
    ok &= (EPIC_COMP_Init(&h) == EPIC_INVALID);
    h.NegChannel = COMP_NEG_PIN0;
    ok &= (EPIC_REG8(PIC_REG_CM1CON0) == 0x00U);
    ok &= (EPIC_COMP_DeInit((COMP_InstanceTypeDef)3) == EPIC_INVALID);

    /* C1 on pins: RA2 high against RA0 low drives the output high. */
    ok &= (EPIC_COMP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CM1NSEL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1PSEL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1CON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1CON0) == 0x80U);
    pic16f188xx_sim_drive_input('A', 2U, 1U);
    step(1U);
    ok &= (EPIC_COMP_ReadOutput(COMP_INSTANCE_1) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_CM1CON0) & PIC_CM1CON0_OUT) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_CMOUT) & PIC_CMOUT_MC1OUT) != 0U);
    pic16f188xx_sim_drive_input('A', 2U, 0U);
    step(1U);
    ok &= (EPIC_COMP_ReadOutput(COMP_INSTANCE_1) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_CMOUT) & PIC_CMOUT_MC1OUT) == 0U);

    /* C2 DAC against FVR with inversion: DAC top at VDD beats FVR 4x,
     * so the raw output is high and the inverted read is low. */
    fvr.CompDacGain = FVR_GAIN_4X;
    ok &= (EPIC_FVR_Init(&fvr) == EPIC_OK);
    dac.OutputValue = 0x1FU;
    ok &= (EPIC_DAC_Init(&dac) == EPIC_OK);
    h.Instance = COMP_INSTANCE_2;
    h.PosChannel = COMP_POS_DAC;
    h.NegChannel = COMP_NEG_FVR;
    h.InvertOutput = 1U;
    h.HysteresisOn = 1U;
    h.InterruptEdge = COMP_INT_EDGE_BOTH;
    h.EventCallback = comp_cb;
    ok &= (EPIC_COMP_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CM2NSEL) == 0x06U);
    ok &= (EPIC_REG8(PIC_REG_CM2PSEL) == 0x05U);
    ok &= (EPIC_REG8(PIC_REG_CM2CON1) == 0x03U);
    ok &= (EPIC_REG8(PIC_REG_CM2CON0) == 0x92U);
    step(1U);
    ok &= (EPIC_COMP_ReadOutput(COMP_INSTANCE_2) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_CM2CON0) & PIC_CM2CON0_OUT) == 0U);

    /* The OUT edge vectors through the dispatcher, runs the slot and
     * clears C2IF; dropping the DAC code flips the raw input low so the
     * inverted OUT bit rises. */
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_CMP2);
    s_calls = 0U;
    ok &= (EPIC_DAC_SetValue(0x00U) == EPIC_OK);
    step(2U);
    ok &= (EPIC_COMP_ReadOutput(COMP_INSTANCE_2) != 0U);
    step(2U);
    ok &= (s_calls != 0U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_CMP2) == 0U);

    /* Unknown instance reads low. */
    ok &= (EPIC_COMP_ReadOutput((COMP_InstanceTypeDef)3) == 0U);

    /* DeInit restores POR and drops the callback. */
    ok &= (EPIC_COMP_DeInit(COMP_INSTANCE_1) == EPIC_OK);
    ok &= (EPIC_COMP_DeInit(COMP_INSTANCE_2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CM1CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1CON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1NSEL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM1PSEL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM2CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM2CON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM2NSEL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CM2PSEL) == 0x00U);

    epic_harness_log("comp: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
