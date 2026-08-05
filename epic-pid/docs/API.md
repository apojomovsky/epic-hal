# `epic-pid` API reference

Authoritative declarations: [`include/pid.h`](../include/pid.h).
Design rationale and the per-decision reasoning are in
[`ARCHITECTURE.md`](ARCHITECTURE.md); the implementation plan
that motivated the design is in [`docs/epic-pid-plan.md`](../../docs/epic-pid-plan.md).

## Types & constants

```c
typedef enum {
    PID_MODE_MANUAL = 0,
    PID_MODE_AUTO,
} pid_mode_t;

typedef struct {
    int16_t    kp_q8, ki_q8, kd_q8;    /* Q8.8 discrete-time gains */
    int16_t    out_min, out_max;       /* actuator output clamp */
    int32_t    integrator_q8;          /* Q8.8 accumulated integral, always
                                        * clamped to [out_min<<8, out_max<<8] */
    int16_t    prev_measurement;       /* for derivative-on-measurement */
    bool       have_prev_measurement;  /* gates the D term on the first
                                        * call after init/reset */
    bool       skip_next_i_increment;  /* one-shot flag for bumpless
                                        * MANUAL->AUTO transfer; set in
                                        * MANUAL, consumed by next AUTO */
    pid_mode_t mode;
    int16_t    manual_output;          /* caller-set target output in MANUAL */
} pid_t;
```

### `pid_t`

One PID control loop, caller-owned storage. Fields are written by
`pid_init` / `pid_set_*` and by `pid_update` (the integrator, the
D-term history, and the back-calculated MANUAL integrator). The
caller reads them through the API; reading them directly is
unsupported but harmless.

`pid_t` is 21 bytes on both PIC16 and PIC18 (XC8 packs the bools
and the post-`int32_t` `int16_t` without padding). See
[`ARCHITECTURE.md`](ARCHITECTURE.md#footprint) for the measured
per-target footprint.

## Gain conversion: continuous-time Kp/Ki/Kd to Q8.8 discrete-time

The library takes pre-scaled Q8.8 discrete-time gains, not
continuous-time gains handed a runtime `dt`. Convert once at
configuration time, before calling `pid_init`:

```c
/* Kp, Ki, Kd: continuous-time gains the user tuned (any units).
 * Ts       : sample period in seconds (fixed, known, the period at
 *            which pid_update will be called -- e.g. 0.01 for 100 Hz).
 * kp_q8, ki_q8, kd_q8: the int16_t Q8.8 values to pass to pid_init.
 *
 *   kp_q8 = (int16_t)round(Kp         * 256.0)
 *   ki_q8 = (int16_t)round(Ki * Ts    * 256.0)
 *   kd_q8 = (int16_t)round(Kd / Ts    * 256.0)
 */
```

A change to the calling period always requires recomputing `ki_q8`
and `kd_q8`, never just `kp_q8`. Q8.8 holds `value * 256`, so
`round(Kp * 256.0)` produces an `int16_t` whose value `V`
corresponds to continuous-time `Kp = V / 256.0`.

### Limitations (not guarded, documented)

- **`kd_q8` overflows the Q8.8 `int16_t` range for very small `Ts`.**
  If `Kd / Ts > 127.996`, the cast truncates and the effective `Kd`
  used by the loop is wrong. For very fast loops (small `Ts`),
  consider a different Q format (Q16.16, etc.); this library is
  Q8.8-only by design.
- **`ki_q8` quantizes to 0 for very small `Ts`.** If `Ki * Ts <
  1/256 ~= 0.0039`, the rounded product is 0 and the integrator
  contribution is permanently zero. For very slow loops (large `Ts`)
  with small `Ki`, the integral term disappears; pick a `Ts`/`Ki`
  combination where the product is at least a few Q8.8 units.
- **`pid_update` does not check `|setpoint - measurement| <= 32767`.**
  The `error` value must fit in `int16_t`. A `setpoint` jump or
  `measurement` glitch that produces a larger error is a
  misconfigured caller, not a normal runtime condition; the
  library does not branch on it.

## Functions

### `void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8, int16_t out_min, int16_t out_max)`

Initialize one instance. Stores the gains and the output clamp range,
sets `mode = PID_MODE_AUTO`, zeroes the integrator, clears the
D-term history, zeroes `manual_output`, and clears the
`skip_next_i_increment` flag. The first `pid_update()` call therefore
behaves like the first call on a freshly-initialized loop: P+I from
zero, D zeroed by the no-prev-measurement gate.

Precondition: `out_min <= out_max`. Not runtime-checked.

### `void pid_reset(pid_t *pid)`

Zero the integrator and clear the D-term history. Keep gains, clamp
range, and mode untouched. Also clear the `skip_next_i_increment`
flag (a fresh start makes any prior MANUAL->AUTO handoff no longer
relevant). The next `pid_update()` call behaves like the very first
call on a freshly-`pid_init`'d instance despite the gains being
unchanged from before the reset.

For recovering from an external fault (e-stop, sensor dropout)
without losing tuning. For a full re-init (new gains, new clamp),
use `pid_init` instead.

### `void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8)`

Replace the three gains, leaving the integrator, D-term history,
mode, and `skip_next_i_increment` untouched. Safe to call while the
loop is running (the new gains apply on the very next `pid_update`).

### `void pid_set_mode(pid_t *pid, pid_mode_t mode)`

Switch between `AUTO` and `MANUAL`. Does **not** reset the
integrator or D-term history. Switching mode is not a fault, and
bumpless transfer depends on integrator state carrying across the
switch.

If switching MANUAL -> AUTO without ever calling `pid_update` in
MANUAL (so the integrator was never back-calculated), no
`skip_next_i_increment` flag is set, and the first AUTO call
applies its I increment normally. The flag is only set inside
`pid_update` itself, not by `pid_set_mode`.

### `void pid_set_manual_output(pid_t *pid, int16_t value)`

Set the target output used while `mode == PID_MODE_MANUAL`. Only
consulted by `pid_update` while in MANUAL; ignored in AUTO. Call
every cycle the operator / supervisor wants a new manual output in
effect.

### `int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement)`

Step the controller once. Call once per fixed control-loop period,
in EITHER mode; this is the single per-cycle entry point, there is
no separate "manual step" function.

In `AUTO`: integrates `ki_q8 * error`, adds the P, I, and
`-d(measurement)/dt` terms, and returns
`clamp((P + I + D) >> 8, out_min, out_max)`. The integrator is itself
clamped to `[out_min << 8, out_max << 8]` so it can recover
immediately when the error reverses sign. If the previous MANUAL
call set `skip_next_i_increment`, this AUTO call skips the I
increment and clears the flag (the bumpless-transfer detail;
see `ARCHITECTURE.md`).

In `MANUAL`: returns `clamp(manual_output, out_min, out_max)`, and
back-calculates the integrator so that resuming AUTO is bumpless (the
held output equals the next AUTO call's output exactly, if the next
AUTO call is given the same setpoint/measurement). Sets
`skip_next_i_increment` so the next AUTO call skips its I increment.

Precondition: `|setpoint - measurement| <= 32767` (so `error` fits
in `int16_t`). Not runtime-checked.

Return: the clamped output, always in `[out_min, out_max]`.

## Usage

```c
#include "pid.h"

/* Configuration: a 100 Hz control loop (Ts = 0.01 s), tuned
 * Kp = 1.0, Ki = 5.0 (i.e. 0.05 / 0.01), Kd = 0.001.
 * Output clamp [-1000, 1000]. */
#define FOSC_HZ     20000000UL
#define CONTROL_HZ  100UL
#define TS_SEC      (1.0f / (float)CONTROL_HZ)

static int16_t q8(float x) { return (int16_t)(x * 256.0f); }

static pid_t g_loop;
static int16_t g_setpoint, g_measurement;

void control_init(void)
{
    pid_init(&g_loop,
             q8(1.0f),                       /* Kp          */
             q8(5.0f * TS_SEC),              /* Ki * Ts     */
             q8(0.001f / TS_SEC),            /* Kd / Ts     */
             (int16_t)-1000, (int16_t)1000);
    g_setpoint    = 0;
    g_measurement = 0;
}

/* Call from a fixed-period task (e.g. a epic-taskmgr entry, or a
 * timer ISR's deferred work, at exactly CONTROL_HZ). */
void control_tick(void)
{
    int16_t output = pid_update(&g_loop, g_setpoint, g_measurement);
    drive_actuator(output);
}

/* Operator / supervisor takes over: */
void control_take_manual(int16_t target)
{
    pid_set_manual_output(&g_loop, target);
    pid_set_mode(&g_loop, PID_MODE_MANUAL);
}

/* Operator / supervisor hands back: */
void control_resume_auto(void)
{
    pid_set_mode(&g_loop, PID_MODE_AUTO);
    /* The first AUTO call after MANUAL returns the held manual value
     * exactly (no jump), then the integrator resumes evolving. */
}
```

## Cheat sheet

| Symbol | Purpose |
|---|---|
| `pid_t` | one instance (caller-owned plain data, 21 B on PIC16/PIC18) |
| `pid_init` | set up with gains + clamp; zero state |
| `pid_reset` | zero integrator + D history; keep gains, clamp, mode |
| `pid_set_gains` | replace the three Q8.8 gains |
| `pid_set_mode` | switch AUTO <-> MANUAL; carries state across |
| `pid_set_manual_output` | set the operator's target in MANUAL |
| `pid_update` | one control step; returns the clamped output |
| `PID_MODE_AUTO` / `PID_MODE_MANUAL` | mode constants |
| `pid_t.integrator_q8` | Q8.8 integrator, always in clamp range |
| `pid_t.have_prev_measurement` | internal: gates the D term on first call |
| `pid_t.skip_next_i_increment` | internal: bumpless MANUAL->AUTO handoff |
