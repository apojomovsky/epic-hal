/**
 * @file    pic8_adcfilter.h
 * @brief   Vendor-agnostic ADC oversampling and moving-average filter.
 *          Zero dependencies beyond <stdint.h>.
 *
 * @details
 *   `pic8_adcfilter_oversample`: oversample-and-decimate (average 4^n raw
 *   samples, right-shift by n) for n extra bits of resolution.
 *   `pic8_adcfilter_avg_push`: an O(1) moving average over a caller-owned
 *   ring buffer. Both take a `pic8_adcfilter_read_fn` callback instead of
 *   calling `EPIC_ADC_Read` directly, so they work over any HAL family's
 *   ADC or a mock source in tests.
 */

#ifndef PIC8_ADCFILTER_H
#define PIC8_ADCFILTER_H

#include <stdint.h>

/** Read callback returning one raw ADC sample (or any 16-bit value). */
typedef uint16_t (*pic8_adcfilter_read_fn)(void *ctx);

/**
 * @brief  Oversample and decimate: take 4^extra_bits raw samples via `read`,
 *         sum them, and right-shift by extra_bits.
 * @param  read        callback returning one raw sample.
 * @param  ctx         opaque context for the callback (may be NULL).
 * @param  extra_bits  extra bits of resolution (0 = single read, 6 = 4096
 *                     samples). In practice extra_bits <= 6: a 10-bit ADC
 *                     reading is at most 1023, and 4^6 * 1023 ~ 4.2M, which
 *                     fits comfortably in the uint32_t accumulator.
 * @return the decimated reading with extra_bits more effective resolution.
 */
uint16_t pic8_adcfilter_oversample(pic8_adcfilter_read_fn read, void *ctx,
                                   uint8_t extra_bits);

/** Moving-average filter state. Caller-owned buffer, no hidden allocation. */
typedef struct {
    uint16_t *buf;    /**< caller-owned storage, `count` entries            */
    uint8_t   count;  /**< window length (buf's capacity)                   */
    uint8_t   index;  /**< next slot to overwrite                           */
    uint8_t   filled; /**< valid entries so far (< count until warmed up)   */
    uint32_t  sum;    /**< running sum, for O(1) average                    */
} pic8_adcfilter_avg_t;

/**
 * @brief  Initialize a moving-average filter. `buf` must have room for
 *         `count` entries and outlive `f`.
 */
void pic8_adcfilter_avg_init(pic8_adcfilter_avg_t *f, uint16_t *buf, uint8_t count);

/**
 * @brief  Push one new sample, evicting the oldest if the window is full.
 * @return the new average (integer division, truncating). Before the window
 *         fills, the average is over only the samples pushed so far, not
 *         artificially divided by the full `count`.
 */
uint16_t pic8_adcfilter_avg_push(pic8_adcfilter_avg_t *f, uint16_t sample);

#endif /* PIC8_ADCFILTER_H */
