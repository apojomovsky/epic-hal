/**
 * Vendor-agnostic ADC oversampling and moving-average filter. Zero
 * dependencies beyond <stdint.h>. Both filters take a read callback
 * instead of calling EPIC_ADC_Read directly, so they work over any
 * HAL family's ADC or a mock source in tests.
 */

#ifndef EPIC_ADCFILTER_H
#define EPIC_ADCFILTER_H

#include <stdint.h>

/** Read callback returning one raw ADC sample (or any 16-bit value). */
typedef uint16_t (*epic_adcfilter_read_fn)(void *ctx);

/** Oversample-and-decimate: take 4^extra_bits raw samples via `read`,
 *  sum them, and right-shift by extra_bits. In practice extra_bits
 *  <= 6: a 10-bit ADC reading is at most 1023, and 4^6 * 1023 ~ 4.2M,
 *  which fits comfortably in the uint32_t accumulator. Returns the
 *  decimated reading with extra_bits more effective resolution. */
uint16_t epic_adcfilter_oversample(epic_adcfilter_read_fn read, void *ctx,
                                   uint8_t extra_bits);

/** Moving-average filter state. Caller-owned buffer, no hidden allocation. */
typedef struct {
    uint16_t *buf;    /**< caller-owned storage, `count` entries            */
    uint8_t   count;  /**< window length (buf's capacity)                   */
    uint8_t   index;  /**< next slot to overwrite                           */
    uint8_t   filled; /**< valid entries so far (< count until warmed up)   */
    uint32_t  sum;    /**< running sum, for O(1) average                    */
} epic_adcfilter_avg_t;

/** Initialize a moving-average filter. `buf` must have room for
 *  `count` entries and outlive `f`. */
void epic_adcfilter_avg_init(epic_adcfilter_avg_t *f, uint16_t *buf, uint8_t count);

/** Push one new sample, evicting the oldest if the window is full.
 *  Returns the new average (integer division, truncating). Before the
 *  window fills, the average is over only the samples pushed so far,
 *  not artificially divided by the full `count`. */
uint16_t epic_adcfilter_avg_push(epic_adcfilter_avg_t *f, uint16_t sample);

#endif /* EPIC_ADCFILTER_H */
