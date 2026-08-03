/**
 * @file    fsm.h
 * @brief   Vendor-agnostic, table-driven finite state machine engine.
 *
 * @details
 *   A machine is one `static const fsm_transition_t[]` table plus one
 *   `fsm_t` handle. `fsm_dispatch()` scans top-to-bottom for the first row
 *   whose state (or `FSM_ANY_STATE`) and event match and whose guard (if
 *   any) passes, runs its action, and moves to `next_state`; a rejected
 *   guard falls through to the next matching row instead of stopping. No
 *   HAL dependency; `ctx` is an opaque `void *` each guard/action casts
 *   back to its real type. See the README for a worked example and
 *   docs/ARCHITECTURE.md for the design rationale.
 */

#ifndef FSM_H
#define FSM_H

#include <stddef.h>   /* NULL, used by every transition row's guard/action */
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Storage type for states and events, `uint8_t` by default (255
 *         reserved for @ref FSM_ANY_STATE). Override with
 *         `#define FSM_STATE_TYPE <type>` before including this header.
 */
#ifndef FSM_STATE_TYPE
#define FSM_STATE_TYPE uint8_t
#endif

typedef FSM_STATE_TYPE fsm_state_t;
typedef FSM_STATE_TYPE fsm_event_t;

/** Wildcard: a row with `state == FSM_ANY_STATE` matches any current state. */
#define FSM_ANY_STATE  ((fsm_state_t)-1)

/** Optional predicate gating a row; false lets @ref fsm_dispatch keep
 *  scanning for another match. NULL always allows the row. */
typedef bool (*fsm_guard_fn)(void *ctx);

/** Optional side effect run when a row fires, before the state changes.
 *  NULL means no side effect. */
typedef void (*fsm_action_fn)(void *ctx);

/** One row of a transition table; a whole machine is a `static const`
 *  array of these, declared once in the caller's file. */
typedef struct {
    fsm_state_t   state;       /**< Row applies from this state, or @ref FSM_ANY_STATE. */
    fsm_event_t   event;       /**< Row applies to this event. */
    fsm_guard_fn  guard;       /**< NULL = always allowed. */
    fsm_action_fn action;      /**< NULL = no side effect, just move state. */
    fsm_state_t   next_state;  /**< State to move to when this row fires. */
} fsm_transition_t;

/** A running machine instance. Multiple instances, even sharing one table,
 *  are fully independent; there is no shared mutable state. */
typedef struct {
    const fsm_transition_t *table;      /**< The machine's transition table (lives in flash). */
    uint8_t                 table_len;  /**< Number of rows in `table`. */
    fsm_state_t             state;      /**< Current state. */
    void                   *ctx;        /**< Opaque, passed to every guard/action. */
} fsm_t;

/**
 * @brief  Initialize a machine instance.
 *
 * @param  fsm            Instance to initialize.
 * @param  table           The transition table (must outlive `fsm`; a
 *                        `static const` array is the normal case).
 * @param  table_len       Number of rows in `table`. Prefer @ref FSM_INIT,
 *                        which computes this for you.
 * @param  initial_state   The machine's starting state.
 * @param  ctx            Opaque pointer passed to every guard/action;
 *                        may be NULL if none of them need it.
 */
void fsm_init(fsm_t *fsm, const fsm_transition_t *table, uint8_t table_len,
              fsm_state_t initial_state, void *ctx);

/**
 * @brief  Convenience wrapper over @ref fsm_init computing `table_len` via
 *         `sizeof(table) / sizeof(table[0])` at the call site.
 *
 * @warning `table` must be the actual array (not a decayed pointer):
 *          `sizeof` on a pointer silently gives the wrong length.
 */
#define FSM_INIT(fsm, table, initial_state, ctx) \
    fsm_init((fsm), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), \
             (initial_state), (ctx))

/**
 * @brief  Feed one event to the machine (see the file-level doc comment
 *         for the scan/guard/fall-through semantics).
 *
 * @return true if a row fired; false if none matched, leaving the state
 *         unchanged. No policy is imposed on an unhandled event (no
 *         logging, no assert); that decision is left to the caller.
 */
bool fsm_dispatch(fsm_t *fsm, fsm_event_t event);

/** Current state of the machine. */
fsm_state_t fsm_state(const fsm_t *fsm);

#endif /* FSM_H */
