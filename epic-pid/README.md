# epic-pid, vendor-agnostic fixed-point PID controller

A single-loop, fixed-point PID controller with anti-windup,
derivative-on-measurement, and bumpless auto/manual transfer. One
caller-owned `pid_t` instance per control loop, initialized once,
then stepped once per fixed-period control cycle via `epic_pid_update()`.

- **Vendor-agnostic**: a PID controller is arithmetic on plain data,
  not a hardware operation, so `src/pid.c` is one file, compiles
  unchanged for host, PIC16, and PIC18. No per-family backend, no
  inline asm. The host unit suite tests the exact code that ships
  on-target.
- **Fixed-point Q8.8 throughout** (gains, integrator, intermediate
  terms), so the same source runs on 8-bit parts that pay dearly
  for `float` (XC8 emulates it in software). The only non-trivial
  arithmetic primitive is a 16x16->32 signed multiply, which
  `epic-pid` gets from `epic-math` (the host reference build links
  no HAL; the XC8 cross-compile links `epic-math`'s real per-family
  asm backend).
- **Discrete-time gains, pre-scaled by the sample period**: the
  caller calls `epic_pid_update` at a fixed, known period `Ts` and
  pre-scales `Ki`/`Kd` by `Ts` once at configuration. This keeps
  `epic_pid_update` division-free (the most expensive primitive
  `epic-math` provides on PIC16) and lets the module have zero
  dependency on any timebase.
- **Anti-windup by clamping** (the integrator is hard-capped to
  the Q8.8 clamp rails after every accumulation, not back-
  calculation or conditional integration). Recovery from a
  saturated state is immediate on the very next call when the
  error reverses sign.
- **Derivative-on-measurement only** (no setpoint-step derivative
  kick, by construction). The first call after `epic_pid_init` or
  `epic_pid_reset` zeroes the D term, gating it on
  `have_prev_measurement`.
- **Bumpless AUTO<->MANUAL transfer built in**: `epic_pid_set_mode` does
  not touch state, and `epic_pid_update` back-calculates the integrator
  in MANUAL so the next AUTO call returns the held manual value
  exactly (no output jump at the handoff).

## Documentation

- [API reference](docs/API.md), per-function semantics, the
  Kp/Ki/Kd/Ts -> Q8.8 conversion formulas, the documented-not-guarded
  preconditions.

## Quick start

### Host simulator

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # 13 test cases, 156 checks, all pass
./build/example_pid_setpoint_step              # setpoint step + manual/auto transfer
```

### Real target (XC8)

Cross-compile sanity check + footprint report. `pid.c` is family-
agnostic so this is a portability check, not a second correctness
gate (correctness is fully proven on host).

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make -C mcu/pic16f87xa-pid-mplabx MCU=16F877A   # also 873A / 874A / 876A
make -C mcu/pic18fxx5x-pid-mplabx MCU=18F4550   # also 2455 / 2550 / 4455
```

## Use it

```c
#include "pid.h"

/* A 100 Hz loop, tuned Kp = 1.0, Ki = 5.0, Kd = 0.001. Output
 * clamped to [-1000, 1000]. */
#define TS_SEC (1.0f / 100.0f)
static int16_t q8(float x) { return (int16_t)(x * 256.0f); }

static pid_t g_loop;

void control_init(void)
{
    epic_pid_init(&g_loop,
             q8(1.0f),                  /* Kp          */
             q8(5.0f * TS_SEC),         /* Ki * Ts     */
             q8(0.001f / TS_SEC),       /* Kd / Ts     */
             (int16_t)-1000, (int16_t)1000);
}

/* Call once per fixed-period control cycle, in EITHER mode. */
void control_tick(int16_t setpoint, int16_t measurement)
{
    int16_t output = epic_pid_update(&g_loop, setpoint, measurement);
    drive_actuator(output);
}

/* Operator / supervisor takes over (e.g. a manual jog from a UI). */
void control_take_manual(int16_t target)
{
    epic_pid_set_manual_output(&g_loop, target);
    epic_pid_set_mode(&g_loop, PID_MODE_MANUAL);
}

/* Operator / supervisor hands back. The first AUTO call returns the
 * held manual value exactly, no output jump. */
void control_resume_auto(void)
{
    epic_pid_set_mode(&g_loop, PID_MODE_AUTO);
}
```

## License

MIT, see the [repo LICENSE](../LICENSE).
