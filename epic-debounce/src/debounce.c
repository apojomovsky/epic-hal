/**
 * Poll-driven timestamp-comparison debounce, one implementation for
 * host, PIC16, and PIC18 alike.
 */

#include "debounce.h"
#include "epic_tick.h"

#ifdef __EPIC_CC__
// epic-cc: tick is HAL-3c (timer2); pure probe stubs time.
#undef epic_tick_get
#define epic_tick_get() ((uint32_t)0)
#undef epic_tick_elapsed_since
#define epic_tick_elapsed_since(x) ((uint32_t)0)
#endif
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
#ifdef __EPIC_CC__
    /* Stub: the real init calls read(read_ctx) once and the real poll
     * calls read() on every poll; isel cannot resolve an indirect call
     * through a struct-stored fn ptr (epic-cc#155). */
    db->read           = read;
    db->read_ctx       = read_ctx;
    db->debounce_ms    = debounce_ms;
    db->candidate_since = 0;
    db->flags = 0U;
    set_stable(&db->flags, false);
    set_candidate(&db->flags, false);
#else
    db->read           = read;
    db->read_ctx       = read_ctx;
    db->debounce_ms    = debounce_ms;
    db->candidate_since = epic_tick_get();
    bool initial = read(read_ctx);
    db->flags = 0U;
    set_stable(&db->flags, initial);
    set_candidate(&db->flags, initial);
#endif
}

/**
 * @brief Poll the input once (implementation).
 * @param db the instance to poll
 * @return event
 */
epic_debounce_event_t epic_debounce_poll(epic_debounce_t *db) {
#ifdef __EPIC_CC__
    /* Stub: epic-cc#155 (see epic_debounce_init above). */
    (void)db;
    return DEBOUNCE_EVENT_NONE;
#else
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
#endif
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
