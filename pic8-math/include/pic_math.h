/**
 * @file    pic_math.h
 * @brief   Family-agnostic fixed-point math utility library for 8-bit
 *          PICs, ported from AN526/AN544 into a stateless API (everything
 *          by value/pointer, no fixed operand addresses, explicit RNG
 *          state). One neutral public API; a portable-C host backend, a
 *          PIC16 inline-asm backend, and a PIC18 inline-asm backend
 *          (exploiting hardware `MULWF` the app notes' chips lacked) sit
 *          behind it. See docs/ARCHITECTURE.md for the backend split.
 */

#ifndef PIC_MATH_H
#define PIC_MATH_H

#include <stdint.h>
#include <stdbool.h>

/* ─── configuration ────────────────────────────────────────────── */

/**
 * @brief  PIC16 8x8 multiply code-size vs speed trade-off: default 1
 *         selects AN526's looped (smaller) form, 0 selects straight-line
 *         (faster, larger). No effect on PIC18, which always uses `MULWF`.
 */
#ifndef PIC_MATH_OPTIMIZE_FOR_SIZE
#define PIC_MATH_OPTIMIZE_FOR_SIZE 1
#endif

/* ─── multiply ─────────────────────────────────────────────────── */

/**
 * @brief  8x8 -> 16 unsigned multiply.
 * @param  a  multiplicand, 0..255
 * @param  b  multiplier,   0..255
 * @return a*b as a 16-bit value, 0..65535.
 *
 * @details On PIC18 this is the single-cycle hardware `MULWF` path
 *         (result in PRODH:PRODL). On PIC16 (no hardware multiply) this is
 *         AN526's shift-and-add loop. The host reference is plain
 *         `(uint16_t)a * (uint16_t)b`.
 */
uint16_t pic_math_mul_u8(uint8_t a, uint8_t b);

/**
 * @brief  16x16 -> 32 unsigned multiply.
 * @return a*b as a 32-bit value. PIC18 builds this from three 8x8 partial
 *         products via `MULWF`; PIC16 uses AN526's 16x16 shift-add.
 */
uint32_t pic_math_mul_u16(uint16_t a, uint16_t b);

/**
 * @brief  16x16 -> 32 signed multiply.
 * @return (int32_t)a*b. Built on the unsigned path with the app notes'
 *         negate-operands/negate-result sign handling.
 */
int32_t  pic_math_mul_s16(int16_t a, int16_t b);

/* ─── divide / modulo ─────────────────────────────────────────────
 * `ok` is set false and the result fields are zeroed on divide-by-zero,
 * instead of AN526/AN544's documented "produces incorrect results, caller
 * must ensure denominator != 0" behavior. A NULL `ok` pointer is allowed:
 * the divide-by-zero check still runs, the result is still zeroed, but no
 * flag is written back. */

/** Unsigned 16/16 quotient+remainder. */
typedef struct { uint16_t quotient, remainder; } pic_math_udiv16_t;
/** Signed 16/16 quotient+remainder. */
typedef struct { int16_t  quotient, remainder; } pic_math_sdiv16_t;

/**
 * @brief  Unsigned 16/16 divide with remainder.
 * @param  num  numerator,  0..65535
 * @param  den  denominator,1..65535 (0 -> *ok=false, fields zeroed)
 * @param  ok   out: true if den != 0; may be NULL
 * @return { quotient = num/den, remainder = num%den }.
 */
pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok);

/**
 * @brief  Signed 16/16 divide with remainder.
 * @note   INT16_MIN / -1 is the one signed divide that can overflow the
 *         16-bit quotient; it does not crash or wrap silently -- see
 *         docs/API.md for the documented result.
 */
pic_math_sdiv16_t pic_math_divmod_s16(int16_t  num, int16_t  den, bool *ok);

/**
 * @brief  Wide unsigned 32/16 divide -- the "scale a 16-bit ADC reading by
 *         a 16-bit factor without overflow" convenience form.
 * @return { quotient = num/den, remainder = num%den } (16-bit quotient;
 *         caller must ensure num < den*65536 or the quotient is truncated
 *         to 16 bits, as documented).
 */
pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok);

/* ─── add / sub / negate, with explicit carry/borrow out ───────── */

/**
 * @brief  16-bit unsigned add with carry out.
 * @param  carry_out  set true on overflow (sum > 65535); may be NULL.
 * @return (a + b) truncated to 16 bits.
 */
uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out);

/**
 * @brief  16-bit unsigned subtract with borrow out.
 * @param  borrow_out  set true on underflow (a < b); may be NULL.
 * @return (a - b) truncated to 16 bits.
 */
uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out);

/** @brief 16-bit two's-complement negate (INT16_MIN negates to itself). */
int16_t  pic_math_negate_s16(int16_t v);

/** @brief 32-bit two's-complement negate (INT32_MIN negates to itself). */
int32_t  pic_math_negate_s32(int32_t v);

/* ─── BCD ─────────────────────────────────────────────────────────
 * "16"/"8" name the *binary* width; BCD width follows (5 digits / 2
 * digits). BCD values are packed BCD (one nibble per digit), not ASCII --
 * e.g. the decimal value 42 is the byte 0x42, and 9999 is 0x009999 packed
 * into the low 3 nibbles of a uint32_t. */

/**
 * @brief  5-digit packed BCD (0x00000..0x99999) -> binary, returned as
 *         uint16_t. The BCD can represent up to 99999, but the binary side
 *         is 16-bit: BCD representing 0..65535 returns exactly; 65536..99999
 *         wrap to the low 16 bits (documented truncation, mirroring the
 *         library's other 16-bit-width forms).
 * @note   A nibble > 9 is invalid input; the conversion treats each nibble
 *         independently (each contributes its value times its place), so
 *         bcd8_to_bin(0x0A) = 10 and bcd16_to_bin(0xABCDE) is well-defined.
 */
uint16_t pic_math_bcd16_to_bin(uint32_t bcd5);

/**
 * @brief  0..65535 (the uint16_t range) -> 5-digit packed BCD
 *         (0x00000..0x65535). The "16" names the binary width; a uint16_t
 *         cannot reach 99999, so the BCD output tops out at 0x65535.
 */
uint32_t pic_math_bin_to_bcd16(uint16_t value);

/** @brief 2-digit packed BCD (0x00..0x99) -> binary 0..99. */
uint8_t  pic_math_bcd8_to_bin(uint8_t bcd2);

/** @brief 0..99 -> 2-digit packed BCD. */
uint8_t  pic_math_bin_to_bcd8(uint8_t value);

/**
 * @brief  Packed-BCD 2-digit add with carry out (DAW-style +/-6 adjust).
 * @param  carry_out  set true if the BCD sum exceeds 99; may be NULL.
 * @return packed-BCD 2-digit sum.
 */
uint8_t  pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out);

/**
 * @brief  Packed-BCD 2-digit subtract with borrow out.
 * @param  borrow_out  set true on BCD underflow (a < b in BCD); may be NULL.
 * @return packed-BCD 2-digit difference.
 */
uint8_t  pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out);

/* ─── built on the above; portable C, one implementation ────────── */

/** @brief floor(sqrt(value)) for 0..65535, via 16-bit Newton-Raphson on
 *        the division primitive (as AN544 does -- sqrt calls div, not asm). */
uint16_t pic_math_sqrt_u16(uint16_t value);

/**
 * @brief  3-point numerical first derivative: (x_now - x_prev2) / (2h),
 *         computed in fixed point.
 * @param  inv_2h_q8  Q8.8 fixed-point representation of 1/(2h) (the caller
 *         precomputes this once, as AN544 does, since multiply is cheaper
 *         than divide). E.g. h=1 -> inv_2h = 0.5 -> 0x0080.
 * @return Q8.8 fixed-point estimate of the derivative at the midpoint.
 *         Checked against analytic linear/quadratic functions within a
 *         documented error bound in tests, not for exact equality.
 */
int16_t pic_math_diff3(int16_t x_prev2, int16_t x_prev1, int16_t x_now,
                       int16_t inv_2h_q8);

/**
 * @brief  Simpson's-3/8-rule numerical integration over four samples:
 *           integral ~= (3h/8) * (f0 + 3*f1 + 3*f2 + f3).
 * @param  three_h_over_8_q16  Q16.16 fixed-point representation of 3h/8
 *         (caller precomputes once). E.g. h=1 -> 3h/8 = 0.375 -> 0x6000.
 * @return Q16.16 fixed-point estimate of the integral.
 */
int32_t pic_math_integrate_simpson38(int16_t f0, int16_t f1, int16_t f2,
                                     int16_t f3,
                                     int32_t three_h_over_8_q16);

/* ─── RNGs: explicit state, reentrant, no hidden global ───────────
 * The LFSR never gets stuck at the all-zero state it could not otherwise
 * recover from: a zero `*state` is mapped to the LFSR's documented nonzero
 * seed on the first call, so the period is the full 2^16-1 sequence. */

/**
 * @brief  16-bit maximal-length LFSR pseudo-random step.
 * @param  state  in/out LFSR state; must point at a persistent uint16_t.
 *                A zero state is treated as the documented nonzero seed.
 * @return the next 16-bit pseudo-random value (also written back to *state).
 */
uint16_t pic_math_rand_next(uint16_t *state);

/**
 * @brief  Approximate Gaussian (mean 0) pseudo-random sample via the
 *         Central Limit Theorem: the sum of several LFSR samples,
 *         normalized. Mirrors AN544 Figure 3's distribution.
 * @param  state  in/out LFSR state shared with pic_math_rand_next.
 * @return a signed sample with an approximately bell-shaped distribution.
 */
int16_t  pic_math_rand_gauss(uint16_t *state);

#endif /* PIC_MATH_H */
