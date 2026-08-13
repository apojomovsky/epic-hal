/*
 * epic-taskmgr target demo: two periodic tasks on the cooperative
 * scheduler. A blink task toggles the RB0 LED every 25 ticks; a
 * counter task counts scheduler ticks and toggles the RB1 LED every
 * 40 counts. The ~10 ms Timer0 tick is derived from FOSC_HZ and wired
 * to the scheduler with epic_taskmgr_attach_timer0.
 */

#include <stdint.h>

#include "epic_taskmgr.h"
#include "core/hal_irq.h"           /* EPIC_IRQ_Restore: enable GIE for TMR0 */
#include "peripherals/hal_gpio.h"   /* EPIC_GPIO_* (family-neutral) */

/* Defined by the build; the fallback keeps the file parseable standalone. */
#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define TICK_PRESCALER TIMER0_PRESCALER_1_256   /* the maximum prescaler */
#define BLINK_TICKS     25u   /* RB0 toggles every 25 ticks (~250 ms) */
#define COUNT_TICKS      5u   /* the counter task runs every 5 ticks */
#define COUNT_TARGET    40u   /* RB1 toggles every 40 counts (200 ticks) */

/** State carried through the counter task's spawn `arg`. */
typedef struct {
    uint16_t count;
} counter_arg_t;

static counter_arg_t g_counter = { 0u };

/**
 * @brief Blink task: toggle the RB0 LED every BLINK_TICKS.
 */
static void task_blink(void *arg)
{
    (void)arg;
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

/**
 * @brief Counter task: count scheduler ticks, toggle RB1 every COUNT_TARGET.
 */
static void task_counter(void *arg)
{
    counter_arg_t *c = (counter_arg_t *)arg;
    c->count++;
    if (c->count >= COUNT_TARGET) {
        c->count = 0u;
        EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
    }
}

/**
 * @brief Run a blink and a counter task on the cooperative scheduler.
 */
int main(void)
{
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_OUTPUT);

    epic_taskmgr_init();
    epic_taskmgr_spawn(task_blink, NULL, BLINK_TICKS, 0u);
    epic_taskmgr_spawn(task_counter, &g_counter, COUNT_TICKS, 1u);

    /* ~10 ms tick from FOSC_HZ: with the 1:256 prescaler a tick takes
     * (256 - reload) x 256 x 4 / FOSC_HZ seconds, so 256 - reload =
     * FOSC_HZ / 102400. Clamp at 256 counts: faster parts (48 MHz
     * PIC18) cannot reach 10 ms and just tick proportionally faster. */
    uint32_t counts = (uint32_t)FOSC_HZ / 102400u;
    if (counts > 256u) {
        counts = 256u;
    }
    epic_taskmgr_attach_timer0((uint8_t)(256u - counts), TICK_PRESCALER);
    EPIC_IRQ_Restore(1);   /* arm the Timer0 interrupt */

    epic_taskmgr_run();    /* the canonical scheduler loop, never returns */
    return 0;
}
