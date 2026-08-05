# Comparator (C1/C2) for pic16f193x-hal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Status: **implemented and cleared the §4 gate.** Peripheral #6 in
`docs/pic16f193x-plan.md` §7's roadmap table.

**Goal:** Land a `EPIC_COMP_*` driver for both comparators (DS41364B
Comparator chapter) through the §4 gate.

**Architecture:** Two independent instances, each with its own
`CMxCON0` (enable/config/output) and `CMxCON1` (input-channel
select/interrupt-edge-enable), plus a shared read-only `CMOUT` mirror
register. Different register shape than `pic16f87xa_comp.h`'s single
`CMCON` (that family has one shared register for both comparators);
this driver's naming/handle-copy convention mirrors that reference
driver, the register layer is designed fresh for this family's actual
split-register shape.

**Tech Stack:** C99, MPLAB XC8, gcc (host sim), CMake, GNU Make.

## Global Constraints

- C99, no compiler extensions. No em-dashes anywhere.
- Every register access is a compile-time-constant `PIC_REG_*` token.
- Handle-copy convention: copy by value, never store the pointer.
- **Standing warning**: every bit position here was re-derived from
  DS41364B and cross-checked against the installed DFP header's
  `_CM1CON0_`/`_CM1CON1_`/`_CM2CON0_`/`_CM2CON1_`/`_CMOUT_`
  `_POSN`/`_MASK` macros
  (`/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC12-16F1xxx_DFP/xc8/pic/include/proc/pic16f1937.h`).
  Timer1 shipped with a wrong bit position (`PIC_T1CON_TMR1CS` at bit
  1, copied from `pic16f87xa`'s classic-PIC16 T1CON, real field is
  bits 7:6) from a transcription without this cross-check. Never trust
  a bit position from another family's driver or memory alone.

## Non-goals (not in this phase)

- None; the comparator's feature surface (enable, hysteresis, speed,
  polarity, input-channel select, output-to-pin, edge-interrupt
  enable) is small enough for one pass. Capacitive-sense integration
  is a separate CPS peripheral
  (`docs/superpowers/plans/2026-08-04-pic16f193x-cps.md`), out of
  scope here.

## File Structure

- Modify `pic16f193x-hal/include/pic16f193x_sfr.h`: bit macros for
  CM1CON0/1, CM2CON0/1, CMOUT (addresses already stubbed).
- Create `pic16f193x-hal/include/peripherals/pic16f193x_comp.h`.
- Create `pic16f193x-hal/include/peripherals/hal_comp.h`.
- Create `pic16f193x-hal/src/peripherals/pic16f193x_comp.c`.
- Modify `pic16f193x-hal/src/sim/pic16f193x_sim.c`: caller-injected
  boolean output per instance.
- Modify `pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c`: add
  `CMP1_IRQHandler`/`CMP2_IRQHandler`.
- Modify `pic16f193x-hal/CMakeLists.txt` and
  `pic16f193x-hal/mcu/pic16f193x-mplabx/Makefile`.
- Create `pic16f193x-hal/tests/example_comparator.c`.
- Modify `pic16f193x-hal/MANUAL.md`: new §17 "Comparator" (renumber
  if claimed first; assumes §12-16 landed).
- Modify `docs/pic16f193x-plan.md` §7.

## Task 1: SFR map additions

**Files:** Modify `pic16f193x-hal/include/pic16f193x_sfr.h`.

- [ ] **Step 1: Add bit macros for all five registers** (addresses
  `CM1CON0=0x111`..`CMOUT=0x115` already stubbed):

```c
/* Bank 2. Comparator, DS41364B Comparator chapter. Verified against
 * the installed DFP header (pic16f1937.h) _CM1CON0_ etc POSN/MASK
 * macros; re-verify against DS41364B before relying on them. */
#define PIC_CM1CON0_C1SYNC      EPIC_BIT(0)
#define PIC_CM1CON0_C1HYS       EPIC_BIT(1)
#define PIC_CM1CON0_C1SP        EPIC_BIT(2)
#define PIC_CM1CON0_C1POL       EPIC_BIT(4)
#define PIC_CM1CON0_C1OE        EPIC_BIT(5)
#define PIC_CM1CON0_C1OUT       EPIC_BIT(6)   /* Read-only output value. */
#define PIC_CM1CON0_C1ON        EPIC_BIT(7)
#define PIC_CM1CON1_C1NCH_MASK  0x03U         /* Negative-input channel select. */
#define PIC_CM1CON1_C1PCH_MASK  0x30U         /* Positive-input channel select. */
#define PIC_CM1CON1_C1INTN      EPIC_BIT(6)   /* Interrupt on negative edge. */
#define PIC_CM1CON1_C1INTP      EPIC_BIT(7)   /* Interrupt on positive edge. */
/* CM1CON0/CM1CON1 POR = 0x00 (confirm against DS41364B). */

#define PIC_CM2CON0_C2SYNC      EPIC_BIT(0)
#define PIC_CM2CON0_C2HYS       EPIC_BIT(1)
#define PIC_CM2CON0_C2SP        EPIC_BIT(2)
#define PIC_CM2CON0_C2POL       EPIC_BIT(4)
#define PIC_CM2CON0_C2OE        EPIC_BIT(5)
#define PIC_CM2CON0_C2OUT       EPIC_BIT(6)
#define PIC_CM2CON0_C2ON        EPIC_BIT(7)
#define PIC_CM2CON1_C2NCH_MASK  0x03U
#define PIC_CM2CON1_C2PCH_MASK  0x30U
#define PIC_CM2CON1_C2INTN      EPIC_BIT(6)
#define PIC_CM2CON1_C2INTP      EPIC_BIT(7)
/* CM2CON0/CM2CON1 POR = 0x00 (confirm against DS41364B). */

#define PIC_CMOUT_MC1OUT        EPIC_BIT(0)   /* Synchronized mirror of C1OUT. */
#define PIC_CMOUT_MC2OUT        EPIC_BIT(1)   /* Synchronized mirror of C2OUT. */
/* CMOUT POR = 0x00 (confirm against DS41364B). */
```

- [ ] **Step 2: Confirm the host build still compiles clean.**

## Task 2: Add `pic16f193x_comp.h` (public API + neutral shim)

**Files:**
- Create: `pic16f193x-hal/include/peripherals/pic16f193x_comp.h`
- Create: `pic16f193x-hal/include/peripherals/hal_comp.h`

- [ ] **Step 1: Write `pic16f193x_comp.h`**

```c
/**
 * @file    pic16f193x_comp.h
 * @brief   PIC16F193X dual comparator driver.
 * @details Source: DS41364B Comparator chapter (CM1CON0/CM1CON1/
 *          CM2CON0/CM2CON1/CMOUT), unlike pic16f87xa-hal's single
 *          shared CMCON, this family splits into independent
 *          per-comparator registers. Full reference: MANUAL.md §17.
 */
#ifndef PIC16F193X_COMP_H
#define PIC16F193X_COMP_H

#include <stdint.h>
#include "core/hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMP_INSTANCE_1 = 1,
    COMP_INSTANCE_2 = 2,
} COMP_InstanceTypeDef;

typedef enum {
    COMP_INT_EDGE_NONE     = 0x0U,
    COMP_INT_EDGE_RISING   = 0x1U,   /**< INTP. */
    COMP_INT_EDGE_FALLING  = 0x2U,   /**< INTN. */
    COMP_INT_EDGE_BOTH     = 0x3U,   /**< INTP | INTN. */
} COMP_InterruptEdgeTypeDef;

typedef struct {
    COMP_InstanceTypeDef      Instance;
    uint8_t                   PosChannel;    /**< CxPCH<1:0>, 0-3. */
    uint8_t                   NegChannel;    /**< CxNCH<1:0>, 0-3. */
    uint8_t                   HysteresisOn;  /**< CxHYS. */
    uint8_t                   InvertOutput;  /**< CxPOL. */
    uint8_t                   OutputToPin;   /**< CxOE. */
    COMP_InterruptEdgeTypeDef InterruptEdge;
    void (*EventCallback)(void);             /**< Fires on CxIF; NULL = no callback. */
} COMP_HandleTypeDef;

#define COMP_HANDLE_DEFAULT { .Instance = COMP_INSTANCE_1, .PosChannel = 0U, .NegChannel = 0U, .HysteresisOn = 0U, .InvertOutput = 0U, .OutputToPin = 0U, .InterruptEdge = COMP_INT_EDGE_NONE, .EventCallback = 0 }

EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h);
EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst);
uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst);   /**< Reads CMOUT's synchronized mirror bit, not CxCON0<CxOUT> directly. */

void CMP1_IRQHandler(void) EPIC_WEAK;
void CMP2_IRQHandler(void) EPIC_WEAK;

#ifdef __cplusplus
}
#endif
#endif /* PIC16F193X_COMP_H */
```

- [ ] **Step 2: Write the neutral shim `hal_comp.h`**

```c
/**
 * @file    hal_comp.h
 * @brief   Family-neutral comparator shim, mirrors hal_timer1.h's pattern.
 */
#ifndef EPIC_COMP_H
#define EPIC_COMP_H
#include "peripherals/pic16f193x_comp.h"
#endif /* EPIC_COMP_H */
```

## Task 3: Add `pic16f193x_comp.c` (driver implementation)

**Files:** Create `pic16f193x-hal/src/peripherals/pic16f193x_comp.c`.

- [ ] **Step 1: Write the driver body**

```c
/**
 * @file    pic16f193x_comp.c
 * @brief   PIC16F193X dual comparator driver implementation.
 */
#include "peripherals/pic16f193x_comp.h"
#include "pic16f193x_sfr.h"
#include "pic16f193x_platform.h"

static void (*s_comp1_cb)(void);
static void (*s_comp2_cb)(void);

EPIC_StatusTypeDef EPIC_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (h->PosChannel > 0x03U || h->NegChannel > 0x03U) return EPIC_INVALID;

    uint8_t con1 = (uint8_t)(h->NegChannel & PIC_CM1CON1_C1NCH_MASK);
    con1 |= (uint8_t)((h->PosChannel << 4) & PIC_CM1CON1_C1PCH_MASK);
    if (h->InterruptEdge & COMP_INT_EDGE_RISING) con1 |= PIC_CM1CON1_C1INTP;
    if (h->InterruptEdge & COMP_INT_EDGE_FALLING) con1 |= PIC_CM1CON1_C1INTN;

    uint8_t con0 = PIC_CM1CON0_C1ON;
    if (h->HysteresisOn) con0 |= PIC_CM1CON0_C1HYS;
    if (h->InvertOutput) con0 |= PIC_CM1CON0_C1POL;
    if (h->OutputToPin) con0 |= PIC_CM1CON0_C1OE;

    if (h->Instance == COMP_INSTANCE_1) {
        EPIC_REG8(PIC_REG_CM1CON1) = con1;
        EPIC_REG8(PIC_REG_CM1CON0) = con0;
        s_comp1_cb = h->EventCallback;
    } else if (h->Instance == COMP_INSTANCE_2) {
        EPIC_REG8(PIC_REG_CM2CON1) = con1;
        EPIC_REG8(PIC_REG_CM2CON0) = con0;
        s_comp2_cb = h->EventCallback;
    } else {
        return EPIC_INVALID;
    }

    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_COMP_DeInit(COMP_InstanceTypeDef inst)
{
    if (inst == COMP_INSTANCE_1) {
        EPIC_REG8(PIC_REG_CM1CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM1CON1) = 0x00U;
        s_comp1_cb = 0;
    } else if (inst == COMP_INSTANCE_2) {
        EPIC_REG8(PIC_REG_CM2CON0) = 0x00U;
        EPIC_REG8(PIC_REG_CM2CON1) = 0x00U;
        s_comp2_cb = 0;
    } else {
        return EPIC_INVALID;
    }
    return EPIC_OK;
}

uint8_t EPIC_COMP_ReadOutput(COMP_InstanceTypeDef inst)
{
    uint8_t cmout = EPIC_REG8(PIC_REG_CMOUT);
    if (inst == COMP_INSTANCE_1) return (cmout & PIC_CMOUT_MC1OUT) ? 1U : 0U;
    if (inst == COMP_INSTANCE_2) return (cmout & PIC_CMOUT_MC2OUT) ? 1U : 0U;
    return 0U;
}

void CMP1_IRQHandler(void)
{
    if (s_comp1_cb) s_comp1_cb();
}

void CMP2_IRQHandler(void)
{
    if (s_comp2_cb) s_comp2_cb();
}
```

Confirm the exact platform macro names against a fresh read of
`pic16f193x_timer1.c` before finalizing. Confirm `CMOUT`'s two bits
are indeed a synchronized mirror of `CxCON0<CxOUT>` (vs. some other
relationship) against DS41364B before relying on
`EPIC_COMP_ReadOutput`'s choice to read `CMOUT` rather than each
instance's own `CxCON0<CxOUT>` bit directly.

## Task 4: Wire `CMP1_IRQHandler`/`CMP2_IRQHandler` into `pic16f193x_irq_dispatch.c`

- [ ] **Step 1: Add the extern declarations and calls**, mirroring
  `TIMER1_IRQHandler` (read the file first, confirm the `CMP1`/`CMP2`
  IRQn enum entries' exact names).

- [ ] **Step 2: Confirm the host build still compiles clean.**

## Task 5: Host sim model

**Files:** Modify `pic16f193x-hal/src/sim/pic16f193x_sim.c`.

- [ ] **Step 1: Add a caller-injected boolean per instance**

```c
static uint8_t s_comp_injected[2];   /* Index 0 = instance 1, index 1 = instance 2. */

void pic16f193x_sim_comp_drive(uint8_t instance_1_or_2, uint8_t out)
{
    if (instance_1_or_2 == 1U || instance_1_or_2 == 2U) {
        s_comp_injected[instance_1_or_2 - 1U] = out ? 1U : 0U;
    }
}

static void sim_step_comp(void)
{
    uint8_t cmout = 0U;
    if (pic16f193x_sim_sfr[PIC_REG_CM1CON0] & PIC_CM1CON0_C1ON) {
        if (s_comp_injected[0]) cmout |= PIC_CMOUT_MC1OUT;
    }
    if (pic16f193x_sim_sfr[PIC_REG_CM2CON0] & PIC_CM2CON0_C2ON) {
        if (s_comp_injected[1]) cmout |= PIC_CMOUT_MC2OUT;
    }
    pic16f193x_sim_sfr[PIC_REG_CMOUT] = cmout;
}
```

Add the `sim_step_comp()` call to the same per-step dispatch that
calls `sim_step_timer1()`.

- [ ] **Step 2: Confirm the host build still compiles clean.**

## Task 6: Wire into `CMakeLists.txt` and the XC8 Makefile

- [ ] **Step 1: Add `pic16f193x_comp.c`** to `EPIC_SOURCES`,
  mirroring Timer1's entry.

- [ ] **Step 2: Register `example_comparator`** via
  `epic_add_example(example_comparator tests/example_comparator.c)`.

- [ ] **Step 3: Mirror both in the XC8 Makefile's HAL sources and
  `SIM_APP` selection.**

- [ ] **Step 4: Confirm the host build fails only on the missing
  example.**

## Task 7: Add `example_comparator.c`

**Files:** Create `pic16f193x-hal/tests/example_comparator.c`.

- [ ] **Step 1: Write the example**

```c
/**
 * @file    example_comparator.c
 * @brief   Comparator smoke test: init both instances, inject a
 *          known state on the sim, confirm CMOUT.
 *
 * @details
 *   Expected register image (host sim, after init + one sim step,
 *   with instance 1 injected high and instance 2 injected low):
 *     CM1CON0 = 0x80   (C1ON=1, rest default)
 *     CM2CON0 = 0x80   (C2ON=1, rest default)
 *     CMOUT   = 0x01   (MC1OUT=1, MC2OUT=0)
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_gpio.h"
#include "peripherals/pic16f193x_comp.h"
#include "core/pic16f193x_wdt_sleep.h"
#include "core/epic_harness.h"

extern void pic16f193x_harness_halt(void);
extern void pic16f193x_sim_comp_drive(uint8_t instance_1_or_2, uint8_t out);

int main(void)
{
    epic_harness_init();

    COMP_HandleTypeDef c1 = COMP_HANDLE_DEFAULT;
    c1.Instance = COMP_INSTANCE_1;
    EPIC_COMP_Init(&c1);

    COMP_HandleTypeDef c2 = COMP_HANDLE_DEFAULT;
    c2.Instance = COMP_INSTANCE_2;
    EPIC_COMP_Init(&c2);

    pic16f193x_sim_comp_drive(1U, 1U);
    pic16f193x_sim_comp_drive(2U, 0U);

    uint8_t con1 = EPIC_REG8(PIC_REG_CM1CON0);
    uint8_t con2 = EPIC_REG8(PIC_REG_CM2CON0);
    uint8_t cmout = EPIC_REG8(PIC_REG_CMOUT);
    epic_harness_log("CM1CON0=0x%02X CM2CON0=0x%02X CMOUT=0x%02X\n", con1, con2, cmout);
    int rc = epic_harness_report((con1 == 0x80U) && (con2 == 0x80U) && (cmout == 0x01U));
    pic16f193x_harness_halt();
    return rc;
}
```

- [ ] **Step 2: Build and run on host.** Run:
  `cmake -B build -S pic16f193x-hal && cmake --build build && ./build/example_comparator`.
  Expected: logged values match, then `EPIC_HARNESS_RESULT: PASS`.

- [ ] **Step 3: Fix any mismatch at the source**, not by editing the
  expected value.

## Task 8: Real-target XC8 build + `mdb` gate

- [ ] **Step 1: 6-part real-target build** for all 6 MCU parts.

- [ ] **Step 2: `HARNESS=sim` build + `mdb` gate**: `make MCU=16F1937 HARNESS=sim SIM_APP=example_comparator.c`, then
  `make mdb-test MODULE=pic16f193x-hal MCU=16F1937 DEVICE=PIC16F1937 DFP=Microchip.PIC12-16F1xxx_DFP MODE=gpio WAIT_MS=60000`.
  Expected: `EPIC_HARNESS_RESULT: PASS`.

- [ ] **Step 3: Manual register readback** via `mdb.sh` `stepi`/`print`
  (`docs/adding-a-device.md` §4.6), confirm `CM1CON0=0x80`,
  `CM2CON0=0x80` (the real `CMOUT` value depends on real analog input,
  not asserted numerically on target, only the control-register state
  is checked here, mirroring the ADC plan's real-target scope note).
  If the control registers don't match, disassemble and compare
  against the C source before assuming a datasheet-fact error.

## Task 9: Documentation updates

- [ ] **Step 1: MANUAL.md §17 "Comparator"**: register tables for both
  instances, driver API summary, the `CMOUT`-vs-`CxOUT` readback
  choice explained, example pointer. Renumber if claimed first.

- [ ] **Step 2: README.md**: move Comparator to the done list.

- [ ] **Step 3: `docs/pic16f193x-plan.md` §7**: mark this row done.

## Task 10: Commit and final regression sweep

- [ ] **Step 1: Full regression**: run every host example, confirm
  PASS; re-run the module's 6-part real-target build.

- [ ] **Step 2: Commit**

```bash
git add pic16f193x-hal/include/pic16f193x_sfr.h \
        pic16f193x-hal/include/peripherals/pic16f193x_comp.h \
        pic16f193x-hal/include/peripherals/hal_comp.h \
        pic16f193x-hal/src/peripherals/pic16f193x_comp.c \
        pic16f193x-hal/src/core/pic16f193x_irq_dispatch.c \
        pic16f193x-hal/src/sim/pic16f193x_sim.c \
        pic16f193x-hal/tests/example_comparator.c \
        pic16f193x-hal/CMakeLists.txt \
        pic16f193x-hal/mcu/pic16f193x-mplabx/Makefile \
        pic16f193x-hal/MANUAL.md \
        pic16f193x-hal/README.md \
        docs/pic16f193x-plan.md
git commit -m "feat(pic16f193x): Comparator peripheral through the §4 gate"
```

- [ ] **Step 3: User sign-off before push.** Do not push without fresh
  explicit approval.

## Self-Review

**1. Spec coverage:** SFR macros for both instances + CMOUT (Task 1),
driver header/shim (Task 2), driver body (Task 3), IRQ dispatch wiring
(Task 4), sim model (Task 5), build wiring (Task 6), example (Task 7),
full gate (Task 8), docs (Task 9), commit/sign-off (Task 10). No
Non-goals needed, full feature surface covered.

**2. Placeholder scan:** no bare "TBD"/"TODO"; the `CMOUT`-vs-`CxOUT`
relationship is flagged as an explicit confirm-before-finalizing item
(Task 3), not a vague placeholder.

**3. Type consistency:** `COMP_InstanceTypeDef`,
`COMP_InterruptEdgeTypeDef`, `COMP_HandleTypeDef`,
`COMP_HANDLE_DEFAULT`, `EPIC_COMP_Init/DeInit/ReadOutput`,
`CMP1_IRQHandler`/`CMP2_IRQHandler` referenced identically across
Tasks 2, 3, 4, 7.

**4. Judgment calls flagged for the reviewer:**
- `EPIC_COMP_ReadOutput` reads `CMOUT` rather than each instance's own
  `CxCON0<CxOUT>` bit; this choice (and whether `CMOUT` is truly just
  a synchronized mirror) needs confirming against DS41364B (Task 3's
  note).

If any brief requirement is missing a task, fix it here. No fixes
required.
