# epic-adcfilter API

## Public header

```c
#include "epic_adcfilter.h"
```

The public header depends only on `<stdint.h>`.

## Functions

### `uint16_t epic_adcfilter_oversample(epic_adcfilter_read_fn read, void *ctx, uint8_t extra_bits);`

Calls `read(ctx)` exactly `4^extra_bits` times, sums those raw samples in a
`uint32_t`, then right-shifts by `extra_bits`.

- `extra_bits == 0` is valid and performs exactly one raw read
- in practice `extra_bits <= 6` is the intended ceiling for 10-bit ADC input
- the callback can come from a HAL ADC, a test mock, or any other 16-bit source

### `void epic_adcfilter_avg_init(epic_adcfilter_avg_t *f, uint16_t *buf, uint8_t count);`

Initializes a moving-average instance over caller-owned storage. `buf` must have
room for `count` entries and remain valid for the lifetime of `f`.

### `uint16_t epic_adcfilter_avg_push(epic_adcfilter_avg_t *f, uint16_t sample);`

Pushes one new sample into the fixed window and returns the current average via
integer division.

- before the window fills, the average is over only the samples pushed so far
- once full, each push evicts exactly one oldest sample
- `count == 1` degenerates to "return the latest sample"
