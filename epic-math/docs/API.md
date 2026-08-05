# `epic-math` API reference

The authoritative declarations live in [`include/pic_math.h`](../include/pic_math.h);
this document is the prose reference (per-function semantics, the
divide-by-zero and `INT16_MIN / -1` contracts, the Q-format conventions for
`diff3`/`integrate_simpson38`, and the RNG state/seed). See
[ARCHITECTURE.md](ARCHITECTURE.md) for the backend split and the inline-asm
binding convention.

## Types & constants

```c
typedef struct { uint16_t quotient, remainder; } pic_math_udiv16_t;  /* unsigned 16/16 */
typedef struct { int16_t  quotient, remainder; } pic_math_sdiv16_t;  /* signed 16/16   */
```

`PIC_MATH_OPTIMIZE_FOR_SIZE` (default `1`): the PIC16 8x8 multiply code-size
vs speed trade-off. `1` (default) = looped (small); `0` = straight-line
unrolled (fast). PIC18 is unaffected (hardware `MULWF`).

## Multiply

### `uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)`
8x8 -> 16 unsigned. PIC18: hardware `MULWF`. PIC16: AN526 shift-add.

### `uint32_t pic_math_mul_u16(uint16_t a, uint16_t b)`
16x16 -> 32 unsigned. PIC18: four 8x8 partial products via `MULWF`. PIC16:
16-iteration shift-add.

### `int32_t pic_math_mul_s16(int16_t a, int16_t b)`
16x16 -> 32 signed. Built on the unsigned path with the app notes'
negate-operands/negate-result trick. `INT16_MIN*INT16_MIN = 1073741824`
fits `int32`.

## Divide / modulo

All three forms take an `ok` out-flag (may be `NULL`): set `false` and the
result fields zeroed on divide-by-zero, instead of AN526/AN544's "produces
incorrect results, caller must avoid zero" behavior.

### `pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok)`
Unsigned 16/16. `{ num/den, num%den }`. `den==0` -> `*ok=false`, `{0,0}`.

### `pic_math_sdiv16_t pic_math_divmod_s16(int16_t num, int16_t den, bool *ok)`
Signed 16/16, C99 truncated division (quotient toward zero, remainder takes
the dividend's sign). `INT16_MIN / -1` is the one signed divide whose true
quotient (32768) does not fit `int16`; it returns the two's-complement wrap
`{ INT16_MIN, 0 }` -- documented, not a crash.

### `pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok)`
Wide unsigned 32/16 -- "scale a 16-bit reading by a 16-bit factor without
overflow". The 16-bit quotient is the **low 16 bits** of `num/den`; the
caller must ensure `num < den*65536` or the high quotient bits are lost
(documented truncation, mirroring the library's 16-bit-width philosophy).
`{ (uint16_t)(num/den), (uint16_t)(num%den) }`.

## Add / sub / negate

### `uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)`
16-bit unsigned add; `carry_out` set on overflow (`a+b > 65535`), may be NULL.

### `uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)`
16-bit unsigned subtract; `borrow_out` set on underflow (`a < b`), may be NULL.

### `int16_t pic_math_negate_s16(int16_t v)` / `int32_t pic_math_negate_s32(int32_t v)`
Two's-complement negate (`0 - v` in unsigned, so no `-INT_MIN` overflow).
`negate(INT16_MIN) = INT16_MIN`, `negate(INT32_MIN) = INT32_MIN` (the
documented self-negation of the most-negative value).

## BCD

BCD values are **packed** (one nibble per digit): decimal 42 is the byte
`0x42`; decimal 9999 is `0x009999` in the low 3 nibbles of a `uint32_t`. "8"/"16"
name the *binary* width; the BCD width follows (2 / 5 digits). Invalid nibbles
(> 9) are processed arithmetically -- each nibble contributes its value times
its place -- so `bcd8_to_bin(0x0A) = 10`.

### `uint8_t pic_math_bcd8_to_bin(uint8_t bcd2)` / `uint8_t pic_math_bin_to_bcd8(uint8_t value)`
2-digit BCD <-> 0..99.

### `uint16_t pic_math_bcd16_to_bin(uint32_t bcd5)`
5-digit BCD (0x00000..0x99999) -> binary, returned as `uint16_t`. The BCD can
represent up to 99999, but the binary side is 16-bit: 0..65535 returns
exactly; 65536..99999 wrap to the low 16 bits (documented truncation).

### `uint32_t pic_math_bin_to_bcd16(uint16_t value)`
0..65535 (the `uint16_t` range) -> 5-digit BCD (0x00000..0x65535). A `uint16_t`
cannot reach 99999, so the output tops out at 0x65535.

### `uint8_t pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out)` / `uint8_t pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out)`
Packed-BCD 2-digit add/subtract with the DAW-style +/-6 adjust. `carry_out`
set if the BCD sum exceeds 99; `borrow_out` set on BCD underflow (`a < b` in
decimal). The subtract wraps modulo 100 on underflow. Defined for valid BCD
operands (0..99), which is what the tests exercise.

## Derived (portable C, one body, built on the leaf primitives)

### `uint16_t pic_math_sqrt_u16(uint16_t value)`
`floor(sqrt(value))` for 0..65535, via 16-bit Newton-Raphson on the division
primitive (as AN544 does -- `sqrt` calls `divmod`, not fresh asm).

### `int16_t pic_math_diff3(int16_t x_prev2, int16_t x_prev1, int16_t x_now, int16_t inv_2h_q8)`
3-point central first derivative `(x_now - x_prev2) / (2h)` in **Q8.8** fixed
point. `inv_2h_q8` is `1/(2h) * 2^8` (caller precomputes once; e.g. `h=1` ->
`0.5 * 256 = 128 = 0x0080`). The result is `(x_now - x_prev2) * inv_2h_q8`,
truncated to `int16` if it exceeds the Q8.8 range. `x_prev1` (the midpoint) is
in the signature for the 3-sample window but unused by the central-difference
formula. Exact for polynomials of degree <= 2.

### `int32_t pic_math_integrate_simpson38(int16_t f0, int16_t f1, int16_t f2, int16_t f3, int32_t three_h_over_8_q16)`
Simpson's 3/8 rule `(3h/8)*(f0 + 3f1 + 3f2 + f3)` in **Q16.16** fixed point.
`three_h_over_8_q16` is `3h/8 * 2^16` (e.g. `h=1` -> `0.375 * 65536 =
0x6000`). The product is computed in 32-bit (PIC16 XC8 has no 64-bit type);
the caller must scale so the result fits `int32` (documented truncation).
Exact for polynomials of degree <= 3.

## RNGs (explicit state, reentrant, no hidden global)

### `uint16_t pic_math_rand_next(uint16_t *state)`
16-bit maximal-length LFSR PRNG (taps {0,2,3,5}, polynomial
`x^16+x^14+x^13+x^11+1`, period `2^16-1 = 65535`). `*state` is in/out; a zero
state is mapped to the documented seed `0xACE1` on entry (the all-zero state
is a fixed point the LFSR cannot otherwise escape). Returns the next value
(also written to `*state`); never returns 0 over the period.

### `int16_t pic_math_rand_gauss(uint16_t *state)`
Approximate Gaussian (mean 0) sample via the Central Limit Theorem: the sum
of four `rand_next` samples, mean-subtracted (`4*32768`) and `>>2` into
`int16`. Approximately bell-shaped; not a cryptographic or high-quality RNG.

## Cheat sheet

| Function | Purpose |
|---|---|
| `pic_math_mul_u8/u16/s16` | unsigned/signed multiply |
| `pic_math_divmod_u16/s16/u32_16` | divide with remainder + `ok` flag |
| `pic_math_add_u16/sub_u16` | 16-bit add/sub with carry/borrow out |
| `pic_math_negate_s16/s32` | two's-complement negate |
| `pic_math_bcd8_to_bin/bin_to_bcd8` | 2-digit BCD <-> 0..99 |
| `pic_math_bcd16_to_bin/bin_to_bcd16` | 5-digit BCD <-> 0..65535 |
| `pic_math_bcd_add8/sub8` | packed-BCD add/sub with carry/borrow |
| `pic_math_sqrt_u16` | floor(sqrt), Newton on div |
| `pic_math_diff3` | 3-point central derivative, Q8.8 |
| `pic_math_integrate_simpson38` | Simpson 3/8 integration, Q16.16 |
| `pic_math_rand_next` | 16-bit LFSR PRNG (explicit state) |
| `pic_math_rand_gauss` | CLT Gaussian sample |