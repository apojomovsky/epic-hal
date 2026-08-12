/**
 * Poll-driven timestamp-comparison debounce, one implementation for
 * host, PIC16, and PIC18 alike.
 */

#include "debounce.h"
#include "epic_tick.h"

/**
 * @brief  Read the candidate (last raw) state flag.
 * @param flags the instance flag byte.
 * @return true when the candidate state is active.
 */
static inline bool get_candidate(uint8_t flags)
{
    return (flags & DEBOUNCE_FLAG_CANDIDATE) != 0U;
}

/**
 * @brief  Read the stable (committed) state flag.
 * @param flags the instance flag byte.
 * @return true when the stable state is active.
 */
static inline bool get_stable(uint8_t flags)
{
    return (flags & DEBOUNCE_FLAG_STABLE) != 0U;
}

/**
 * @brief  Set or clear the candidate (last raw) state flag.
 * @param flags pointer to the instance flag byte.
 * @param val   the new candidate state.
 */
static inline void set_candidate(uint8_t *flags, bool val)
{
    if (val) { *flags |= DEBOUNCE_FLAG_CANDIDATE; }
    else     { *flags &= (uint8_t)~DEBOUNCE_FLAG_CANDIDATE; }
}

/**
 * @brief  Set or clear the stable (committed) state flag.
 * @param flags pointer to the instance flag byte.
 * @param val   the new stable state.
 */
static inline void set_stable(uint8_t *flags, bool val)
{
    if (val) { *flags |= DEBOUNCE_FLAG_STABLE; }
    else     { *flags &= (uint8_t)~DEBOUNCE_FLAG_STABLE; }
}

/**
 * @brief  Initialize a debounce instance (implementation).
 * @param db          the instance to initialize.
 * @param read        pin-read callback (returns true = active).
 * @param read_ctx    opaque context passed to `read` (may be NULL).
 * @param debounce_ms stability window in ms.
 */
void epic_debounce_init(epic_debounce_t *db, epic_debounce_read_fn read, void *read_ctx,
                   uint16_t debounce_ms)
{
    db->read           = read;
    db->read_ctx       = read_ctx;
    db->debounce_ms    = debounce_ms;
    db->candidate_since = epic_tick_get();
    bool initial = read(read_ctx);
    db->flags = 0U;
    set_stable(&db->flags, initial);
    set_candidate(&db->flags, initial);
}

/**
 * @brief  Poll the input once (implementation).
 * @param db the instance to poll.
 * @return `DEBOUNCE_EVENT_PRESSED`, `DEBOUNCE_EVENT_RELEASED`, or
 *         `DEBOUNCE_EVENT_NONE`.
 */
epic_debounce_event_t epic_debounce_poll(epic_debounce_t *db)
{
    bool raw       = db->read(db->read_ctx);
    bool candidate = get_candidate(db->flags);
    bool stable    = get_stable(db->flags);

    if (raw != candidate) {
        set_candidate(&db->flags, raw);
        db->candidate_since = epic_tick_get();
        return DEBOUNCE_EVENT_NONE;
    }

    if (raw != stable &&
        epic_tick_elapsed_since(db->candidate_since) >= (uint32_t)db->debounce_ms) {
        set_stable(&db->flags, raw);
        return raw ? DEBOUNCE_EVENT_PRESSED : DEBOUNCE_EVENT_RELEASED;
    }

    return DEBOUNCE_EVENT_NONE;
}

/**
 * @brief  Query the committed stable state (implementation).
 * @param db the instance to query.
 * @return true when the stable state is active.
 */
bool epic_debounce_is_active(const epic_debounce_t *db)
{
    return get_stable(db->flags);
}
