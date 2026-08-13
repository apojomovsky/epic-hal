/*
 * epic-tick target demo: 1 ms timebase toggling an LED at 500 ms.
 * Demonstrates epic_tick_init (the Timer2-based 1 ms timebase derived
 * from FOSC_HZ), the blocking epic_tick_delay_ms, and the non-blocking
 * epic_tick_elapsed_since timing idiom.
 */

#include <stdint.h>

#include "epic_tick.h"
#include "peripherals/hal_gpio.h"   /* EPIC_GPIO_* (family-neutral) */

/* Defined by the build; the fallback keeps the file parseable standalone. */
#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define BLINK_MS 500u   /* LED toggle period in milliseconds */

/**
 * @brief 1 ms timebase demo: LED on for 500 ms, then toggling at 500 ms.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    epic_tick_init(FOSC_HZ);        /* starts the 1 ms tick and enables GIE */

    /* Blocking delay: LED on for one period, then off. */
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    epic_tick_delay_ms(BLINK_MS);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* Elapsed-time check: non-blocking 500 ms period via the elapsed
     * idiom (unsigned subtraction, wraparound-safe). */
    uint32_t last = epic_tick_get();
    for (;;) {
        if (epic_tick_elapsed_since(last) >= BLINK_MS) {
            last = epic_tick_get();
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
    }
}
