# epic-adcfilter, vendor-agnostic ADC oversampling and moving-average filter

Two composable utilities for noisy 10-bit ADC readings on 8-bit PICs:
oversample-and-decimate (gain extra bits of resolution) and an O(1) moving
average over a caller-owned ring buffer.

- **Vendor-agnostic**: the core takes a `epic_adcfilter_read_fn` callback,
  never calls `EPIC_ADC_Read` directly. Works over any HAL family's ADC, a
  mock in tests, or any 16-bit numeric source.
- **Zero dependency**: both header and implementation need only `<stdint.h>`.
  The host suite tests the exact code that ships on-target.
- **O(1) moving average**: running sum, no recompute per sample.

## Quick start

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 31 checks, all pass
```

## Use it

```c
#include "epic_adcfilter.h"
uint16_t raw = epic_adcfilter_oversample(read_adc, NULL, 2); /* 16x, +2 bits */
uint16_t avg = epic_adcfilter_avg_push(&filter, raw);        /* O(1) average */
```

## License

MIT, see the [repo LICENSE](../LICENSE).
