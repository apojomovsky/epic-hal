/**
 * @file    example_adcfilter_hal.c
 * @brief   Demonstrates pic8-adcfilter with a real HAL ADC read callback.
 *
 * @details
 *   Host-sim runnable (build with -DPIC8_ADCFILTER_BUILD_EPIC_EXAMPLE=ON).
 *   Wires `EPIC_ADC_Read` through the `pic8_adcfilter_read_fn` callback,
 *   oversamples 16x (extra_bits=2), then feeds the oversampled readings into
 *   a moving-average filter. On the host sim, ADC results are injected via
 *   the family sim's `*_sim_drive_adc_done` helper.
 */

#include "pic8_adcfilter.h"
#include "pic8_hal.h"
#include "core/pic8_harness.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_ADC(r) pic18_sim_drive_adc_done((uint16_t)(r))
#else
  #include "pic16f87xa_sim.h"
  #define SIM_ADC(r) pic16f87xa_sim_drive_adc_done((uint16_t)(r))
#endif

static uint16_t read_adc(void *ctx)
{
    (void)ctx;
    return EPIC_ADC_Read();
}

int main(void)
{
    pic8_harness_init(100000UL);

    ADC_HandleTypeDef hadc = ADC_HANDLE_DEFAULT;
    EPIC_ADC_Init(&hadc);

    uint16_t buf[8];
    pic8_adcfilter_avg_t filter;
    pic8_adcfilter_avg_init(&filter, buf, 8);

    for (int i = 0; i < 16; i++) {
        SIM_ADC(500 + i * 10);
        pic8_harness_tick();
        uint16_t oversampled = pic8_adcfilter_oversample(read_adc, NULL, 2);
        uint16_t avg = pic8_adcfilter_avg_push(&filter, oversampled);
        pic8_harness_log("sample %d: oversampled=%u avg=%u\n",
                         i, (unsigned)oversampled, (unsigned)avg);
    }
    return pic8_harness_report(1);
}
