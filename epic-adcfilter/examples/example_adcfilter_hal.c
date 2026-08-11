/**
 * Demonstrates epic-adcfilter with a real HAL ADC read callback
 * (host-sim runnable, -DEPIC_ADCFILTER_BUILD_EPIC_EXAMPLE=ON): wires
 * EPIC_ADC_Read through the read callback, oversamples 16x
 * (extra_bits=2), then feeds the results into a moving-average
 * filter. On the host sim, ADC results are injected via the family
 * sim's *_sim_drive_adc_done helper.
 */

#include "epic_adcfilter.h"
#include "epic_hal.h"
#include "core/epic_harness.h"

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
    epic_harness_init(100000UL);

    ADC_HandleTypeDef hadc = ADC_HANDLE_DEFAULT;
    EPIC_ADC_Init(&hadc);

    uint16_t buf[8];
    epic_adcfilter_avg_t filter;
    epic_adcfilter_avg_init(&filter, buf, 8);

    for (int i = 0; i < 16; i++) {
        SIM_ADC(500 + i * 10);
        epic_harness_tick();
        uint16_t oversampled = epic_adcfilter_oversample(read_adc, NULL, 2);
        uint16_t avg = epic_adcfilter_avg_push(&filter, oversampled);
        epic_harness_log("sample %d: oversampled=%u avg=%u\n",
                         i, (unsigned)oversampled, (unsigned)avg);
    }
    return epic_harness_report(1);
}
