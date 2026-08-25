/* Pure footprint probe for epic-cc: no tick, no HAL, no indirect calls. */
#include "fsm.h"

static epic_fsm_transition_t transitions[2] = {
    { 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0 },
};
static epic_fsm_t g_fsm;

/** @brief Main. @return 0. */
int main(void)
{
    static int dummy_ctx = 0;
    epic_fsm_init(&g_fsm, transitions, 2, 0, &dummy_ctx);
    // No dispatch - avoids indirect call isel (epic-cc#73) while still
    // pulling fsm.c into the link for footprint. XC8 keeps the full
    // example/dispatch probe.
    return (int)epic_fsm_state(&g_fsm);
}
