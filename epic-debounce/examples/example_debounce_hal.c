/**
 * @file    example_debounce_hal.c
 * @brief   Two debounce instances on two HAL GPIO pins (RA0/RA1), a press
 *          on either toggles the LED on RB0.
 *
 * @details
 *   The read callback wraps `EPIC_GPIO_ReadPin` through a small `pin_ctx_t`,
 *   so the debounce core never sees a HAL type. Host sim drives RA0/RA1
 *   high/low to simulate presses; on target the pins read real switches.
 */

#include "debounce.h"
#include "epic_tick.h"
#include "epic_hal.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_CYCLES 3000000UL
#define DB_MS      20u

/* The pin context the read callback uses, demonstrates "any gpio": the
 * debounce core never sees GPIO_TypeDef, only the bool the callback returns. */
typedef struct { uint8_t port; uint16_t pin; } pin_ctx_t;

static bool read_pin(void *ctx)
{
    pin_ctx_t *p = (pin_ctx_t *)ctx;
    return EPIC_GPIO_ReadPin((GPIO_TypeDef)p->port, p->pin) == GPIO_PIN_SET;
}

int main(void)
{
    epic_harness_init(SIM_CYCLES);
    epic_tick_init(FOSC_HZ);

    /* Two button inputs on RA0, RA1; one LED output on RB0. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_INPUT);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    pin_ctx_t pin_a = { GPIOA, GPIO_PIN_0 };
    pin_ctx_t pin_b = { GPIOA, GPIO_PIN_1 };
    debounce_t db_a, db_b;
    debounce_init(&db_a, read_pin, &pin_a, DB_MS);
    debounce_init(&db_b, read_pin, &pin_b, DB_MS);

    int events = 0;
    for (uint32_t i = 0; epic_harness_running(i); i++) {
        /* Simulate a button press on RA0 around tick 50, release at tick 100. */
        uint32_t t = epic_tick_get();
        if (t == 50u) { EPIC_REG8(PIC_REG_PORTA) |=  (uint8_t)GPIO_PIN_0; }
        if (t == 100u){ EPIC_REG8(PIC_REG_PORTA) &= ~(uint8_t)GPIO_PIN_0; }

        epic_harness_tick();

        debounce_event_t ea = debounce_poll(&db_a);
        debounce_event_t eb = debounce_poll(&db_b);
        if (ea != DEBOUNCE_EVENT_NONE) {
            epic_harness_log("[t=%lu] A: %s\n", (unsigned long)t,
                             ea == DEBOUNCE_EVENT_PRESSED ? "PRESSED" : "RELEASED");
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            events++;
        }
        if (eb != DEBOUNCE_EVENT_NONE) {
            epic_harness_log("[t=%lu] B: %s\n", (unsigned long)t,
                             eb == DEBOUNCE_EVENT_PRESSED ? "PRESSED" : "RELEASED");
            events++;
        }
    }

    epic_harness_log("debounce example: %d events\n", events);
    return epic_harness_report(events >= 2);  /* at least press + release on A */
}
