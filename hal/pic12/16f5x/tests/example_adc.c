/* 16F506 ADC probe (DS41268D §9.0, Register 9-1/9-2). Inits the
 * ADC from a handle, starts polled conversions on every CHS channel
 * against driven analog levels, then DeInits. 16F506-only: the sole
 * part with the analog block, so CMake and the manifest build this
 * file for 16F506 alone.
 *
 * Host: POR image, GPIO-cleared image, programmed images, driven
 * levels vs ADRES on AN0/AN1/AN2 plus the 0.6V slot, 13-TAD timing
 * boundaries, busy/ADON-off Start refusal, DeInit. Target
 * (1024-word budget): POR, programmed and DeInit images plus the
 * Start return on AN2 (RB2 stays an untouched input; SIM latches no
 * GO without an analog stimulus and models no 0.6V input, W0239,
 * so the host owns completion), then the PORTB:0 blink on pass
 * (the MODE=toggle gate watches it) or a dark halt on fail.
 *
 * Hand-computed images, from the GPIO-cleared block (all 0x00).
 *   host Init {AN1, FOSC_8}: 0xD5 (ADON|CHS=01|ADCS=01|ANS=11)
 *   host SelectChannel AN2: 0x59 (ADON|CHS=10|ADCS=01|ANS=01)
 *   host SelectChannel VREF: 0x5D (ANS kept at 01)
 *   host Init {AN0, FOSC_4}: 0xA1 (ADON|CHS=00|ADCS=10|ANS=10)
 *   target Init {AN2, FOSC_8}: 0x59 (same mux as the host select)
 *   DeInit: 0x00. POR ADCON0: 0xFC.
 */

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"
#include "peripherals/pic16f5x_adc.h"
#include "core/epic_harness.h"
#ifdef PIC16F5X_HOST
#include "pic16f5x_sim.h"
#endif

#ifdef PIC16F5X_HOST
/* POR byte for the host image check. */
uint8_t g_adc_por_ad;

static uint8_t g_fail = 0U;

/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index, logged as F characters.
 */
static void fail(uint8_t idx)
{
    uint8_t i = 0U;
    g_fail++;
    for (i = 0U; i <= idx; i++)
    {
        epic_harness_log("F");
    }
    epic_harness_log("\n");
}
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/**
 * @brief Run one polled conversion and read the result.
 * @param want the driven level the sim latches into ADRES.
 * @return the ADRES byte read after GO clears.
 */
static uint8_t adc_convert_once(uint8_t want)
{
    (void)want;
    CHECK(EPIC_ADC_Start() == 0U, 0x20U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, 0x21U);
    pic16f5x_sim_step(100U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, 0x22U);
    return EPIC_ADC_Read();
}
#endif

/**
 * @brief ADC probe: register images, driven conversions, DeInit.
 */
int main(void)
{
#ifdef PIC16F5X_HOST
    ADC_HandleTypeDef h = ADC_HANDLE_DEFAULT;

    epic_harness_init(1000U);
    g_adc_por_ad = EPIC_REG8(PIC_REG_ADCON0);

    /* POR image: analog selects set, ADC off (Table 4-2). */
    CHECK(g_adc_por_ad == 0xFCU, 0x00U);

    /* GPIO init clears the analog block to 0x00. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_MODE_INPUT);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x00U, 0x01U);

    /* NULL handle is rejected. */
    CHECK(EPIC_ADC_Init(NULL) == EPIC_INVALID, 0x02U);

    /* Init AN1 at Fosc/8: ADCON0 = 0xD5. */
    h.Channel = ADC_CHANNEL_AN1;
    h.ClockSource = ADC_CLOCK_FOSC_8;
    CHECK(EPIC_ADC_Init(&h) == EPIC_OK, 0x03U);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0xD5U, 0x04U);

    /* Driven AN1 level converts into ADRES. */
    pic16f5x_sim_drive_analog(1U, 0xA5U);
    CHECK(adc_convert_once(0xA5U) == 0xA5U, 0x05U);

    /* Busy Start is refused while GO is set. */
    CHECK(EPIC_ADC_Start() == 0U, 0x06U);
    CHECK(EPIC_ADC_Start() == 0xFFFFU, 0x07U);
    pic16f5x_sim_step(100U);
    CHECK(EPIC_ADC_Read() == 0xA5U, 0x08U);

    /* AN2 select re-muxes CHS and narrows ANS: 0x59. */
    EPIC_ADC_SelectChannel(ADC_CHANNEL_AN2);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x59U, 0x09U);
    pic16f5x_sim_drive_analog(2U, 0x3CU);
    CHECK(adc_convert_once(0x3CU) == 0x3CU, 0x0AU);

    /* 0.6V slot programs CHS only, ANS kept: 0x5D. */
    EPIC_ADC_SelectChannel(ADC_CHANNEL_VREF_0V6);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x5DU, 0x0BU);
    pic16f5x_sim_drive_analog(3U, 0x1FU);
    CHECK(adc_convert_once(0x1FU) == 0x1FU, 0x0CU);

    /* Fosc/4 converts in 13 instruction cycles: busy at 12, done
     * at 13. Init AN0 for the 0xA1 image. */
    h.Channel = ADC_CHANNEL_AN0;
    h.ClockSource = ADC_CLOCK_FOSC_4;
    CHECK(EPIC_ADC_Init(&h) == EPIC_OK, 0x0DU);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0xA1U, 0x0EU);
    pic16f5x_sim_drive_analog(0U, 0x7EU);
    CHECK(EPIC_ADC_Start() == 0U, 0x0FU);
    pic16f5x_sim_step(12U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 1U, 0x10U);
    pic16f5x_sim_step(1U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, 0x11U);
    CHECK(EPIC_ADC_Read() == 0x7EU, 0x12U);

    /* Start with ADON clear is refused, GO stays 0. */
    CHECK(EPIC_ADC_DeInit() == EPIC_OK, 0x13U);
    CHECK(EPIC_REG8(PIC_REG_ADCON0) == 0x00U, 0x14U);
    CHECK(EPIC_ADC_Start() == 0xFFFFU, 0x15U);
    CHECK(EPIC_ADC_IsConversionInProgress() == 0U, 0x16U);

    (void)epic_harness_report(g_fail == 0U);
    return g_fail == 0U ? 0 : 1;
#else
    uint8_t ok = 1U;
    uint8_t last_t0 = 0U;
    uint16_t poll = 5000U;
    TIMER0_HandleTypeDef h;
    ADC_HandleTypeDef a;
    /* Field-by-field handle setup (the example_blink shape): no
     * bulk initializer copy, every byte is an explicit store. AN2:
     * RB2 is an untouched input pin, and MPLAB SIM models no input
     * for the 0.6V channel (W0239-ADC), so the gate converts there. */
    a.Channel = ADC_CHANNEL_AN2;
    a.ClockSource = ADC_CLOCK_FOSC_8;

    /* POR image, read before any HAL call clears the block. */
    if (EPIC_REG8(PIC_REG_ADCON0) != 0xFCU) ok = 0U;

    /* Blink pin as output; clears the analog block. RB2 stays an
     * untouched input for the AN2 conversion below. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);

    /* Timer0 for the pass blink (example_blink shape). */
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.ClockEdge         = TIMER0_EDGE_RISING;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
    last_t0 = EPIC_TIMER0_ReadCounter();

    /* AN2 at Fosc/8 from the cleared block: 0x59. */
    if (EPIC_ADC_Init(&a) != EPIC_OK) ok = 0U;
    if (EPIC_REG8(PIC_REG_ADCON0) != 0x59U) ok = 0U;

    /* Start on silicon: the return proves the driver issued the GO
     * write (ADON set, GO clear). No GO-set assert: SIM latches no
     * GO without an analog stimulus (measured: ADCON0 never reads
     * 0x5B), so the host probe owns the poll/completion path. The
     * stuck-GO assert stays: it fails only if GO latches and never
     * clears, a genuine failure anywhere. DeInit returns analog-off. */
    if (EPIC_ADC_Start() != 0U) ok = 0U;
    while ((EPIC_ADC_IsConversionInProgress() != 0U) && (poll != 0U))
    {
        poll--;
    }
    if (EPIC_ADC_IsConversionInProgress() != 0U) ok = 0U;
    if (EPIC_ADC_DeInit() != EPIC_OK) ok = 0U;
    if (EPIC_REG8(PIC_REG_ADCON0) != 0x00U) ok = 0U;

    if (ok == 0U)
    {
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        for (;;)
        {
        }
    }
    for (;;)
    {
        uint8_t t0 = EPIC_TIMER0_ReadCounter();
        if (t0 < last_t0)
        {
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        last_t0 = t0;
    }
#endif
}
