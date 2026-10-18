/**
 * ADC host-sim probe. Host-only: conversion timing and triggers come
 * from the analog model through epic_harness_tick, so it is not in the
 * XC8 build (the mdb gate probe is example_gpio_mdb.c). Covers handle
 * validation, channel validation on both pin counts, the Init register
 * image, manual conversion in both justification modes, the ADIF vector
 * through the shared dispatcher, and a Timer2-match auto trigger.
 *
 * Expected register image (FRC, VDD/VSS, right justified, manual):
 *   ADCON0 = 0x94 (ADON, ADCS, ADFM), ADCON1 = 0x00, ADCON2 = 0x00,
 *   ADREF = 0x00, ADCLK = 0x00, ADACT = 0x00.
 * Conversion: injecting 0x2A5 on channel 0 reads ADRESH 0x02, ADRESL
 * 0xA5; left justified it reads ADRESH 0xA9, ADRESL 0x40.
 * The model completes a conversion two ticks after ADGO rises.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_adc.h"
#include "peripherals/pic16f188xx_timer2.h"

/**
 * @brief Wait for a conversion to start and finish, with a tick bound.
 * @details Arms on ADGO first, so a stale done from an earlier
 * conversion cannot pass the wait without a new trigger.
 * @param bound Maximum ticks per phase.
 * @return 1 when a conversion ran to done inside the bound, 0 otherwise.
 */
static uint8_t wait_done(unsigned int bound)
{
    unsigned int i;
    uint8_t started = 0U;

    for (i = 0U; i < bound; i++)
    {
        if ((EPIC_REG8(PIC_REG_ADCON0) & PIC_ADCON0_GO_NDONE) != 0U)
        {
            started = 1U;
            break;
        }
        epic_harness_tick();
    }
    for (i = 0U; (i < bound) && (started != 0U); i++)
    {
        epic_harness_tick();
        if (EPIC_ADC_IsConversionDone() != 0U)
        {
            return 1U;
        }
    }
    return 0U;
}

/**
 * @brief ADC checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    unsigned int i;
    int ok = 1;
    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

    epic_harness_init(10000UL);
    EPIC_IRQ_Restore(1U);

    /* POR: ADC off, manual trigger, channel 0. */
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x00U);
    ok &= (EPIC_ADC_IsConversionDone() != 0U);

    /* Validation: NULL handle rejected before anything is written. */
    ok &= (EPIC_ADC_Init(NULL) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x00U);

    /* Init image: FRC, VDD/VSS, right justified, manual trigger. */
    ok &= (EPIC_ADC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x94U);
    ok &= (EPIC_REG8(PIC_REG_ADCON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADCON2) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADREF) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADCLK) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADACT) == 0x00U);

    /* Channel validation: pin range is family-sized, internals fixed. */
    ok &= (EPIC_ADC_SelectChannel(0x00U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADPCH) == 0x00U);
    ok &= (EPIC_ADC_SelectChannel(PIC16F188XX_FAMILY_ADC_MAX_PIN) == EPIC_OK);
    ok &= (EPIC_ADC_SelectChannel((uint8_t)(PIC16F188XX_FAMILY_ADC_MAX_PIN + 1U)) ==
        EPIC_INVALID);
    ok &= (EPIC_ADC_SelectChannel(ADC_CHANNEL_AVSS) == EPIC_OK);
    ok &= (EPIC_ADC_SelectChannel(ADC_CHANNEL_TEMP) == EPIC_OK);
    ok &= (EPIC_ADC_SelectChannel(ADC_CHANNEL_DAC) == EPIC_OK);
    ok &= (EPIC_ADC_SelectChannel(ADC_CHANNEL_FVR) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADPCH) == ADC_CHANNEL_FVR);
    ok &= (EPIC_ADC_SelectChannel(0x23U) == EPIC_INVALID);
    ok &= (EPIC_ADC_SelectChannel(0x3BU) == EPIC_INVALID);
#if !PIC16F188XX_FAMILY_HAS_PORTD
    ok &= (EPIC_ADC_SelectChannel(0x18U) == EPIC_INVALID);
#else
    ok &= (EPIC_ADC_SelectChannel(0x18U) == EPIC_OK);
#endif

    /* Manual conversion, right justified: 0x2A5 lands split across ADRESH:L. */
    ok &= (EPIC_ADC_SelectChannel(0x00U) == EPIC_OK);
    pic16f188xx_sim_adc_set_sample(0x00U, 0x2A5U);
    EPIC_ADC_Start();
    ok &= (EPIC_ADC_IsConversionDone() == 0U);
    ok &= (wait_done(10U) != 0U);
    ok &= (EPIC_REG8(PIC_REG_ADRESH) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_ADRESL) == 0xA5U);
    ok &= (EPIC_ADC_Read() == 0x2A5U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_ADC) != 0U);

    /* Left justified: same sample reads back the same 10-bit value. */
    h.Format = ADC_FORMAT_LEFT_JUSTIFIED;
    ok &= (EPIC_ADC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x90U);
    EPIC_ADC_Start();
    ok &= (wait_done(10U) != 0U);
    ok &= (EPIC_REG8(PIC_REG_ADRESH) == 0xA9U);
    ok &= (EPIC_REG8(PIC_REG_ADRESL) == 0x40U);
    ok &= (EPIC_ADC_Read() == 0x2A5U);

    /* FOSC clock with a divider and VREF/FVR references program ADCLK/ADREF. */
    h.Format = ADC_FORMAT_RIGHT_JUSTIFIED;
    h.ClockSource = ADC_CLOCK_FOSC;
    h.ClockDiv = 0x0FU;
    h.PosRef = ADC_POSREF_FVR;
    h.NegRef = ADC_NEGREF_VREFPIN;
    ok &= (EPIC_ADC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x84U);
    ok &= (EPIC_REG8(PIC_REG_ADCLK) == 0x0FU);
    ok &= (EPIC_REG8(PIC_REG_ADREF) == 0x13U);

    /* ADIF vectors through the dispatcher and the handler clears it. */
    h.ClockSource = ADC_CLOCK_FRC;
    h.ClockDiv = 0U;
    h.PosRef = ADC_POSREF_VDD;
    h.NegRef = ADC_NEGREF_VSS;
    h.Format = ADC_FORMAT_RIGHT_JUSTIFIED;
    h.Trigger = ADC_TRIGGER_DISABLED;
    ok &= (EPIC_ADC_Init(&h) == EPIC_OK);
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_ADC);
    pic16f188xx_sim_adc_set_sample(0x00U, 0x100U);
    EPIC_ADC_Start();
    for (i = 0U; i < 10U; i++)
    {
        epic_harness_tick();
    }
    ok &= (EPIC_ADC_Read() == 0x100U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_ADC) == 0U);

    /* Timer2-match auto trigger: no EPIC_ADC_Start, the match converts. */
    h.Trigger = ADC_TRIGGER_TMR2;
    ok &= (EPIC_ADC_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADACT) == 0x04U);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_ADC);
    pic16f188xx_sim_adc_set_sample(0x00U, 0x3FFU);
    t2.Period = 1U;
    ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
    ok &= (wait_done(20U) != 0U);
    ok &= (EPIC_ADC_Read() == 0x3FFU);

    /* DeInit restores POR. */
    ok &= (EPIC_ADC_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_ADCON0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADCON1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADCON2) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADREF) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADCLK) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_ADACT) == 0x00U);

    epic_harness_log("adc: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
