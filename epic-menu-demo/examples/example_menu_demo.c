/*
 * epic-menu-demo target build: real PIC18F4550 hardware. Wires the
 * real RB<7:4> change interrupt to menu_demo_core's event queue, spawns
 * the cooperative-scheduler tasks, and runs forever. Config words come
 * from the manifest (see epic-common/manifest/modules.toml), not a
 * #pragma config block here.
 */

#include "menu_demo_core.h"

#include "epic_hal.h"
#include "epic_taskmgr.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define TICK_PRESCALER TIMER0_PRESCALER_1_256

#define TASK_PERIOD_ADC       5U    /* ~50 ms at a 10 ms tick */
#define TASK_PERIOD_UI        1U    /* every tick: responsive buttons  */
#define TASK_PERIOD_EEPROM    5U    /* ~50 ms: plenty for a self-timed write */
#define TASK_PERIOD_HEARTBEAT 100U  /* ~1 s */

/**
 * @brief RB-change callback: translate the button bits into queued events.
 *
 * Edge-triggered on whichever bits changed; a real board would also
 * debounce in hardware (RC filter) or here in software. Kept simple:
 * any observed low level on a button's bit after a change queues its
 * event (buttons are active-low with the internal pull-ups enabled in
 * menu_demo_init).
 */
static void on_button_change(uint8_t portb_value)
{
    if ((portb_value & GPIO_PIN_4) == 0U)
    {
        menu_demo_push_event(MENU_EVENT_UP);
    }
    if ((portb_value & GPIO_PIN_5) == 0U)
    {
        menu_demo_push_event(MENU_EVENT_DOWN);
    }
    if ((portb_value & GPIO_PIN_6) == 0U)
    {
        menu_demo_push_event(MENU_EVENT_SELECT);
    }
}

int main(void)
{
    menu_demo_init();
    EPIC_GPIO_RegisterChangeCallback(on_button_change);
    EPIC_IRQ_Enable(PIC18_IRQ_RB);

    epic_taskmgr_init();
    epic_taskmgr_spawn(menu_demo_task_adc,       NULL, TASK_PERIOD_ADC,       0U);
    epic_taskmgr_spawn(menu_demo_task_ui,        NULL, TASK_PERIOD_UI,        1U);
    epic_taskmgr_spawn(menu_demo_task_eeprom,    NULL, TASK_PERIOD_EEPROM,    2U);
    epic_taskmgr_spawn(menu_demo_task_heartbeat, NULL, TASK_PERIOD_HEARTBEAT, 3U);

    /* ~10 ms tick from FOSC_HZ, same derivation as
     * epic-taskmgr/examples/example_taskmgr.c: with the 1:256 prescaler
     * a tick takes (256 - reload) x 256 x 4 / FOSC_HZ seconds, so
     * 256 - reload = FOSC_HZ / 102400, clamped at 256 counts. */
    uint32_t counts = (uint32_t)FOSC_HZ / 102400U;
    if (counts > 256U)
    {
        counts = 256U;
    }
    epic_taskmgr_attach_timer0((uint8_t)(256U - counts), TICK_PRESCALER);
    EPIC_IRQ_Restore(1); /* arm Timer0 + RB-change interrupts */

    epic_taskmgr_run(); /* never returns */
    return 0;
}
