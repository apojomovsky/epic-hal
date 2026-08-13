/*
 * epic-adcfilter target example: oversample-and-decimate plus a moving
 * average on one ADC channel (AN0), the averaged value logged over
 * serial. The read callback runs a full blocking conversion, so the
 * filter consumes real EPIC_ADC_* hardware samples.
 */

#include "epic_adcfilter.h"
#include "epic_tick.h"
#include "epic_serial.h"
#include "epic_hal.h"

#include <stdio.h>

#define ADC_EXTRA_BITS 2u       /* 4^2 = 16x oversampling, +2 bits */
#define AVG_WINDOW     16u
#define ADC_WAIT_SPINS 10000u   /* generous bound for one conversion */
#define LOG_PERIOD_MS  500u

static uint16_t g_avg_buf[AVG_WINDOW];
static epic_adcfilter_avg_t g_filter;

/**
 * @brief Take one blocking ADC conversion on the selected channel.
 */
static uint16_t read_adc_sample(void *ctx)
{
    (void)ctx;
    uint16_t spins = 0u;

    (void)EPIC_ADC_Start();
    while (!EPIC_ADC_IsConversionDone() && spins < ADC_WAIT_SPINS) {
        spins++;
    }
    EPIC_ADC_ClearITFlag();
    return EPIC_ADC_Read();
}

/**
 * @brief Oversample and moving-average one ADC channel, logged over serial.
 */
int main(void)
{
    epic_tick_init(FOSC_HZ);
    epic_serial_init(FOSC_HZ, 115200u);
    EPIC_IRQ_Restore(1);

    ADC_HandleTypeDef hadc = ADC_HANDLE_DEFAULT;   /* AN0, VDD/VSS refs */
    EPIC_ADC_Init(&hadc);

    epic_adcfilter_avg_init(&g_filter, g_avg_buf, AVG_WINDOW);

    printf("epic-adcfilter: AN0 oversampled 16x + %u-sample moving average\r\n",
           (unsigned)AVG_WINDOW);

    uint32_t last_log = epic_tick_get();
    for (;;) {
        uint16_t raw = epic_adcfilter_oversample(read_adc_sample, NULL, ADC_EXTRA_BITS);
        uint16_t avg = epic_adcfilter_avg_push(&g_filter, raw);
        if (epic_tick_elapsed_since(last_log) >= LOG_PERIOD_MS) {
            last_log = epic_tick_get();
            printf("raw=%u avg=%u\r\n", (unsigned)raw, (unsigned)avg);
        }
    }
}
