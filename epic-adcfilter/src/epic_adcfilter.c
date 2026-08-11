/**
 * ADC oversampling and moving-average filter, one implementation for
 * host, PIC16, and PIC18 alike (zero hardware dependency).
 */

#include "epic_adcfilter.h"

uint16_t epic_adcfilter_oversample(epic_adcfilter_read_fn read, void *ctx,
                                   uint8_t extra_bits)
{
    /* 4^extra_bits = 2^(2*extra_bits). extra_bits <= 6 in practice
     * (see the header doc), so count fits uint32_t. */
    uint32_t count = 1UL << (extra_bits * 2u);
    uint32_t sum   = 0UL;
    for (uint32_t i = 0UL; i < count; i++) {
        sum += (uint32_t)read(ctx);
    }
    return (uint16_t)(sum >> extra_bits);
}

void epic_adcfilter_avg_init(epic_adcfilter_avg_t *f, uint16_t *buf, uint8_t count)
{
    f->buf    = buf;
    f->count  = count;
    f->index  = 0u;
    f->filled = 0u;
    f->sum    = 0UL;
}

uint16_t epic_adcfilter_avg_push(epic_adcfilter_avg_t *f, uint16_t sample)
{
    if (f->filled < f->count) {
        /* Window not yet full: just add, average over what's been pushed. */
        f->buf[f->index] = sample;
        f->sum += (uint32_t)sample;
        f->filled++;
        f->index++;
        if (f->index >= f->count) { f->index = 0u; }
        return (uint16_t)(f->sum / f->filled);
    }
    /* Window full: evict oldest, add new, average over the full window. */
    f->sum -= (uint32_t)f->buf[f->index];
    f->buf[f->index] = sample;
    f->sum += (uint32_t)sample;
    f->index++;
    if (f->index >= f->count) { f->index = 0u; }
    return (uint16_t)(f->sum / f->count);
}
