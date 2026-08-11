/**
 * x4 quadrature decode via a Gray-code transition table, one
 * implementation for host, PIC16, and PIC18. State ordering 00,01,11,10
 * (true Gray code); physical direction is a wiring convention: swap
 * `pin_a`/`pin_b` at init to invert it (see docs/API.md).
 */

#include "encoder.h"
#include "epic_tick.h"        /* glitch-gate timebase                       */
#include "core/hal_irq.h"

/* Gray-code quadrature step table, indexed by (last_state<<2)|new_state
 * where state = (A<<1)|B. +-1 = valid single-bit transition (one edge,
 * x4); 0 = no change or an impossible transition (both bits flipped, a
 * missed edge or corruption), counted as an error by the caller. */
static const int8_t QUAD_TABLE[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0,
};

/* Extract this instance's 2-bit (a<<1)|b state from a port byte. */
static uint8_t extract_state(const encoder_t *enc, uint8_t port_value)
{
    uint8_t a = (uint8_t)((port_value >> enc->pin_a) & 1U);
    uint8_t b = (uint8_t)((port_value >> enc->pin_b) & 1U);
    return (uint8_t)((a << 1) | b);
}

void encoder_init(encoder_t *enc, uint8_t pin_a, uint8_t pin_b,
                  uint16_t min_edge_interval_ms, uint8_t port_value)
{
    enc->pin_a                = pin_a;
    enc->pin_b                = pin_b;
    enc->min_edge_interval_ms = min_edge_interval_ms;
    enc->position             = 0;
    enc->error_count          = 0;
    enc->glitch_count         = 0;
    enc->last_edge_tick       = epic_tick_get();
    enc->last_state           = extract_state(enc, port_value);
}

void encoder_reset(encoder_t *enc, uint8_t port_value)
{
    /* Pins and min_edge_interval_ms unchanged; only accumulators and
     * resync state reset. */
    enc->position       = 0;
    enc->error_count    = 0;
    enc->glitch_count   = 0;
    enc->last_edge_tick = epic_tick_get();
    enc->last_state     = extract_state(enc, port_value);
}

void encoder_update(encoder_t *enc, uint8_t port_value)
{
    uint8_t new_state = extract_state(enc, port_value);

    /* Another instance's pins may have changed on the same port byte;
     * this instance's state is unchanged, so no-op, not an error. */
    if (new_state == enc->last_state) return;

    if (enc->min_edge_interval_ms != 0U) {
        uint32_t now = epic_tick_get();
        if (epic_tick_elapsed_since(enc->last_edge_tick) <
            (uint32_t)enc->min_edge_interval_ms) {
            /* last_state intentionally not updated: the next sample
             * compares against the last accepted state, not this one. */
            enc->glitch_count++;
            return;
        }
        enc->last_edge_tick = now;
    }

    int8_t delta = QUAD_TABLE[(uint8_t)((enc->last_state << 2) | new_state)];
    if (delta == 0) {
        /* Both bits appear to have flipped between samples: a missed edge
         * or corruption. last_state still advances so decode resyncs. */
        enc->error_count++;
    }
    enc->position  += delta;
    enc->last_state = new_state;
}

int32_t encoder_get_position(const encoder_t *enc)
{
    /* Read-twice-retry (the epic_tick_get pattern): the ISR updates
     * position as a multi-byte RMW, so a single read can tear; retry
     * until two consecutive reads agree. */
    int32_t p;
    do {
        p = enc->position;
    } while (p != enc->position);
    return p;
}

uint16_t encoder_get_error_count(const encoder_t *enc)
{
    /* Read-twice-retry, same discipline as encoder_get_position. */
    uint16_t c;
    do {
        c = enc->error_count;
    } while (c != enc->error_count);
    return c;
}

uint16_t encoder_get_glitch_count(const encoder_t *enc)
{
    /* Read-twice-retry, same discipline as encoder_get_position. */
    uint16_t c;
    do {
        c = enc->glitch_count;
    } while (c != enc->glitch_count);
    return c;
}
