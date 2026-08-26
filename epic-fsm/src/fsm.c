/**
 * Implementation of the table-driven FSM engine (see fsm.h). No hardware
 * dependency: one implementation compiles unchanged for host, PIC16, PIC18.
 */

#include "fsm.h"

/**
 * @brief Initialize a machine instance (see fsm.h).
 *
 * @param fsm            the machine instance to initialize
 * @param table          the transition table backing the machine
 * @param table_len      number of rows in `table`
 * @param initial_state  state the machine starts in
 * @param ctx            opaque context passed to guards/actions, or NULL
 */
void epic_fsm_init(epic_fsm_t *fsm, const epic_fsm_transition_t *table, uint8_t table_len,
              epic_fsm_state_t initial_state, void *ctx)
{
    fsm->table     = table;
    fsm->table_len = table_len;
    fsm->state     = initial_state;
    fsm->ctx       = ctx;
}

/**
 * @brief Feed one event to the machine (see fsm.h).
 *
 * @param fsm    the machine to dispatch into
 * @param event  the event to feed
 * @return true if a row fired; false if none matched
 */
bool epic_fsm_dispatch(epic_fsm_t *fsm, epic_fsm_event_t event)
{
#ifdef __EPIC_CC__
    /* Stub: the real body reads guard/action fn ptrs out of a static
     * const transition table; irparse cannot decode a const struct
     * field whose value is a function symbol (epic-cc#154). */
    (void)fsm; (void)event;
    return false;
#else
    uint8_t i; // pure probe

    for (i = 0; i < fsm->table_len; i++) {
        const epic_fsm_transition_t *row = &fsm->table[i];

        if ((row->state != fsm->state) && (row->state != EPIC_FSM_ANY_STATE)) {
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
#endif
}

/**
 * @brief Current state of the machine (see fsm.h).
 *
 * @param fsm the machine to query
 * @return the state the machine is currently in
 */
epic_fsm_state_t epic_fsm_state(const epic_fsm_t *fsm)
{
    return fsm->state;
}
