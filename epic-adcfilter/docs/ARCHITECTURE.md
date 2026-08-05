# epic-adcfilter architecture

## Model

`epic-adcfilter` provides two small numeric utilities for noisy sampled data:

- `epic_adcfilter_oversample`: oversample-and-decimate through a caller-supplied
  `read(ctx)` callback
- `epic_adcfilter_avg_t`: an O(1) moving average over a caller-owned ring buffer

The module is intentionally hardware-neutral. The core never calls a HAL ADC
directly and does not include any HAL header. That keeps the shipped code the
same on host and target and lets tests exercise the literal implementation.

## Oversampling

`epic_adcfilter_oversample(read, ctx, extra_bits)` takes `4^extra_bits` raw
samples, sums them in a `uint32_t`, then right-shifts by `extra_bits`. This is
the standard oversample-and-decimate technique: `extra_bits == 0` is a plain
single read, and each additional bit multiplies the raw-sample count by four.

The implementation uses a running `samples` counter and a `uint32_t`
accumulator. At the practical ceiling documented by the API (`extra_bits <= 6`
for 10-bit ADC data), that sum stays comfortably within 32 bits.

## Moving average

`epic_adcfilter_avg_t` is a caller-owned fixed window. The caller supplies the
storage buffer and count at init time; the module keeps only:

- the next overwrite index
- how many slots have been filled so far
- a `uint32_t` running sum

That makes `epic_adcfilter_avg_push` O(1): when the window is full it subtracts
the evicted sample from the running sum, adds the new sample, advances the ring
index, and returns `sum / active_count`. There is never an O(N) rescan of the
whole window.
