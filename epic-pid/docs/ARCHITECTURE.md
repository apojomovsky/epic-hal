# `epic-pid` architecture

The design of `epic-pid`: a vendor-agnostic, single-loop, fixed-point
PID controller with anti-windup, derivative-on-measurement, and
bumpless auto/manual transfer. Why Q8.8 fixed point, why no runtime
`dt` argument, why clamping anti-windup over back-calculation or
conditional integration, why derivative-on-measurement is the only
mode, why bumpless transfer is built into `pid_update` and not
left to the caller, and the signed-shift precondition the Q8.8
truncation relies on. Full implementation plan:
`docs/epic-pid-plan.md` (the source of the design discussion; this
document is the post-implementation architecture write-up, including
the signed-shift probe's actual finding and the measured footprint).

## What it is and why it's simple

A single-loop, fixed-point PID controller, packaged the same way
`epic-fsm` and `epic-debounce` are: one caller-owned `pid_t` instance
per control loop, initialized once, then stepped once per fixed-period
control cycle via `pid_update()`. A PID controller is arithmetic on
plain data, not a hardware operation, so this module needs no
per-family backend and no inline asm: `src/pid.c` is one file,
compiles unchanged for host, PIC16, and PIC18, and the host unit
suite tests the exact code that ships on-target.

The one real dependency is `epic-math`, for the 16x16->32 signed
multiply `pic_math_mul_s16` the Q8.8 fixed-point math needs. On PIC16
(no hardware multiply) that call is AN526's shift-add asm; on PIC18
it is the hardware `MULWF` path; on host it is the portable-C
reference. `pid.c` never knows or cares which, exactly the same
"depend on the module that already solved this" reasoning
`epic-debounce` used for `epic-tick`.

## Why fixed-point, and the Q8.8 format

8-bit PICs pay dearly for `float` (XC8 emulates it in software;
`epic-math` exists specifically so this repo never needs to). PID
gains, the integrator, and every intermediate term are fixed-point
instead, using the **Q8.8** format (`int16_t`/`int32_t` holding
`value * 256`) `epic-math` already established as this repo's
convention for scaled coefficients (`pic_math_diff3`'s `inv_2h_q8`
argument, `docs/API.md` in `epic-math`). Q8.8 gains cover the
practical range for a hand-tuned loop (+/- 127.996, step ~0.0039)
while every product of two Q8.8-scale `int16_t` values fits
comfortably in `int32_t` (max `32767 * 32767 ~= 1.07e9`, well under
`INT32_MAX ~= 2.15e9`): no overflow guard is needed on the multiply
itself. This is a documented precondition (the caller must keep
gains in Q8.8 range and the error in `int16_t` range), not a
runtime check, the same style `pic_math_integrate_simpson38` already
uses for its own scaling overflow.

## Why gains are pre-scaled by the sample period, not a runtime `dt`

The classic PID form needs `Ki * dt` (integral) and `Kd / dt`
(derivative) every step. Two options were considered:

1. Pass `dt` (e.g. milliseconds elapsed, from `epic-tick`) into
   `pid_update()` every call, and multiply/divide by it at runtime.
2. Require the caller to call `pid_update()` at a **fixed, known
   period** (documented precondition, enforced by whatever drives the
   loop: a `epic-taskmgr` task at a fixed rate, or a timer ISR), and
   require the caller to **pre-scale** `Ki` and `Kd` by that period
   once, at configuration time, before converting to Q8.8.

**Chosen: option 2.** This is the standard convention real fixed-point
MCU PID libraries use (discrete-time gains, not continuous-time gains
handed a runtime `dt`), and it has a real payoff here: `pid_update()`
needs no division at all (division is the single most expensive
primitive `epic-math` provides on PIC16), and `epic-pid` ends up with
**zero dependency on `epic-tick` or any timebase**: `pid.h` needs
only `<stdint.h>`/`<stdbool.h>`, `pid.c` only additionally needs
`pic_math.h`. The cost is a documentation burden, not a code one:
the conversion from a tuned continuous-time `Kp`/`Ki`/`Kd` and a
chosen sample period `Ts` (seconds) to the three `int16_t` arguments
`pid_init()` takes must be spelled out precisely in `docs/API.md`:

```
kp_q8 = (int16_t)round(Kp * 256.0)
ki_q8 = (int16_t)round(Ki * Ts * 256.0)
kd_q8 = (int16_t)round(Kd / Ts * 256.0)
```

A change to the calling period always requires recomputing `ki_q8`
and `kd_q8`, never just `kp_q8`. For very small `Ts` (fast loops),
`Kd / Ts` can overflow the Q8.8 `int16_t` range, and for very small
`Ki * Ts` it can quantize to `0`; this module does not auto-scale,
it documents the limitation and leaves picking a workable `Ts`/gain
combination to the caller, the same way `pic_math`'s
divide-by-zero and Q-format overflow are documented preconditions
rather than runtime-guarded.

## Why clamping anti-windup, not back-calculation or conditional integration

Integrator windup (the integral term overshooting the actuator's real
range while the actuator sits saturated) was discussed against three
standard fixes:

1. **Back-calculation**: feed `(actual_output - unsaturated_output)`
   back into the integrator through an extra tracking gain `Kt`. Best
   transient behavior, but adds a fourth tunable gain and a second
   feedback path: more surface area than this plan's scope justifies.
2. **Conditional integration**: stop accumulating the integrator
   once saturated, but don't unwind what's already there. Simple,
   but recovers from windup slower than clamping once the error
   reverses sign.
3. **Clamping**: after every accumulation, clamp the integrator
   itself to `[out_min, out_max]` (converted to Q8.8). No extra
   gain, one comparison pair, and, because the integrator is *capped*
   rather than merely frozen, recovery when the error reverses sign
   is immediate (see `test_windup_recovery_immediate` in
   `tests/test_pid.c`).

**Chosen: option 3.** It is the simplest of the three for the same
reason `epic-fsm` picked one design over another elsewhere in this
repo: no new tunable knob, correctness is directly testable on host,
and it is "enough": back-calculation is a plausible future addition
if a concrete loop ever needs the better transient, not built now.

## Why derivative-on-measurement, not derivative-on-error

`D = Kd * d(error)/dt` spikes hard the instant `setpoint` steps (the
classic "derivative kick"), because `d(error)/dt` includes
`d(setpoint)/dt`. Since `d(error)/dt = d(setpoint)/dt -
d(measurement)/dt`, computing the D term from
`-d(measurement)/dt` instead gives the same steady-state damping with
no setpoint-step kick, at the one-line cost of a sign flip. This is a
strict improvement with no real downside for this module's scope, so
it is the *only* mode implemented; there is no
error-on-derivative fallback flag to keep the API and the test matrix
small.

## Why bumpless auto/manual transfer is built in, not left to the caller

An operator (or a supervisory `epic-fsm` machine) flipping a loop from
`AUTO` to `MANUAL` and back is a normal operational event, not an edge
case, and if the integrator isn't kept in sync while in `MANUAL`, the
output jumps the instant `AUTO` resumes (a "bump"). The fix is a
well-known trick (back-calculate what the integrator *would have to
be* for the current output to already equal what `AUTO` would compute)
and is cheap: one extra subtract in the `MANUAL` branch of
`pid_update()`. Since `pid_update()` is called every cycle in both
modes (see the API below), there is no separate "manual step"
function to keep in sync with the `AUTO` one, only a branch inside
the one function.

The implementation adds one detail beyond the plan's pseudocode, a
single-shot `skip_next_i_increment` flag on `pid_t`:

- In `MANUAL`, after the integrator is back-calculated, set the flag.
- In `AUTO`, if the flag is set, skip the I increment and clear it.

This makes the held-output-equals-first-AUTO-output handoff hold
exactly, the property `test_bumpless_transfer` asserts. Without the
flag, the first `AUTO` call after `MANUAL` would apply `ki_q8 * error`
to the back-calculated integrator and the output would shift by that
amount: not a "bump" (the controller state is continuous), but not
the exact held value the test asserts. The flag is a single `bool`
per `pid_t` (it lives in the existing struct), single-shot (consumed
by the `AUTO` call that uses it), and is cleared by `pid_reset` (a
fresh start makes any prior `MANUAL`->`AUTO` handoff no longer
relevant). No separate "manual step" function is added; the
single-function entry point the plan committed to is preserved.

## The signed-shift precondition (the probe the plan required)

`pid_update` truncates the Q8.8 accumulator to `int16_t` with
`sum_q8 >> 8`. On a negative `int32_t`, this relies on the compiler
implementing `>>` on signed integers as an arithmetic (sign-extending)
shift. This is implementation-defined in C99, not guaranteed
portable, and this repo's own convention (`AGENTS.md`, "before
trusting an uncertain compiler ... behavior, write a throwaway probe
and inspect the generated `.s`/`.map`") is not to assume it. The
Phase 0 commit (`feat(epic-pid): Phase 0 scaffold + signed-shift
probe`) ran the probe on the three targets this module ships on:

- **Host (gcc 13.3.0, -O2)**: signed `>>` on `int32_t` is
  arithmetic (sign-extending). Every value probed, including
  `((int32_t)-1) >> 8 == -1`, `((int32_t)0xFF800000) >> 8 == -32768`,
  `((int32_t)INT32_MIN) >> 8 == -8388608`, was sign-extending.
- **PIC16F877A, XC8 v3.10, -O2**: emits the textbook PIC16
  arithmetic-right-shift idiom for `v >> 8` on a 32-bit signed
  value:

  ```
      movlw   8
      movwf   ??_probe+4
  u25:
      rlf     ??_probe+3,w   ; carry = sign bit of the high byte
      rrf     ??_probe+3,f   ; rotate right, carry cascades down
      rrf     ??_probe+2,f
      rrf     ??_probe+1,f
      rrf     ??_probe,f
      decfsz  ??_probe+4,f
      goto    u25
  ```

  The `rlf high,w` reads the high byte's sign bit into CARRY, then
  the `rrf` chain rotates right while preserving the sign through
  the carry chain. This is exactly an arithmetic (sign-extending)
  right shift; the result lands in `??_probe[0..3]`.

- **PIC18F4550, XC8 v3.10, -O2**: emits the PIC18 manual
  sign-extension idiom:

  ```
      movff   probe@v+1, ??_probe          ; result[0] = v[1]
      movff   probe@v+2, ??_probe+1        ; result[1] = v[2]
      movff   probe@v+3, ??_probe+2        ; result[2] = v[3]
      clrf    (??_probe+3)^0, c            ; result[3] = 0
      btfsc   (??_probe+2)^0, 7, c         ; if (v[3] & 0x80) ...
      setf    (??_probe+3)^0, c            ;   result[3] = 0xFF
  ```

  This is the PIC18 standard pattern: clear the top byte, then
  conditionally set it to `0xFF` based on the sign bit of the high
  byte. Exactly an arithmetic shift by 8.

All three targets confirm the plan's assumption: no portable
truncating-toward-zero fallback is needed. (For reference, that
fallback would be `(x < 0) ? -((-x) >> 8) : (x >> 8)`; it would
differ from the raw `>> 8` result on negative numbers that aren't
exact multiples of 256, e.g. `((int32_t)-12345) / 256 == -48` but
`((int32_t)-12345) >> 8 == -49`. The Q8.8 representation works the
same either way as long as the same convention is used on both
sides; we use the raw shift, the convention that matches what every
mainstream compiler actually emits.)

## Footprint

Measured via `mcu/*/Makefile` with `mcu/target_sizecheck.c` (a
minimal program that calls `pid_init` once and `pid_update` in a
loop), real XC8 v3.10, `-O2`, linking the full `epic-math` (mul /
div / addsub / bcd / sqrt / numeric / rand: the same set `epic-fsm`
and `epic-debounce`'s mcu builds link, since a real on-target
application pulling in `pid.c` may also want any of these):

| Target       | `pid_t` (RAM) | Program space                | Data space (full math)   |
|--------------|---------------|------------------------------|--------------------------|
| PIC16F877A   | 21 B          | 1787 words (21.8% of 8 KW)   | 142 B (38.6% of 368 B)   |
| PIC16F873A   | 21 B          | 1789 words (43.7% of 4 KW)   | 140 B (72.9% of 192 B)   |
| PIC18F4550   | 21 B          | 1734 B    (5.3%  of 32 KB)   | 163 B (8.0%  of 2 KB)    |

`pid_t` is 21 bytes on both PIC16 and PIC18 (XC8 packs the bools and
the post-`int32_t` `int16_t` without padding: 6 B for the three
Q8.8 gains + 4 B for the clamp range + 4 B for the Q8.8 integrator
+ 2 B for `prev_measurement` + 1 B each for the two bools + 1 B for
the `pid_mode_t` enum + 2 B for `manual_output`). The plan's
"13-15 bytes" estimate was on the low side; 21 B is the real number
and is still in the same "small plain struct" order as `epic-fsm`'s
`fsm_t` (14 B on PIC16, 11 B on PIC18) and `epic-debounce`'s
`debounce_t`. The data space column above is the **upper bound**
(the full `epic-math` is available); a real on-target application
that uses only `pic_math_mul_s16` will link a much smaller subset
of math, so the actual on-target RAM is lower.

## Constraints

- **No runtime `dt`**, by design (see "Why gains are pre-scaled by
  the sample period" above). The caller must call `pid_update` at
  a fixed, known period `Ts`, and pre-scale `Ki`/`Kd` by `Ts` once
  at configuration. A change to the period requires recomputing
  `ki_q8` and `kd_q8`, never just `kp_q8`.
- **Gains are preconditions, not runtime-guarded**: `kp_q8`,
  `ki_q8`, `kd_q8` must fit in `int16_t` (the Q8.8 range).
  `|setpoint - measurement|` must fit in `int16_t` (so `error` is
  representable); for the values a real 8-bit-MCU control loop sees
  (ADC counts, scaled temperature, encoder position in a bounded
  range), this is naturally true. A runtime check would cost cycles
  on every call to guard a case that indicates a misconfigured
  caller, not a normal runtime condition.
- **Anti-windup by clamping** (not back-calculation or conditional
  integration; see "Why clamping anti-windup" above). The
  integrator is hard-capped to `[out_min << 8, out_max << 8]` after
  every AUTO accumulation and after every MANUAL back-calculation.
- **Derivative-on-measurement only** (not derivative-on-error; see
  "Why derivative-on-measurement" above). The first `pid_update`
  after `pid_init` or `pid_reset` zeroes the D term, gating it on
  `have_prev_measurement`.
- **No global state, no per-instance lock needed**. The `pid_t`
  instance owns all of its state, and two `pid_t` instances driven
  by interleaved calls never affect each other (asserted by
  `test_two_independent_instances`).
- **No PID variant flags**: no derivative-on-error mode flag (only
  one mode, period), no bumpless-disable flag (the plan requires
  it; the cost is one bool per `pid_t`, the same struct already has
  `have_prev_measurement` of the same shape), no back-calculation
  anti-windup tracking gain.

## Out of scope (future modules, not this one)

Discussed alongside this plan and deliberately excluded, each is a
plausible separate module later if a concrete application needs it,
not built now:

- **Back-calculation anti-windup** (the `Kt` tracking-gain approach).
  Clamping (chosen above) covers the common case; revisit only if
  a real loop's transient response genuinely needs the better
  recovery.
- **Gain scheduling / cascaded loops**. Meaningfully more complexity
  (multiple `pid_t` instances plus a scheduling policy) than a
  first control module should take on.
- **A ramp / trajectory (setpoint slew-rate limiter) generator**. A
  genuinely useful companion (feeds a smoothed `setpoint` into
  `pid_update`), but it's a self-contained few lines of state that
  deserves its own tiny module (e.g. `pic8-ramp`) rather than
  bloating this one, same reasoning `epic-debounce` used to exclude
  click / long-press detection.
- **Bang-bang / hysteresis control**. Not a PID variant at all, a
  different controller entirely; a separate module if a
  thermostatic-load application needs it.
- **A shared fixed-point low-pass / IIR filter** for conditioning
  `D_q8` or the raw `measurement` before it reaches `pid_update`.
  Composes at the call site (feed a filtered `measurement` in)
  exactly like `epic-adcfilter` already composes with anything that
  wants filtered ADC readings; no filtering belongs inside `pid.c`
  itself.
