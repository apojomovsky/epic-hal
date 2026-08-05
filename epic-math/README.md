# epic-math, fixed-point math utility library for 8-bit PICs

A stateless fixed-point math library for the PIC families this repo already
supports, **PIC16F87XA** (mid-range, no hardware multiply) and
**PIC18F2455/2550/4455/4550** (PIC18 core, hardware `MULWF`), ported from
Microchip application notes **AN526** and **AN544** and modernized into one
family-agnostic C library with a hand-written inline-asm core.

- **One neutral public API** (`include/pic_math.h`, no `#ifdef`): multiply,
  divide/modulo, add/sub/negate, BCD, integer sqrt, numerical differentiation
  and integration, and two PRNGs, all by value/pointer, no hidden global
  arithmetic state.
- **Three backends**: a portable-C host reference (the test oracle), a PIC16
  inline-asm backend (AN526 shift-add), and a PIC18 inline-asm backend that
  uses the hardware `MULWF` the app notes' target chips lacked.
- **Minimal asm surface**: only the true leaf arithmetic primitives get
  per-family inline asm; everything derived (sqrt, differentiation,
  integration, the RNGs) is one portable-C body built on them.
- **Improvements over the source material**: divide-by-zero returns a flagged
  zero instead of garbage; the RNGs take explicit `uint16_t *state` instead of
  a hidden seed; operands pass by value instead of AN526/AN544's fixed shared
  RAM (`ACCa`/`ACCb`) convention.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), backend split, the XC8 inline-asm
  binding convention, banking, and the three test tiers.
- [API reference](docs/API.md), per-function semantics, edge contracts, and
  the Q-format conventions.
- [Implementation plan](../docs/epic-math-plan.md), source material and the
  phase-by-phase plan.

## Quick start

### Host simulator (the test oracle)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 8/8 tests, incl. exhaustive
```

The host build links `src/host/` + `src/common/` (no HAL, the library is
stateless computation). `tools/gen_golden_vectors.c` is a host tool that
emits `tests/golden_vectors.h` (the Tier-2/3 vector table):
`./build/gen_golden_vectors > tests/golden_vectors.h`.

### Real target (XC8)

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
# PIC18F4550: full golden-vector on-target self-test over USART
make -C mcu/pic18fxx5x-math-mplabx MCU=18F4550
# PIC16F877A: on-target smoke (the 8 K-flash part can't hold the full self-test)
make -C mcu/pic16f87xa-math-mplabx MCU=16F877A
```

See `docs/ARCHITECTURE.md` "Testing tiers" for why PIC16 builds a smoke and
PIC18 builds the full self-test.

## Use it in your own firmware

```c
#include "pic_math.h"

uint32_t scale = pic_math_mul_u16(adc_reading, factor);   /* no overflow */
pic_math_udiv16_t d = pic_math_divmod_u16(n, 10, &ok);     /* ok=false on /0 */
uint16_t r = pic_math_sqrt_u16(value);                      /* floor(sqrt) */
uint16_t s; pic_math_rand_next(&s);                        /* explicit state */
```

## License

MIT, see the [repo LICENSE](../LICENSE).