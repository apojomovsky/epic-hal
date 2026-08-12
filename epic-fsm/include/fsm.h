/**
 * Vendor-agnostic, table-driven FSM engine: one `static const
 * epic_fsm_transition_t[]` table plus one `epic_fsm_t` handle per machine. No HAL
 * dependency; `ctx` is an opaque `void *` each guard/action casts back.
 * See README for a worked example and docs/API.md for the full surface.
 */

#ifndef FSM_H
#define FSM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Storage type for states and events; `uint8_t` by default (255 reserved
 * for EPIC_FSM_ANY_STATE). Override with `#define EPIC_FSM_STATE_TYPE <type>` before
 * including this header.
 */
#ifndef EPIC_FSM_STATE_TYPE
#define EPIC_FSM_STATE_TYPE uint8_t
#endif

typedef EPIC_FSM_STATE_TYPE epic_fsm_state_t;
typedef EPIC_FSM_STATE_TYPE epic_fsm_event_t;

/** Wildcard: a row with `state == EPIC_FSM_ANY_STATE` matches any current state. */
#define EPIC_FSM_ANY_STATE  ((epic_fsm_state_t)-1)

/** Optional predicate gating a row; false lets @ref epic_fsm_dispatch keep
 *  scanning for another match. NULL always allows the row. */
typedef bool (*epic_fsm_guard_fn)(void *ctx);

/** Optional side effect run when a row fires, before the state changes.
 *  NULL means no side effect. */
typedef void (*epic_fsm_action_fn)(void *ctx);

/** One row of a transition table; a whole machine is a `static const`
 *  array of these, declared once in the caller's file. */
typedef struct {
    epic_fsm_state_t   state;
    epic_fsm_event_t   event;
    epic_fsm_guard_fn  guard;
    epic_fsm_action_fn action;
    epic_fsm_state_t   next_state;
} epic_fsm_transition_t;

/** A running machine instance. Multiple instances, even sharing one table,
 *  are fully independent; there is no shared mutable state. */
typedef struct {
    const epic_fsm_transition_t *table;      /**< The machine's transition table (lives in flash). */
    uint8_t                 table_len;
    epic_fsm_state_t             state;
    void                   *ctx;
} epic_fsm_t;

/**
 * @brief Initialize a machine instance.
 *
 * `table` must outlive `fsm` (a `static const` array is the normal case).
 * Prefer EPIC_FSM_INIT, which computes `table_len` for you. `ctx` may be NULL
 * if no guard/action needs it.
 *
 * @param fsm            the machine instance to initialize
 * @param table          the transition table backing the machine
 * @param table_len      number of rows in `table`
 * @param initial_state  state the machine starts in
 * @param ctx            opaque context passed to guards/actions, or NULL
 */
void epic_fsm_init(epic_fsm_t *fsm, const epic_fsm_transition_t *table, uint8_t table_len,
              epic_fsm_state_t initial_state, void *ctx);

/** Convenience wrapper over epic_fsm_init computing `table_len` via
 *  sizeof(table)/sizeof(table[0]) at the call site. `table` must be the
 *  actual array, not a decayed pointer: sizeof on a pointer silently
 *  gives the wrong length. */
#define EPIC_FSM_INIT(fsm, table, initial_state, ctx) \
    epic_fsm_init((fsm), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), \
             (initial_state), (ctx))

/**
 * @brief Feed one event to the machine.
 *
 * Scans the table top-to-bottom for the first row whose state (or
 * EPIC_FSM_ANY_STATE) and event match and whose guard (if any) passes, runs
 * its action, and moves to next_state; a rejected guard falls through
 * to the next matching row.
 *
 * @param fsm    the machine to dispatch into
 * @param event  the event to feed
 *
 * @return true if a row fired; false if none matched, leaving the state
 *         unchanged. No policy is imposed on an unhandled event (no
 *         logging, no assert); that decision is left to the caller.
 */
bool epic_fsm_dispatch(epic_fsm_t *fsm, epic_fsm_event_t event);

/**
 * @brief Current state of the machine.
 *
 * @param fsm the machine to query
 * @return the state the machine is currently in
 */
epic_fsm_state_t epic_fsm_state(const epic_fsm_t *fsm);

#endif /* FSM_H */
