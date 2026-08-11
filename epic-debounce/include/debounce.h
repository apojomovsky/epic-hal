/**
 * Vendor-agnostic, instantiable digital-input debouncer: one instance
 * per input, plain data, no global state. The caller supplies a
 * `debounce_read_fn` resolving active-high/low, so the module works
 * over a HAL GPIO pin, an I2C-expander bit, or a host-test mock.
 */

#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Pin-read callback. Returns `true` when the pin currently reads
 *         "active." The callback resolves active-high vs. active-low.
 */
typedef bool (*debounce_read_fn)(void *ctx);

/** Debounce edge events emitted by `debounce_poll`. */
typedef enum {
    DEBOUNCE_EVENT_NONE     = 0,  /**< no state change committed this poll */
    DEBOUNCE_EVENT_PRESSED,       /**< just became stably active */
    DEBOUNCE_EVENT_RELEASED,      /**< just became stably inactive */
} debounce_event_t;

/** Bit flags packed into `debounce_t.flags`, mirroring epic-taskmgr's
 *  `task_t.flags` convention. */
#define DEBOUNCE_FLAG_STABLE     0x01U  /**< current committed (debounced) state */
#define DEBOUNCE_FLAG_CANDIDATE  0x02U  /**< last raw read being watched for stability */

typedef struct {
    debounce_read_fn read;
    void            *read_ctx;
    uint16_t         debounce_ms;
    uint32_t         candidate_since; /**< epic_tick_get() timestamp of last raw change */
    uint8_t          flags;           /**< DEBOUNCE_FLAG_* */
} debounce_t;

/**
 * @brief  Initialize a debounce instance.
 * @param  db          the instance (caller-owned storage).
 * @param  read        pin-read callback (returns true = active).
 * @param  read_ctx    opaque context passed to `read` (may be NULL).
 * @param  debounce_ms stability window in ms (e.g. 20-50).
 * @note   Reads the pin once at init time and sets both the stable and
 *         candidate state to that initial reading, so a button already held
 *         down at boot does NOT spuriously fire a PRESSED event once the
 *         window elapses.
 */
void debounce_init(debounce_t *db, debounce_read_fn read, void *read_ctx,
                   uint16_t debounce_ms);

/**
 * @brief  Poll the input once. Call once per scheduler tick or main-loop
 *         iteration. Reads the pin via the callback, applies the debounce
 *         algorithm, and returns an edge event if the stable state just
 *         committed a transition.
 * @param db the debounce instance to poll.
 * @return `DEBOUNCE_EVENT_PRESSED`, `DEBOUNCE_EVENT_RELEASED`, or
 *         `DEBOUNCE_EVENT_NONE`.
 */
debounce_event_t debounce_poll(debounce_t *db);

/**
 * @brief  Query the last committed (debounced) state.
 * @param db the debounce instance to query.
 * @return `true` if the stable state is "active," `false` if "inactive."
 * @note   Reflects the committed stable state, not the raw/candidate state.
 */
bool debounce_is_active(const debounce_t *db);

#endif /* DEBOUNCE_H */
