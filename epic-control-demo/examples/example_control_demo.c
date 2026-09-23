/*
 * epic-control-demo target build: real PIC18F4550 hardware. Spawns the
 * shared core's cooperative-scheduler tasks and runs forever. Config
 * words come from the manifest, not a #pragma config block here.
 */

#include "control_demo_core.h"

#include "epic_hal.h"
#include "epic_taskmgr.h"

#include <stdint.h>

#ifndef FOSC_HZ
#define FOSC_HZ 48000000UL
#endif

#define TICK_PRESCALER TIMER0_PRESCALER_1_256

#define TASK_PERIOD_CONTROL   10U   /* ~100 ms at a 10 ms tick */
#define TASK_PERIOD_CONSOLE   1U    /* every tick: responsive console */
#define TASK_PERIOD_EEPROM    5U    /* ~50 ms: plenty for a self-timed write */
#define TASK_PERIOD_HEARTBEAT 100U  /* ~1 s */

/**
 * @brief Init the demo and run the cooperative scheduler forever.
 * @return never returns
 */
int main(void)
{
    control_demo_init();

    epic_taskmgr_init();
    epic_taskmgr_spawn(control_demo_task_control,   NULL, TASK_PERIOD_CONTROL,   0U);
    epic_taskmgr_spawn(control_demo_task_console,   NULL, TASK_PERIOD_CONSOLE,   1U);
    epic_taskmgr_spawn(control_demo_task_eeprom,    NULL, TASK_PERIOD_EEPROM,    2U);
    epic_taskmgr_spawn(control_demo_task_heartbeat, NULL, TASK_PERIOD_HEARTBEAT, 3U);

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
    EPIC_IRQ_Restore(1); /* arm the Timer0 + USART interrupts */

    epic_taskmgr_run(); /* never returns */
    return 0;
}
