/**
 * Poll-driven timestamp-comparison debounce, one implementation for
 * host, PIC16, and PIC18 alike.
 */

#include "debounce.h"
#include "epic_tick.h"

static inline bool get_candidate(uint8_t flags)
{
    return (flags & DEBOUNCE_FLAG_CANDIDATE) != 0U;
}

static inline bool get_stable(uint8_t flags)
{
    return (flags & DEBOUNCE_FLAG_STABLE) != 0U;
}

static inline void set_candidate(uint8_t *flags, bool val)
{
    if (val) { *flags |= DEBOUNCE_FLAG_CANDIDATE; }
    else     { *flags &= (uint8_t)~DEBOUNCE_FLAG_CANDIDATE; }
}

static inline void set_stable(uint8_t *flags, bool val)
{
    if (val) { *flags |= DEBOUNCE_FLAG_STABLE; }
    else     { *flags &= (uint8_t)~DEBOUNCE_FLAG_STABLE; }
}

void debounce_init(debounce_t *db, debounce_read_fn read, void *read_ctx,
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

debounce_event_t debounce_poll(debounce_t *db)
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

bool debounce_is_active(const debounce_t *db)
{
    return get_stable(db->flags);
}
