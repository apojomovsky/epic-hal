/**
 * Minimal, dependency-free demonstration of epic-fsm: no HAL, no task
 * manager, host-only, prints each transition to stdout.
 */

#include <stdio.h>
#include "fsm.h"

enum { ST_RED, ST_GREEN, ST_YELLOW };
enum { EV_TIMER };

static const char *state_name(fsm_state_t s)
{
    switch (s) {
    case ST_RED:    return "RED";
    case ST_GREEN:  return "GREEN";
    case ST_YELLOW: return "YELLOW";
    default:        return "?";
    }
}

static void on_enter_yellow(void *ctx)
{
    (void)ctx;
    printf("  (caution: entering YELLOW)\n");
}

static const fsm_transition_t light_transitions[] = {
    { ST_RED,    EV_TIMER, NULL, NULL,             ST_GREEN  },
    { ST_GREEN,  EV_TIMER, NULL, on_enter_yellow,  ST_YELLOW },
    { ST_YELLOW, EV_TIMER, NULL, NULL,             ST_RED    },
};

int main(void)
{
    fsm_t light;
    FSM_INIT(&light, light_transitions, ST_RED, NULL);

    printf("start: %s\n", state_name(fsm_state(&light)));
    for (int i = 0; i < 6; i++) {
        fsm_dispatch(&light, EV_TIMER);
        printf("TIMER -> %s\n", state_name(fsm_state(&light)));
    }
    return 0;
}
