# More API examples

Two programs beyond the [README](../README.md)'s blink and UART echo. The
scheduler program is family-neutral: the same source builds against any
supported family with only the include path swapped at build time. The ADC
program names its family's peripheral header (`pic16f87xa_adc.h`), which
changes per target; the filtering calls around it do not.

## Run two tasks on a cooperative scheduler

```c
#include <xc.h>
#include "epic_hal.h"
#include "epic_taskmgr.h"

static void toggle(void *arg)
{
    EPIC_GPIO_TogglePin(GPIOB, (uint16_t)(uintptr_t)arg);
}

int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_OUTPUT);
    epic_taskmgr_init();

    epic_taskmgr_spawn(toggle, (void *)(uintptr_t)GPIO_PIN_0, 100u, 0u);
    epic_taskmgr_spawn(toggle, (void *)(uintptr_t)GPIO_PIN_1, 300u, 0u);

    epic_taskmgr_attach_timer0(61u, TIMER0_PRESCALER_1_256); /* ~10 ms tick */
    EPIC_IRQ_Restore(1);                                     /* arm IRQs    */
    epic_taskmgr_run();                                      /* never returns */
}
```

`epic_taskmgr` is a priority-ordered, race-free cooperative scheduler:
periodic and one-shot tasks, `EPIC_TASKMGR_MAX_TASKS` fixed slots, no
per-task stack. The 10-line core of
[example_taskmgr](../lib/taskmgr/examples/example_taskmgr.c).

## Oversample and average an ADC channel

```c
#include <xc.h>
#include "peripherals/pic16f87xa_adc.h"
#include "epic_adcfilter.h"

static uint16_t read_ch3(void *ctx)
{
    (void)ctx;
    EPIC_ADC_SelectChannel(ADC_CHANNEL_AN3);
    EPIC_ADC_Start();
    while (EPIC_ADC_IsConversionInProgress()) { }
    return EPIC_ADC_Read();
}

int main(void)
{
    ADC_HandleTypeDef adc = ADC_HANDLE_DEFAULT;
    EPIC_ADC_Init(&adc);

    static uint16_t buf[8];
    epic_adcfilter_avg_t avg;
    epic_adcfilter_avg_init(&avg, buf, 8u);

    for (;;) {
        uint16_t v = epic_adcfilter_avg_push(
            &avg, epic_adcfilter_oversample(read_ch3, NULL, 1u));
        (void)v;
    }
}
```

`epic_adcfilter` decimates the raw samples and keeps an O(1) moving
average, both over a callback you provide. The HAL layer is just
select, start, poll, read.
