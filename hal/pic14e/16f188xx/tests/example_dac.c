/**
 * DAC1 host-sim probe. Host-only: the DAC has no dynamic behavior, so
 * the probe pins the register images; the analog model reads DAC1CON0/1
 * back for the comparator and ADC paths. Not in the XC8 build (the mdb
 * gate probe is example_gpio_mdb.c). Covers handle validation, the Init
 * image, SetValue and the DeInit POR state.
 *
 * Expected register image (code 0x0F, VDD/VSS, no pin output):
 *   DAC1CON1 = 0x0F, DAC1CON0 = 0x80 (EN).
 * Full image (code 0x1F, FVR/VREF-, both pins): DAC1CON1 = 0x1F,
 * DAC1CON0 = 0xB9 (EN, OE1, OE2, PSS 10, NSS).
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_dac.h"

/**
 * @brief DAC1 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    DAC_HandleTypeDef h = DAC_HANDLE_DEFAULT;

    epic_harness_init(100UL);

    /* POR: DAC off, code 0. */
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x00U);

    /* Validation: NULL and oversized codes rejected, nothing written. */
    ok &= (EPIC_DAC_Init(NULL) == EPIC_INVALID);
    h.OutputValue = 0x20U;
    ok &= (EPIC_DAC_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0x00U);
    ok &= (EPIC_DAC_SetValue(0x20U) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x00U);

    /* Default image: mid code, VDD/VSS, internal use only. */
    h.OutputValue = 0x0FU;
    ok &= (EPIC_DAC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x0FU);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0x80U);

    /* Full image: top code, FVR/VREF-, both pin enables. */
    h.OutputValue = 0x1FU;
    h.PosSource = DAC_POSSRC_FVR;
    h.NegSource = DAC_NEGSRC_VREFPIN;
    h.OutputEnable = (uint8_t)(DAC_OUT1_ENABLE | DAC_OUT2_ENABLE);
    ok &= (EPIC_DAC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x1FU);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0xB9U);

    /* SetValue changes the code with the DAC running. */
    ok &= (EPIC_DAC_SetValue(0x00U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0xB9U);

    /* DeInit restores POR. */
    ok &= (EPIC_DAC_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_DAC1CON1) == 0x00U);

    epic_harness_log("dac: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
