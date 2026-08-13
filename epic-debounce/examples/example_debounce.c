/* Two debounced buttons on epic-debounce: each drives its own LED, a
 * PRESSED event lights it and a RELEASED event turns it off, with a
 * 20 ms stability window from the epic-tick timebase. Wiring: buttons
 * on RB0/RB1 to VCC with 10k pull-downs (active high); LEDs on
 * RB2/RB3 to GND via resistors. */

#include <stdint.h>

#include "debounce.h"
#include "epic_tick.h"
#include "epic_hal.h"

#define DB_MS 20u

typedef struct {
    GPIO_TypeDef port;
    uint16_t     pin;
} button_pin_t;

/** @brief Read one button level; true means pressed (active high). */
static bool read_button(void *ctx)
{
    button_pin_t *b = (button_pin_t *)ctx;
    return EPIC_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_SET;
}

/** @brief Poll one debounced button and mirror its edge events on its LED. */
static void drive_led(epic_debounce_t *db, GPIO_TypeDef port, uint16_t pin)
{
    epic_debounce_event_t ev = epic_debounce_poll(db);
    if (ev == DEBOUNCE_EVENT_PRESSED) {
        EPIC_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    } else if (ev == DEBOUNCE_EVENT_RELEASED) {
        EPIC_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Poll both debounced buttons forever, mirroring them on the LEDs.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_INPUT);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_OUTPUT);
    epic_tick_init(FOSC_HZ);
    EPIC_IRQ_Restore(1);

    button_pin_t btn_a = { GPIOB, GPIO_PIN_0 };
    button_pin_t btn_b = { GPIOB, GPIO_PIN_1 };

    epic_debounce_t db_a, db_b;
    epic_debounce_init(&db_a, read_button, &btn_a, DB_MS);
    epic_debounce_init(&db_b, read_button, &btn_b, DB_MS);

    /* A button held at boot commits its state without a spurious PRESSED
     * event; seed the LEDs from the committed state up front. */
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_2,
                       epic_debounce_is_active(&db_a) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_3,
                       epic_debounce_is_active(&db_b) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    for (;;) {
        drive_led(&db_a, GPIOB, GPIO_PIN_2);
        drive_led(&db_b, GPIOB, GPIO_PIN_3);
    }
}
