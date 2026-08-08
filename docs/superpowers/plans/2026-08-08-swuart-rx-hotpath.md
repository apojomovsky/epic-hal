# epic-swuart RX Hot-Path Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix `epic-swuart`'s RX timing race on PIC16F87XA (channel A):
replace the scheduled, racing confirm-then-arm sequence with a
synchronous deglitch check and a relative-reload deadline, and cut real
overhead from the RX capture handler via direct SFR access, so 9600
baud RX on PIC16F877A actually has real timing margin instead of a
guaranteed miss.

**Architecture:** One new, PIC16F87XA/CCP1-specific function,
`rx_capture_event_fast`, replaces channel A's use of the existing
generic `rx_capture_event`. The deglitch check and the first-sample
arm happen in one synchronous pass; the arm is a relative reload off a
freshly-read Timer1 value, not an absolute deadline computed from the
edge. Channel B (PIC16F193X only) and PIC18Fxx5x are untouched in this
plan and keep the old, racy mechanism; this is a disclosed, deliberate
scope limit, not an oversight.

**Tech Stack:** C99, MPLAB XC8, CMake + ctest (host sim), real `mdb`
hardware probing on PIC16F877A.

**Design spec:** `docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md`.

## Global Constraints

- **No em-dashes.**
- **Scope is PIC16F87XA channel A only.** Do not touch `tx_compare_event`,
  `EPIC_SWUART_Write`, or anything TX-side. Do not touch `on_rx_event_b`,
  PIC18Fxx5x, or PIC16F193X's channel B in this plan; porting the same
  pattern to them is an explicit follow-up.
- **The old `rx_capture_event(h, rx_inst)` function is not removed.**
  Channel B (`on_rx_event_b`, PIC16F193X only) keeps calling it
  unchanged. Since it is now used only when `EPIC_SWUART_MAX_CHANNELS
  >= 2`, it (and its supporting `test_get_capture`/`swuart_test_set_capture`
  test hooks, which only that function calls) must move behind
  `#if EPIC_SWUART_MAX_CHANNELS >= 2`, or PIC16F87XA/PIC18Fxx5x builds
  would carry dead code with no caller at all.
- **`RX_CONFIRM_START` stays in the enum.** `rx_capture_event` (channel
  B's unchanged path) still uses it. Only channel A's new function
  never enters that state.
- **CCP1's real register addresses** (confirmed against
  `pic16f87xa_ccp.c`'s own `addrs[0]` entry during this plan's own
  research, not assumed): `CPRL = 0x15`, `CPRH = 0x16`, `CON = 0x17`.
- **`RX_CAPTURE_OVERHEAD_CYCLES` starts at 0** (no correction) in Task
  1, explicitly documented as unverified. Task 2's real `mdb` probe
  replaces it with a measured value. Do not guess a nonzero starting
  value; a wrong guess is worse than an honest, disclosed zero.
- **Host-sim cannot model real CCP timing.** The existing
  `#if EPIC_SWUART_TEST_HOOKS`-gated test-hook idiom continues to
  verify state-machine correctness only; real margin is only provable
  via `mdb`.
- **`test_swuart_dual.c` needs no changes.** Confirmed by reading it:
  it only ever calls `swuart_test_fire_rx_event_b` (channel B) for RX,
  never touches channel A's RX path, so channel A's rewrite doesn't
  affect it.
- **Four test files exercise channel A's RX path with the old
  two-fire capture-then-confirm sequence, not one.** Confirmed by
  reading all of them: `test_swuart_rx.c`, `test_swuart_errors.c`,
  `test_swuart_deinit.c` (all three, all families, via
  `swuart_test_fire_rx_event` + `swuart_test_set_capture`), and
  `test_swuart_dual_deinit.c`'s `receive_byte_a` helper (PIC16F193X
  only, but it exercises channel A specifically, the same CCP1/CCP2
  pins as every other family). All four need the same update: one fire
  instead of two, `swuart_test_set_capture_fast` instead of
  `swuart_test_set_capture` for these channel-A-specific calls.
  `test_swuart_dual_deinit.c`'s *other* capture-value call (for channel
  B, via `swuart_test_fire_rx_event_b`) is untouched, same as
  `test_swuart_dual.c`'s.
- **Conventional Commits, trailing newline, no trailing whitespace,
  commit after every task.**

---

## Task 1: Implement the collapsed RX state machine and fast path

**Files:**
- Modify: `epic-swuart/src/epic_swuart.c`
- Modify: `epic-swuart/tests/test_swuart_rx.c`
- Modify: `epic-swuart/tests/test_swuart_errors.c`
- Modify: `epic-swuart/tests/test_swuart_deinit.c`
- Modify: `epic-swuart/tests/test_swuart_dual_deinit.c`

**Interfaces:**
- Consumes: `EPIC_TIMER1_ReadCounter()` (unchanged, keeps its own
  atomicity protection since Timer1 genuinely free-runs), `g_cycles_per_bit`,
  `rx_push`, `RX_IDLE`/`RX_DATA0..7`/`RX_STOP` (unchanged enum values).
- Produces: `rx_capture_event_fast(EPIC_SWUART_HandleTypeDef *h)`,
  `RX_CAPTURE_OVERHEAD_CYCLES` (starts at `0u`), `swuart_test_set_capture_fast`.
  Task 2 measures the real value for `RX_CAPTURE_OVERHEAD_CYCLES` and
  updates the `#define`. `test_swuart_errors.c`, `test_swuart_deinit.c`,
  and `test_swuart_dual_deinit.c` all consume `swuart_test_set_capture_fast`
  too, alongside `test_swuart_rx.c`.

- [ ] **Step 1: Write the failing test**

Read `epic-swuart/tests/test_swuart_rx.c` in full first (it currently
fires the capture event, then a separately-scheduled confirm event,
two fires before the per-bit loop). Replace it:

```c
/**
 * @file    test_swuart_rx.c
 * @brief   RX-only host test, PIC16F87XA channel A, RX hot-path fix:
 *          the deglitch check and the first-sample arm now happen in
 *          one synchronous pass (rx_capture_event_fast), not two
 *          separately-scheduled events. One fire now does what used
 *          to take two. No real CCP hardware in host sim, so this test
 *          drives the state machine directly via test-only hooks and
 *          injects the captured edge value straight into the same
 *          CCP1 registers rx_capture_event_fast itself reads.
 */
#include <stdio.h>
#include "epic_swuart.h"

#if defined(PIC18F2455) || defined(PIC18F2550) || defined(PIC18F4455) || defined(PIC18F4550)
  #include "pic18fxx5x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic18_sim_drive_input((port), (pin), (lvl))
#elif defined(PIC16F1933) || defined(PIC16F1934) || defined(PIC16F1936) || \
      defined(PIC16F1937) || defined(PIC16F1938) || defined(PIC16F1939)
  #include "pic16f193x_sim.h"
  #define SIM_DRIVE(port, pin, lvl) \
      do { pic16f193x_sim_drive_input((port), (pin), (lvl)); pic16f193x_sim_step(1); } while (0)
#else
  #include "pic16f87xa_sim.h"
  #define SIM_DRIVE(port, pin, lvl) pic16f87xa_sim_drive_input((port), (pin), (lvl))
#endif

static int g_fails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); g_fails++; } } while (0)

extern void swuart_test_fire_rx_event(void);
extern void swuart_test_set_capture_fast(uint16_t value);

int main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, 20000000UL, 9600u);

    /* 'A' = 0x41 = 0b01000001, LSB first: start=0,1,0,0,0,0,0,1,0,stop=1. */
    static const uint8_t bits[] = {0, 1, 0, 0, 0, 0, 0, 1, 0, 1};

    SIM_DRIVE('C', 2, bits[0]);
    swuart_test_set_capture_fast(1000u);
    swuart_test_fire_rx_event(); /* one fire: deglitch check (bits[0]=0,
                                   * still on the line, passes) AND arms
                                   * d0's deadline, IDLE -> DATA0. The
                                   * old two-fire sequence (capture,
                                   * then a separately-scheduled
                                   * confirm) collapses into this one
                                   * synchronous pass; see
                                   * rx_capture_event_fast. */

    for (size_t i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2, bits[i]);
        swuart_test_fire_rx_event(); /* compare event: sample + arm next */
    }

    uint8_t buf[4] = {0};
    int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));
    CHECK(n == 1, "one byte read");
    CHECK(buf[0] == 0x41u, "byte == 'A'");
    CHECK(EPIC_SWUART_GetErrorCount(&h) == 0u, "no framing errors");

    printf("swuart_rx: fails=%d\n", g_fails);
    return g_fails == 0 ? 0 : 1;
}
```

(Nine fires in the loop, one before it: ten total, one fewer than the
old eleven, since the confirm step no longer costs its own fire.)

- [ ] **Step 2: Run it to verify it fails**

```sh
cmake -B build -S epic-swuart && cmake --build build
```

Expected: link failure (`swuart_test_set_capture_fast` does not exist
yet) or, if it happens to link against a stale build, a failing
`swuart_rx` run (the old `rx_capture_event` still expects two fires
for start-bit sync, so the new ten-fire sequence would desync every
subsequent bit).

- [ ] **Step 3: Implement the fix**

Read the current `epic-swuart/src/epic_swuart.c` in full first (this
plan's own research already confirmed CCP1's addresses match
`pic16f87xa_ccp.c`'s `addrs[0] = { 0x15U, 0x16U, 0x17U, PIC16_IRQ_CCP1 }`
exactly; do not re-derive them from anywhere else).

First, wrap the existing generic RX machinery so it only compiles when
channel B needs it. Find this block (currently unconditional):

```c
#if EPIC_SWUART_TEST_HOOKS
static uint16_t g_test_capture_value = 0u;
void swuart_test_set_capture(uint16_t value) { g_test_capture_value = value; }
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { (void)rx_inst; return g_test_capture_value; }
#else
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { return EPIC_CCP_GetCapture(rx_inst); }
#endif

/* Shared RX capture/compare event body, parameterised by handle and CCP
 * instance the same way tx_compare_event is: on_rx_event_a/_b below are
 * thin per-slot wrappers over this. */
static void rx_capture_event(EPIC_SWUART_HandleTypeDef *h, CCP_InstanceTypeDef rx_inst)
{
    /* ... existing body, unchanged ... */
}
```

Wrap the whole thing in `#if EPIC_SWUART_MAX_CHANNELS >= 2` /
`#endif`, since after this task the only remaining caller is
`on_rx_event_b`, which itself only exists on that condition:

```c
#if EPIC_SWUART_MAX_CHANNELS >= 2
#if EPIC_SWUART_TEST_HOOKS
static uint16_t g_test_capture_value = 0u;
void swuart_test_set_capture(uint16_t value) { g_test_capture_value = value; }
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { (void)rx_inst; return g_test_capture_value; }
#else
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { return EPIC_CCP_GetCapture(rx_inst); }
#endif

static void rx_capture_event(EPIC_SWUART_HandleTypeDef *h, CCP_InstanceTypeDef rx_inst)
{
    /* ... existing body, unchanged ... */
}
#endif /* EPIC_SWUART_MAX_CHANNELS >= 2 */
```

Directly below that (still before `on_rx_event_a`'s definition), add
channel A's fast path:

```c
/* Direct SFR addresses for PIC16F87XA's CCP1 (channel A's RX capture),
 * confirmed against pic16f87xa_ccp.c's own addrs[0] entry. Bypasses
 * EPIC_CCP_GetCapture's atomic retry-loop (unnecessary here: the
 * earliest a second real capture could land is a full cycles_per_bit
 * away, ~521 cycles at 9600 baud, vastly longer than the ~10-15 cycles
 * this plain 2-byte read takes) and EPIC_CCP_SetCompare/SetMode's
 * generic call overhead, for this one hot path only. Hardcoded to
 * CCP1 specifically, not parameterised by instance: channel B
 * (PIC16F193X, CCP3) keeps using the slower, generic rx_capture_event
 * above via on_rx_event_b, a disclosed, deliberate scope limit of this
 * fix (see docs/ARCHITECTURE.md's "Known limitations" section).
 * Porting this same pattern to channel B and to PIC18Fxx5x/PIC16F193X's
 * own literal addresses is a follow-up, not this fix. */
#define CCP1_CPRL_ADDR 0x15U
#define CCP1_CPRH_ADDR 0x16U
#define CCP1_CON_ADDR  0x17U

/* Cycles elapsed, on real PIC16F87XA hardware, between the real
 * falling edge and the point inside rx_capture_event_fast() where
 * Timer1 is read fresh (the AN555-style _Cycle_Offset1 correction).
 * Starts at 0 (no correction) until Task 2's real mdb probe replaces
 * it with a measured value. Do not trust this number, or add a guessed
 * nonzero value, before that probe has run: this exact kind of
 * unverified-arithmetic mistake is what produced the 404-vs-260 race
 * this fix exists to close. */
#define RX_CAPTURE_OVERHEAD_CYCLES 0u

#if EPIC_SWUART_TEST_HOOKS
/* Writes straight into the same two registers rx_capture_event_fast
 * itself reads, so an injected test value flows through the exact
 * production code path, not a separate indirection layer. */
void swuart_test_set_capture_fast(uint16_t value)
{
    EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(value >> 8);
    EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(value & 0xFFu);
}
#endif

static void rx_capture_event_fast(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state != RX_IDLE) {
        /* Steady-state per-bit sampling, unchanged from v3's own
         * arithmetic: each event only needs to beat the *next* one by
         * a full cycles_per_bit, already proven to fit with real
         * margin (v3's TX-side measurement). */
        uint8_t sample = (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_SET) ? 1u : 0u;

        if (h->rx_state == RX_STOP) {
            if (sample != 0u) {
                rx_push(h, h->rx_shift);
            } else {
                h->error_count++;
            }
            h->rx_state = RX_IDLE;
            EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_CAPTURE_FALLING;
            return;
        }

        h->rx_shift = (uint8_t)((h->rx_shift >> 1) | (sample ? 0x80u : 0u));
        h->rx_bit_index++;
        h->rx_state = (h->rx_bit_index < 8u) ? (uint8_t)(RX_DATA0 + h->rx_bit_index) : RX_STOP;
        h->rx_deadline = (uint16_t)(h->rx_deadline + g_cycles_per_bit);
        EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
        EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
        return;
    }

    /* RX_IDLE: a start-bit falling edge just latched into CCPR1.
     * Immediate, synchronous deglitch check, no second scheduled
     * event (see docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md
     * for why this removes the v3 timing race). */
    if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
        return; /* noise: pin already back high, stay in Capture mode */
    }

    h->rx_shift = 0u;
    h->rx_bit_index = 0u;
    h->rx_state = RX_DATA0;

    /* Relative reload: "now" is read after this handler's own real
     * latency has already elapsed, so the deadline can never be in
     * the past, unlike v3's edge_time + 0.5*cycles_per_bit scheme.
     * RX_CAPTURE_OVERHEAD_CYCLES corrects for that already-elapsed
     * latency so the arm still lands close to the intended 1.5-bit
     * mark relative to the real edge, not relative to "now". */
    uint16_t now = EPIC_TIMER1_ReadCounter();
    uint16_t target_offset = (uint16_t)(g_cycles_per_bit + g_cycles_per_bit / 2u);
    h->rx_deadline = (uint16_t)(now + target_offset - RX_CAPTURE_OVERHEAD_CYCLES);

    EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
    EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
    EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_COMPARE_SOFT_IF;
}
```

Finally, change channel A's wrapper to call the new function (channel
B's wrapper is untouched):

```c
static void on_rx_event_a(void) { rx_capture_event_fast(g_chan_a); }
#if EPIC_SWUART_MAX_CHANNELS >= 2
static void on_rx_event_b(void) { rx_capture_event(g_chan_b, SWUART_CCP_RX_B); }
#endif
```

`swuart_test_fire_rx_event`'s own definition (`void
swuart_test_fire_rx_event(void) { on_rx_event_a(); }`) does not change
at all; it already just calls `on_rx_event_a`, which now does more
per call.

- [ ] **Step 4: Update the three other test files that exercise
  channel A's RX path**

Three more files use the exact old two-fire sequence against channel
A's fixed pins (confirmed by reading all of them during this plan's
own research), not just `test_swuart_rx.c`. Each needs the same two
changes: `swuart_test_set_capture(N)` becomes
`swuart_test_set_capture_fast(N)`, and the two fires (`/* capture
event: IDLE -> CONFIRM_START */` then `/* confirm event, half a bit
later */`) become one.

In `epic-swuart/tests/test_swuart_errors.c`, `receive_byte`'s body:

```c
static void receive_byte(const uint8_t *bits)
{
    SIM_DRIVE('C', 2, bits[0]);
    swuart_test_set_capture_fast(1000u);
    swuart_test_fire_rx_event(); /* one fire: deglitch check + arm d0,
                                   * IDLE -> DATA0 (see test_swuart_rx.c
                                   * for why this collapsed from two). */
    for (size_t i = 1; i < 10; i++) {
        SIM_DRIVE('C', 2, bits[i]);
        swuart_test_fire_rx_event();
    }
}
```

Change the `extern void swuart_test_set_capture(uint16_t value);`
declaration near the top of the file to
`extern void swuart_test_set_capture_fast(uint16_t value);` to match.
The file's own docstring claims PIC16F193X's channel B "shares the
identical handler body... so this coverage applies there too without a
second copy of the test": after this task that is no longer quite
true (channel A now has its own distinct fast path), update that
comment to say the error-handling *logic* (bad-stop-bit and
ring-overflow counting) is still shared in spirit between
`rx_capture_event_fast` and `rx_capture_event`, but the two are now
separate functions, not one shared body, so this file's coverage
speaks only to channel A's path; channel B's own coverage still comes
from the same file running unmodified there (it never touches channel
A's changed code either way, so no new gap, just a less precise old
claim).

In `epic-swuart/tests/test_swuart_deinit.c`, both places that inject a
capture value for channel A (there are two: once for the original
channel, once for `chan2` after re-`Init`), apply the identical
two-changes-in-one edit: `swuart_test_set_capture` to
`swuart_test_set_capture_fast`, two fires to one. Update its own
`extern` declaration the same way.

In `epic-swuart/tests/test_swuart_dual_deinit.c`, `receive_byte_a`'s
body only (not the channel-B capture-value call a few lines later in
`main`, which stays exactly as it is, since channel B is untouched by
this fix):

```c
static void receive_byte_a(const uint8_t *bits)
{
    pic16f193x_sim_drive_input('C', 2, bits[0]);
    pic16f193x_sim_step(1);
    swuart_test_set_capture_fast(2000u);
    swuart_test_fire_rx_event();
    for (size_t i = 1; i < 10; i++) {
        pic16f193x_sim_drive_input('C', 2, bits[i]);
        pic16f193x_sim_step(1);
        swuart_test_fire_rx_event();
    }
}
```

This file declares both `swuart_test_set_capture` (for channel B,
untouched, keep this `extern` declaration) and needs a new
`extern void swuart_test_set_capture_fast(uint16_t value);` added
alongside it for channel A's calls in `receive_byte_a`.

- [ ] **Step 5: Run the tests, verify they pass**

```sh
cmake -B build -S epic-swuart && cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all of `swuart_tx`, `swuart_rx`, `swuart_errors`,
`swuart_deinit` pass on the default (PIC16F87XA) family, all
genuinely exercising the new one-fire sequence now, not just
unaffected bystanders. Repeat for the other two families, including
`swuart_dual_deinit` on PIC16F193X (it also exercises channel A's
changed path via `receive_byte_a`, alongside channel B's unchanged
one):

```sh
cmake -B build18 -S epic-swuart -DEPIC_FAMILY=PIC18 && cmake --build build18 && ctest --test-dir build18 --output-on-failure
cmake -B build193x -S epic-swuart -DEPIC_FAMILY=PIC16F193X && cmake --build build193x && ctest --test-dir build193x --output-on-failure
```

`swuart_dual` is the one test genuinely untouched end to end (it never
exercises channel A's RX at all); it should still pass unchanged.

- [ ] **Step 6: Commit**

```bash
git add epic-swuart/src/epic_swuart.c epic-swuart/tests/test_swuart_rx.c \
        epic-swuart/tests/test_swuart_errors.c epic-swuart/tests/test_swuart_deinit.c \
        epic-swuart/tests/test_swuart_dual_deinit.c
git commit -m "fix(swuart): collapse RX confirm+arm into one synchronous pass, PIC16F87XA channel A"
```

---

## Task 2: Real mdb probe, derive the real overhead constant

**Files:**
- Create: `docs/superpowers/plans/probe-swuart-rx-hotpath.md`
- Modify: `epic-swuart/src/epic_swuart.c` (only the
  `RX_CAPTURE_OVERHEAD_CYCLES` value, once measured)

**Interfaces:**
- Consumes: Task 1's `rx_capture_event_fast`, the real, compiled,
  final module (not a mirror).
- Produces: a real, measured `RX_CAPTURE_OVERHEAD_CYCLES` value, and a
  real verdict on whether the fix actually closes the race.

Unlike v3's own Task 2 (which had to probe a purpose-built mirror
because the real module didn't exist yet), Task 1 has already produced
the real, final `rx_capture_event_fast`. This probe measures the real
thing directly, the more rigorous version of this project's own
"probe before trusting" rule, not a weaker one.

- [ ] **Step 1: Build a real probe program linking the actual sources**

Outside the repo, `/tmp/swuart-rx-hotpath-probe/probe.c`, PIC16F877A,
linking the real `pic16f87xa_gpio.c`/`pic16f87xa_timer1.c`/
`pic16f87xa_ccp.c`/`pic16_irq.c`/`pic16_irq_dispatch.c`/
`pic16_isr_vector.c` sources plus the real, unmodified
`epic-swuart/src/epic_swuart.c` itself (link the real module this
time, not a stand-in, since Task 1 already built it):

```c
#include <xc.h>
#include "epic_swuart.h"

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

void main(void)
{
    EPIC_SWUART_HandleTypeDef h;
    EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, 20000000UL, 9600u);
    for (;;) { }
}
```

```sh
mkdir -p /tmp/swuart-rx-hotpath-probe && cd /tmp/swuart-rx-hotpath-probe
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin:/opt/microchip/mplabx/v6.35/mplab_platform/bin
REPO=/home/alexis/projects/epicurus
xc8-cc -mcpu=16f877a -I$REPO/pic16f87xa-hal/include/target -I$REPO/pic16f87xa-hal/include \
  -I$REPO/epic-common/include -I$REPO/epic-swuart/include \
  -mdfp=/opt/microchip/mplabx/v6.35/packs/Microchip/PIC16Fxxx_DFP/1.8.167/xc8 \
  -O2 -std=c99 -fasmfile probe.c \
  $REPO/epic-swuart/src/epic_swuart.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_gpio.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_timer1.c \
  $REPO/pic16f87xa-hal/src/peripherals/pic16f87xa_ccp.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq.c \
  $REPO/pic16f87xa-hal/src/core/pic16_irq_dispatch.c \
  $REPO/pic16f87xa-hal/src/core/pic16_isr_vector.c \
  -o probe.hex -ginhx32
```

(`pic16_irq_dispatch.c` declares every peripheral `IRQHandler` with a
strong prototype; link-satisfying empty stubs are needed for the
peripherals not under test, `TIMER0_IRQHandler`, `TIMER2_IRQHandler`,
`SSP_IRQHandler`, `USART_RX_IRQHandler`, `USART_TX_IRQHandler`,
`ADC_IRQHandler`, `EEPROM_IRQHandler`, `COMP_IRQHandler`,
`PSP_IRQHandler`, `RB_IRQHandler`, the same pattern v3's own Task 2
probe used, since their flags are never set here and cannot perturb
the measurement.)

- [ ] **Step 2: Find the real symbol addresses**

```sh
readelf -s probe.elf | grep -i "rx_capture_event_fast\|PIC16_IRQ_Handler"
```

Find the `retfie` address the same way v3's own Task 2 probe did:
disassemble forward from the dispatch entry in `mdb`
(`x /80i <dispatch_addr>`) since `-fasmfile` does not reliably produce
a full linked listing.

- [ ] **Step 3: Drive a real falling edge and measure**

Same breakpoint-and-stopwatch technique used repeatedly in this
module's history: `break *0x0004`, `break *<retfie address>`, drive
`RC2` (CCP1's real capture pin) low via `mdb`'s `write pin RC2 low`
(confirmed working syntax from v3's own Task 2 probe), `Stopwatch
clear` / `Continue` / `Stopwatch`.

Measure twice: once for the `RX_IDLE` branch (the fix's core claim,
first falling edge after `Init`), and once for a steady-state
`RX_DATA*` branch (drive a second edge after manually advancing
`TMR1H:TMR1L` past the first arm, or via a second `write pin` toggle
timed to land inside the armed compare window) to confirm the
unchanged per-bit sampling code still costs what v3's TX-side
measurement suggested it should.

- [ ] **Step 4: Compute the real overhead correction**

The measured `RX_IDLE`-branch cost (vector to `retfie`) minus the
already-known, unrelated dispatch/vector overhead common to every CCP1
event (already measured in v3's own Task 2 probe as part of that
event's total) is the real "cycles already elapsed by the time `now`
is read" value. Record the derivation explicitly, do not just assert a
number.

- [ ] **Step 5: Record the verdict**

Create `docs/superpowers/plans/probe-swuart-rx-hotpath.md` with the
exact commands, exact output, the derived `RX_CAPTURE_OVERHEAD_CYCLES`
value, and a plain verdict: does the fix actually remove the race (no
65536-cycle wraparound observed), and does the resulting d0 sample
point land within a reasonable margin of the intended 1.5-bit-period
mark (state the real number, e.g. "lands at 1.4 bit periods after the
edge, comfortably within d0's [1.0, 2.0) window"), not just "no longer
crashes." If the real numbers reveal a problem (margin still too
tight, correction constant swings with rebuilds), report that
honestly rather than rounding it up to "fine."

- [ ] **Step 6: Apply the real constant and re-verify**

Update `RX_CAPTURE_OVERHEAD_CYCLES` in `epic_swuart.c` from `0u` to the
real measured value from Step 4, with a comment citing the probe
document. Re-run the full host-sim suite on all three families (the
exact value doesn't change host-sim behavior, this just confirms
nothing else broke):

```sh
cmake -B build -S epic-swuart && cmake --build build && ctest --test-dir build --output-on-failure
cmake -B build18 -S epic-swuart -DEPIC_FAMILY=PIC18 && cmake --build build18 && ctest --test-dir build18 --output-on-failure
cmake -B build193x -S epic-swuart -DEPIC_FAMILY=PIC16F193X && cmake --build build193x && ctest --test-dir build193x --output-on-failure
```

- [ ] **Step 7: Commit**

```bash
git add docs/superpowers/plans/probe-swuart-rx-hotpath.md epic-swuart/src/epic_swuart.c
git commit -m "fix(swuart): derive RX_CAPTURE_OVERHEAD_CYCLES from a real mdb probe"
```

---

## Task 3: Documentation

**Files:**
- Modify: `epic-swuart/docs/ARCHITECTURE.md`
- Modify: `docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md`
- Modify: `docs/superpowers/specs/2026-08-07-swuart-v3-design.md`

**Interfaces:**
- Consumes: Task 2's real, verified fix and measured numbers.
- Produces: nothing downstream, last task.

- [ ] **Step 1: Update `docs/ARCHITECTURE.md`**

Replace the RX flow description's account of the confirm step (a
separately-scheduled compare event) with the real, current mechanism:
one synchronous pass, immediate deglitch check, relative-reload arm,
citing the real `RX_CAPTURE_OVERHEAD_CYCLES` value from Task 2's
probe. Add or update a "Known limitations" section stating plainly
that this fix applies to PIC16F87XA channel A only: PIC18Fxx5x and
PIC16F193X's channel B still use the older, racy confirm-then-arm
scheme (the same one this fix replaces) until a follow-up ports the
same pattern to them. This is a real, currently-true limitation; state
it as plainly as the v3 spec disclosed the real-hardware-RX-unverified
limitation before this fix existed.

- [ ] **Step 2: Close out this spec**

In `docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md`,
change `Status:` from `agreed 2026-08-08, not started` to `implemented
<real date>`, and replace the placeholder `RX_CAPTURE_OVERHEAD_CYCLES`
value in the illustrative code block with the real one from Task 2,
citing `docs/superpowers/plans/probe-swuart-rx-hotpath.md`.

- [ ] **Step 3: Update the v3 spec's disclosed limitation**

In `docs/superpowers/specs/2026-08-07-swuart-v3-design.md`'s `Status:`
line, add a note that the real-hardware-RX-unverified limitation it
disclosed is now resolved for PIC16F87XA channel A specifically (cite
this fix's spec and probe doc), while noting it still applies to
PIC18Fxx5x and PIC16F193X's channel B until they are ported. Do not
overclaim: this fix does not touch those two, and the v3 spec's
original disclosure remains accurate for them.

- [ ] **Step 4: Verify and commit**

```bash
grep -rnP '\x{2014}' epic-swuart/docs/ARCHITECTURE.md docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md docs/superpowers/specs/2026-08-07-swuart-v3-design.md && echo "FAIL: em-dash" || echo "OK"
git add epic-swuart/docs/ARCHITECTURE.md docs/superpowers/specs/2026-08-08-swuart-rx-hotpath-design.md docs/superpowers/specs/2026-08-07-swuart-v3-design.md
git commit -m "docs(swuart): document the RX hot-path fix, close its spec, update v3's disclosed limitation"
```

---

## Done when

- `rx_capture_event_fast` exists, is used by channel A only, and never
  enters `RX_CONFIRM_START` (removed from its own logic, though the
  enum value itself remains for channel B's continued use).
- `RX_CAPTURE_OVERHEAD_CYCLES` holds a real, `mdb`-measured value, not
  the initial `0u` placeholder.
- All host-sim tests pass on all three families.
- A real `mdb` probe confirms the fix removes the wraparound-recovery
  failure mode and lands d0's sample within a stated, real margin of
  its intended point.
- Documentation states plainly that PIC18Fxx5x and PIC16F193X's
  channel B still carry the older, unfixed mechanism.

## What this plan deliberately does not do

- Touch TX in any way.
- Port the fix to PIC18Fxx5x or PIC16F193X's channel B. A disclosed,
  deliberate follow-up.
- Remove `EPIC_CCP_GetCapture`'s retry-loop protection for any caller
  other than this one new hot-path call site.
- Change the public API, framing, or error-handling contract.
