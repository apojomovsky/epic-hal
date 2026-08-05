/**
 * @file    fsm.c
 * @brief   Implementation of the table-driven FSM engine (see fsm.h).
 *
 * @details
 *   One implementation, no per-family variant: this file has no hardware
 *   dependency, so it compiles unchanged for the host, PIC16, and PIC18.
 */

#include "fsm.h"

void fsm_init(fsm_t *fsm, const fsm_transition_t *table, uint8_t table_len,
              fsm_state_t initial_state, void *ctx)
{
    fsm->table     = table;
    fsm->table_len = table_len;
    fsm->state     = initial_state;
    fsm->ctx       = ctx;
}

bool fsm_dispatch(fsm_t *fsm, fsm_event_t event)
{
    uint8_t i;

    for (i = 0; i < fsm->table_len; i++) {
        const fsm_transition_t *row = &fsm->table[i];

        if ((row->state != fsm->state) && (row->state != FSM_ANY_STATE)) {
            continue;
        }
        if (row->event != event) {
            continue;
        }
        if ((row->guard != NULL) && !row->guard(fsm->ctx)) {
            continue;  /* guard rejected: keep scanning for another matching row */
        }

        if (row->action != NULL) {
            row->action(fsm->ctx);
        }
        fsm->state = row->next_state;
        return true;
    }

    return false;
}

fsm_state_t fsm_state(const fsm_t *fsm)
{
    return fsm->state;
}
