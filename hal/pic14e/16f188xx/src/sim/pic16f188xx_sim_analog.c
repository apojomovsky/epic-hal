/**
 * Host model of the ADC, FVR, DAC1 and comparators. ADC finishes two
 * ticks after ADGO (sample in, ADGO clear, ADIF set) on manual, Timer0,
 * Timer2 and comparator triggers; FVRRDY follows FVREN; comparators
 * evaluate NSEL/PSEL in millivolts, update CxOUT/CMOUT and raise CxIF
 * on armed edges.
 */
#include <stdint.h>

#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "pic16f188xx_sim_priv.h"
#include "peripherals/pic16f188xx_adc.h"
#include "peripherals/pic16f188xx_dac.h"
#include "peripherals/pic16f188xx_fvr.h"
#include "peripherals/pic16f188xx_comp.h"

/* Simulated supply and FVR base step, in millivolts. */
#define SIM_VDD_MV 5000U
#define SIM_FVR_MV 1024U

/* Ticks from ADGO set to ADIF set. */
#define SIM_ADC_TICKS 2U

static uint16_t sim_adc_sample[64];
static uint8_t sim_adc_left;
static uint8_t sim_comp_prev[2];
static uint8_t sim_comp_rise[2];

/**
 * @brief Shorthand for one SFR cell.
 * @param addr SFR address.
 * @return Pointer to the raw cell.
 */
static uint8_t *cell(uint16_t addr)
{
    return &pic16f188xx_sim_sfr[addr];
}

/**
 * @brief Read a digital pin level as millivolts.
 * @param reg PORTx cell address.
 * @param pin pin number 0..7.
 * @return SIM_VDD_MV when the pin reads high, 0 otherwise.
 */
static uint16_t pin_mv(uint16_t reg, uint8_t pin)
{
    return ((*cell(reg) >> pin) & 1U) ? SIM_VDD_MV : 0U;
}

/**
 * @brief FVR buffer 2 output in millivolts (CDAFVR gain times 1.024V).
 * @return Buffer voltage, 0 when FVREN is clear or the buffer is off.
 */
static uint16_t fvr_buf2_mv(void)
{
    uint8_t fvrcon = *cell(PIC_REG_FVRCON);
    uint8_t gain = (uint8_t)((fvrcon & PIC_FVRCON_CDAFVR_MASK) >>
        PIC_FVRCON_CDAFVR_SHIFT);

    if (((fvrcon & PIC_FVRCON_FVREN) == 0U) || (gain == 0U))
    {
        return 0U;
    }
    return (uint16_t)(gain * SIM_FVR_MV);
}

/**
 * @brief DAC1 output in millivolts from DAC1CON0/1.
 * @return DAC voltage, 0 while the DAC is disabled.
 */
static uint16_t dac_mv(void)
{
    uint8_t con0 = *cell(PIC_REG_DAC1CON0);
    uint8_t code;
    uint16_t vpos;

    if ((con0 & PIC_DAC1CON0_EN) == 0U)
    {
        return 0U;
    }
    code = (uint8_t)(*cell(PIC_REG_DAC1CON1) & PIC_DAC1CON1_DAC1R_MASK);
    if (((con0 & PIC_DAC1CON0_PSS_MASK) >> PIC_DAC1CON0_PSS_SHIFT) ==
        (uint8_t)DAC_POSSRC_FVR)
    {
        vpos = fvr_buf2_mv();
    }
    else
    {
        vpos = SIM_VDD_MV;
    }
    return (uint16_t)(((uint32_t)code * vpos) / 32U);
}

/**
 * @brief Comparator negative source in millivolts (CMxNSEL, Register 18-3).
 * @param inst comparator instance 0 (C1) or 1 (C2).
 * @return Source voltage, 0 for unconnected codes.
 */
static uint16_t neg_mv(uint8_t inst)
{
    uint8_t nch = (uint8_t)(*cell(inst ? PIC_REG_CM2NSEL : PIC_REG_CM1NSEL) &
        PIC_CM1NSEL_NCH_MASK);

    switch (nch)
    {
    case 0U: return pin_mv(PIC_REG_PORTA, 0U);
    case 1U: return pin_mv(PIC_REG_PORTA, 1U);
    case 2U: return pin_mv(PIC_REG_PORTB, 3U);
    case 3U: return pin_mv(PIC_REG_PORTB, 1U);
    case 6U: return fvr_buf2_mv();
    default: return 0U;
    }
}

/**
 * @brief Comparator positive source in millivolts (CMxPSEL, Register 18-4).
 * @param inst comparator instance 0 (C1) or 1 (C2).
 * @return Source voltage, 0 for unconnected codes.
 */
static uint16_t pos_mv(uint8_t inst)
{
    uint8_t pch = (uint8_t)(*cell(inst ? PIC_REG_CM2PSEL : PIC_REG_CM1PSEL) &
        PIC_CM1PSEL_PCH_MASK);

    switch (pch)
    {
    case 0U: return pin_mv(PIC_REG_PORTA, 2U);
    case 1U: return (inst == 0U) ? pin_mv(PIC_REG_PORTA, 3U) : pin_mv(PIC_REG_PORTB, 0U);
    case 5U: return dac_mv();
    case 6U: return fvr_buf2_mv();
    default: return 0U;
    }
}

/**
 * @brief Current comparator output, post-polarity, model-only.
 * @param inst comparator instance 0 (C1) or 1 (C2).
 * @return 1 when high, 0 when low or disabled.
 */
static uint8_t comp_eval(uint8_t inst)
{
    uint16_t con0_addr = inst ? (uint16_t)PIC_REG_CM2CON0 : (uint16_t)PIC_REG_CM1CON0;
    uint8_t con0 = *cell(con0_addr);
    uint8_t out;

    if ((con0 & PIC_CM1CON0_ON) == 0U)
    {
        return 0U;
    }
    out = (pos_mv(inst) > neg_mv(inst)) ? 1U : 0U;
    if ((con0 & PIC_CM1CON0_POL) != 0U)
    {
        out ^= 1U;
    }
    return out;
}

/**
 * @brief Evaluate one comparator: OUT and CMOUT follow the inputs, CxIF
 *        sets on armed edges of the OUT bit.
 * @param inst comparator instance 0 (C1) or 1 (C2).
 */
static void comp_step(uint8_t inst)
{
    uint16_t con0_addr = inst ? (uint16_t)PIC_REG_CM2CON0 : (uint16_t)PIC_REG_CM1CON0;
    uint16_t con1_addr = inst ? (uint16_t)PIC_REG_CM2CON1 : (uint16_t)PIC_REG_CM1CON1;
    uint16_t pir_bit = inst ? (uint16_t)PIC_PIR2_C2IF : (uint16_t)PIC_PIR2_C1IF;
    uint16_t out_bit = inst ? (uint16_t)PIC_CMOUT_MC2OUT : (uint16_t)PIC_CMOUT_MC1OUT;
    uint8_t out = comp_eval(inst);

    if (out != 0U)
    {
        *cell(con0_addr) |= PIC_CM1CON0_OUT;
        *cell(PIC_REG_CMOUT) |= (uint8_t)out_bit;
    }
    else
    {
        *cell(con0_addr) &= (uint8_t)~PIC_CM1CON0_OUT;
        *cell(PIC_REG_CMOUT) &= (uint8_t)~out_bit;
    }
    sim_comp_rise[inst] = (uint8_t)((out != 0U) && (sim_comp_prev[inst] == 0U));
    if (sim_comp_rise[inst] != 0U)
    {
        if ((*cell(con1_addr) & PIC_CM1CON1_INTP) != 0U)
        {
            *cell(PIC_REG_PIR2) |= (uint8_t)pir_bit;
        }
    }
    if ((out == 0U) && (sim_comp_prev[inst] != 0U))
    {
        if ((*cell(con1_addr) & PIC_CM1CON1_INTN) != 0U)
        {
            *cell(PIC_REG_PIR2) |= (uint8_t)pir_bit;
        }
    }
    sim_comp_prev[inst] = out;
}
/**
 * @brief Latch a conversion result: ADRES from the injected sample for
 *        the selected channel, ADGO clear, ADIF set.
 */
static void adc_finish(void)
{
    uint8_t channel = (uint8_t)(*cell(PIC_REG_ADPCH) & PIC_ADPCH_CHS_MASK);
    uint16_t sample = sim_adc_sample[channel];

    if ((*cell(PIC_REG_ADCON0) & PIC_ADCON0_ADFM) != 0U)
    {
        *cell(PIC_REG_ADRESL) = (uint8_t)(sample & 0xFFU);
        *cell(PIC_REG_ADRESH) = (uint8_t)(sample >> 8);
    }
    else
    {
        *cell(PIC_REG_ADRESL) = (uint8_t)((sample << 6) & 0xFFU);
        *cell(PIC_REG_ADRESH) = (uint8_t)(sample >> 2);
    }
    *cell(PIC_REG_ADCON0) &= (uint8_t)~PIC_ADCON0_GO_NDONE;
    *cell(PIC_REG_PIR1) |= PIC_PIR1_ADIF;
}

/**
 * @brief Start a conversion when the ADC is on and idle.
 */
static void adc_start(void)
{
    if (((*cell(PIC_REG_ADCON0) & PIC_ADCON0_ADON) != 0U) &&
        ((*cell(PIC_REG_ADCON0) & PIC_ADCON0_GO_NDONE) == 0U))
    {
        *cell(PIC_REG_ADCON0) |= PIC_ADCON0_GO_NDONE;
        sim_adc_left = SIM_ADC_TICKS;
    }
}

/**
 * @brief Advance the ADC by one tick: manual starts, modelled
 *        auto-trigger sources, and the conversion countdown.
 * @param t0_pulse Non-zero on the tick Timer0 overflowed.
 * @param t2_pulse Non-zero on the tick Timer2 matched PR2.
 */
static void adc_step(uint8_t t0_pulse, uint8_t t2_pulse)
{
    uint8_t trigger;

    if (((*cell(PIC_REG_ADCON0) & PIC_ADCON0_ADON) != 0U &&
        (*cell(PIC_REG_ADCON0) & PIC_ADCON0_GO_NDONE) != 0U) &&
        (sim_adc_left == 0U))
    {
        sim_adc_left = SIM_ADC_TICKS;
    }
    trigger = (uint8_t)(*cell(PIC_REG_ADACT) & PIC_ADACT_ADACT_MASK);
    if ((trigger == (uint8_t)ADC_TRIGGER_TMR0) && (t0_pulse != 0U))
    {
        adc_start();
    }
    else if ((trigger == (uint8_t)ADC_TRIGGER_TMR2) && (t2_pulse != 0U))
    {
        adc_start();
    }
    else if ((trigger == (uint8_t)ADC_TRIGGER_C1) && (sim_comp_rise[0] != 0U))
    {
        adc_start();
    }
    else if ((trigger == (uint8_t)ADC_TRIGGER_C2) && (sim_comp_rise[1] != 0U))
    {
        adc_start();
    }
    else
    {
        /* Manual, PPS-pin and unmodelled sources: no hardware start. */
    }
    if ((*cell(PIC_REG_ADCON0) & PIC_ADCON0_ADON) == 0U)
    {
        sim_adc_left = 0U;
        return;
    }
    if (sim_adc_left > 0U)
    {
        sim_adc_left--;
        if (sim_adc_left == 0U)
        {
            adc_finish();
        }
    }
}

/**
 * @brief Advance the ADC, FVR and comparators by one tick.
 * @param t0_pulse Non-zero on the tick Timer0 overflowed.
 * @param t2_pulse Non-zero on the tick Timer2 matched PR2.
 */
void pic16f188xx_sim_analog_step(uint8_t t0_pulse, uint8_t t2_pulse)
{
    comp_step(0U);
    comp_step(1U);
    adc_step(t0_pulse, t2_pulse);
    if ((*cell(PIC_REG_FVRCON) & PIC_FVRCON_FVREN) != 0U)
    {
        *cell(PIC_REG_FVRCON) |= PIC_FVRCON_FVRRDY;
    }
    else
    {
        *cell(PIC_REG_FVRCON) &= (uint8_t)~PIC_FVRCON_FVRRDY;
    }
}

/**
 * @brief Clear the analog model state (samples hold, conversion idle).
 */
void pic16f188xx_sim_analog_reset(void)
{
    unsigned int i;

    for (i = 0U; i < sizeof sim_adc_sample / sizeof sim_adc_sample[0]; i++)
    {
        sim_adc_sample[i] = 0U;
    }
    sim_adc_left = 0U;
    sim_comp_prev[0] = 0U;
    sim_comp_prev[1] = 0U;
    sim_comp_rise[0] = 0U;
    sim_comp_rise[1] = 0U;
}

/**
 * @brief Inject the 10-bit sample an ADC channel converts.
 * @param channel ADPCH code 0..63.
 * @param value 10-bit sample (higher bits are ignored).
 */
void pic16f188xx_sim_adc_set_sample(uint8_t channel, uint16_t value)
{
    if (channel < sizeof sim_adc_sample / sizeof sim_adc_sample[0])
    {
        sim_adc_sample[channel] = (uint16_t)(value & 0x3FFU);
    }
}
