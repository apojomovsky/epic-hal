/**
 * Minimal on-target build proving fsm.c cross-compiles for real
 * XC8/PIC16/PIC18 silicon and reports flash/RAM footprint; not a
 * correctness test (see ../tests/test_fsm.c).
 */

#include "fsm.h"

enum { ST_A, ST_B };
enum { EV_GO };

static const fsm_transition_t transitions[] = {
    { ST_A, EV_GO, NULL, NULL, ST_B },
    { ST_B, EV_GO, NULL, NULL, ST_A },
};

static fsm_t g_fsm;

/**
 * @brief On-target build proof and footprint report.
 *
 * Runs the machine forever on real silicon; never returns.
 *
 * @return never returns on target
 */
int main(void)
{
    FSM_INIT(&g_fsm, transitions, ST_A, NULL);
    for (;;) {
        fsm_dispatch(&g_fsm, EV_GO);
    }
}
