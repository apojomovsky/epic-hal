/* 16F506 comparator-pair probe (DS41268D §7.0, Register 7-2/7-3;
 * reference §8.0, Register 8-1). Programs both comparators plus the
 * voltage reference, then proves each output bit follows its inputs.
 * 16F506-only: the sole part with the analog block, so CMake and the
 * manifest build this file for 16F506 alone.
 *
 * Host: POR image, GPIO-cleared image, programmed images (OUT bits
 * masked, they are read-only), driven pin levels vs CxOUT under both
 * polarities, DeInit. Target (1024-word budget: C1 stimulus only,
 * C2 and DeInit are image-level): POR and programmed images, C1
 * output following RB0 high/low, both readings flipped by clearing
 * C1POL, then a Timer0-polled RB0 blink on pass (the MODE=toggle
 * gate watches it) or a dark halt on fail. Target stimulus drives
 * RB0 as an output against the 0.6V reference (C1NREF=0 from the
 * clear), with a settle delay per §7.3.
 *
 * Hand-computed images, from the GPIO-cleared block (all 0x00):
 *   host phase 1 {C1 on, inverted; C2 off; Vref tap 0xA low range}:
 *     CM1CON0 = 0x08, CM2CON0 = 0x20, VRCON = 0xAA
 *   host pin mux (C1PREF|C1NREF, C2PREF1|C2NREF): 0x0E, 0x26
 *   host phase 2 {both on, non-inverted}: 0x2E, 0x2E
 *   target {both on, non-inverted; Vref tap 0xA low; C1PREF}:
 *     CM1CON0 = 0x2A, CM2CON0 = 0x28, VRCON = 0xAA
 *   DeInit: C1ON/C2ON clear, VRCON = 0x00, outputs read 0.
 */

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"
#include "peripherals/pic16f5x_comp.h"
#include "core/epic_harness.h"
#ifdef PIC16F5X_HOST
#include "pic16f5x_sim.h"
#endif

/** Mask hiding the read-only output bit for image compares. */
#define COMP_OUT_MASK 0x7FU

/* No gate-record globals: RAM is the binding budget (67 B). The
 * verdict rides the RB0 toggle; post-halt CM1CON0/CM2CON0/VRCON
 * prints show the DeInit state (ON/VREN clear). */

#ifdef PIC16F5X_HOST
/* POR bytes for the host image checks. */
uint8_t g_comp_por_cm1;
uint8_t g_comp_por_cm2;
uint8_t g_comp_por_ad;
uint8_t g_comp_por_vr;

static uint8_t g_fail = 0U;

/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index, logged as F characters.
 */
static void fail(uint8_t idx)
{
    uint8_t i = 0U;
    g_fail++;
    for (i = 0U; i <= idx; i++)
    {
        epic_harness_log("F");
    }
    epic_harness_log("\n");
}
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)
#endif

#ifndef PIC16F5X_HOST
/**
 * @brief Spin ~200 us for the comparator to settle (§7.3).
 */
static void comp_settle(void)
{
    volatile uint8_t d = 200U;
    while (d != 0U)
    {
        d--;
    }
}
#endif

#ifdef PIC16F5X_HOST
/**
 * @brief Capture the analog-block POR bytes before any HAL call.
 */
static void comp_capture_por(void)
{
    g_comp_por_cm1 = EPIC_REG8(PIC_REG_CM1CON0);
    g_comp_por_cm2 = EPIC_REG8(PIC_REG_CM2CON0);
    g_comp_por_ad  = EPIC_REG8(PIC_REG_ADCON0);
    g_comp_por_vr  = EPIC_REG8(PIC_REG_VRCON);
}
#endif

/**
 * @brief Comparator-pair probe: register images, driven outputs,
 *        polarity both ways, DeInit.
 */
int main(void)
{
#ifdef PIC16F5X_HOST
    COMP_HandleTypeDef h = COMP_HANDLE_DEFAULT;

    epic_harness_init(1000U);
    comp_capture_por();

    /* POR image: comparators on, ADC selects set, reference off
     * (VRCON 0x00 is the EDC + MPLAB SIM value, see sfr header). */
    CHECK((g_comp_por_cm1 & COMP_OUT_MASK) == 0x7FU, 0x00U);
    CHECK((g_comp_por_cm2 & COMP_OUT_MASK) == 0x7FU, 0x01U);
    CHECK(g_comp_por_ad == 0xFCU, 0x02U);
    CHECK(g_comp_por_vr == 0x00U, 0x03U);

    /* Stimulus pins as inputs; clears the analog block to 0x00. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_INPUT);
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_0 | GPIO_PIN_1, GPIO_MODE_INPUT);
    CHECK(EPIC_REG8(PIC_REG_CM1CON0) == 0x00U, 0x04U);
    CHECK(EPIC_REG8(PIC_REG_CM2CON0) == 0x00U, 0x05U);

    /* Phase 1: C1 on inverted, C2 off, Vref tap 0xA low range. */
    h.C1Inverted = true;
    h.VrefEnabled = true;
    h.VrefRange = true;
    h.VrefTap = 0x0AU;
    CHECK(EPIC_COMP_Init(&h) == EPIC_OK, 0x06U);
    CHECK((EPIC_REG8(PIC_REG_CM1CON0) & COMP_OUT_MASK) == 0x08U, 0x07U);
    CHECK((EPIC_REG8(PIC_REG_CM2CON0) & COMP_OUT_MASK) == 0x20U, 0x08U);
    CHECK(EPIC_REG8(PIC_REG_VRCON) == 0xAAU, 0x09U);
    CHECK(EPIC_COMP_C2Out() == 0U, 0x0AU);

    /* Pin mux (the POR select; GPIO init cleared it). */
    EPIC_REG8(PIC_REG_CM1CON0) |= (uint8_t)(PIC_CM1CON0_C1PREF | PIC_CM1CON0_C1NREF);
    EPIC_REG8(PIC_REG_CM2CON0) |= (uint8_t)(PIC_CM2CON0_C2PREF1 | PIC_CM2CON0_C2NREF);
    CHECK((EPIC_REG8(PIC_REG_CM1CON0) & COMP_OUT_MASK) == 0x0EU, 0x0BU);
    CHECK((EPIC_REG8(PIC_REG_CM2CON0) & COMP_OUT_MASK) == 0x26U, 0x0CU);

    /* C1 inverted: output is NOT the raw comparison. */
    pic16f5x_sim_drive_input('B', 0U, 1U);
    pic16f5x_sim_drive_input('B', 1U, 0U);
    CHECK(EPIC_COMP_C1Out() == 0U, 0x0DU);
    pic16f5x_sim_drive_input('B', 0U, 0U);
    pic16f5x_sim_drive_input('B', 1U, 1U);
    CHECK(EPIC_COMP_C1Out() == 1U, 0x0EU);

    /* Phase 2: both on, non-inverted (RMW sets POL/ON bits). */
    h.C1Inverted = false;
    h.C2Enabled = true;
    CHECK(EPIC_COMP_Init(&h) == EPIC_OK, 0x0FU);
    CHECK((EPIC_REG8(PIC_REG_CM1CON0) & COMP_OUT_MASK) == 0x2EU, 0x10U);
    CHECK((EPIC_REG8(PIC_REG_CM2CON0) & COMP_OUT_MASK) == 0x2EU, 0x11U);

    /* C1 non-inverted follows the driven levels. */
    pic16f5x_sim_drive_input('B', 0U, 1U);
    pic16f5x_sim_drive_input('B', 1U, 0U);
    CHECK(EPIC_COMP_C1Out() == 1U, 0x12U);
    pic16f5x_sim_drive_input('B', 0U, 0U);
    pic16f5x_sim_drive_input('B', 1U, 1U);
    CHECK(EPIC_COMP_C1Out() == 0U, 0x13U);

    /* C2 on RC0/RC1 follows the driven levels. */
    pic16f5x_sim_drive_input('C', 0U, 1U);
    pic16f5x_sim_drive_input('C', 1U, 0U);
    CHECK(EPIC_COMP_C2Out() == 1U, 0x14U);
    pic16f5x_sim_drive_input('C', 0U, 0U);
    CHECK(EPIC_COMP_C2Out() == 0U, 0x15U);

    /* DeInit returns the block to analog-off, outputs read 0. */
    CHECK(EPIC_COMP_DeInit() == EPIC_OK, 0x16U);
    CHECK((EPIC_REG8(PIC_REG_CM1CON0) & PIC_CM1CON0_C1ON) == 0U, 0x17U);
    CHECK((EPIC_REG8(PIC_REG_CM2CON0) & PIC_CM2CON0_C2ON) == 0U, 0x18U);
    CHECK(EPIC_REG8(PIC_REG_VRCON) == 0x00U, 0x19U);
    CHECK(EPIC_COMP_C1Out() == 0U, 0x1AU);
    CHECK(EPIC_COMP_C2Out() == 0U, 0x1BU);

    (void)epic_harness_report(g_fail == 0U);
    return g_fail == 0U ? 0 : 1;
#else
    uint8_t ok = 1U;
    uint8_t last_t0 = 0U;
    TIMER0_HandleTypeDef h;
    COMP_HandleTypeDef c;

    /* Field-by-field handle setup (the example_blink shape): no
     * bulk initializer copy, every byte is an explicit store. */
    c.C1Enabled = true;
    c.C1Inverted = false;
    c.C2Enabled = false;
    c.C2Inverted = false;
    c.VrefEnabled = false;
    c.VrefOutput = false;
    c.VrefRange = false;
    c.VrefTap = 0x00U;

    /* POR image, read before any HAL call clears the block. */
    if ((EPIC_REG8(PIC_REG_CM1CON0) & COMP_OUT_MASK) != 0x7FU) ok = 0U;
    if ((EPIC_REG8(PIC_REG_CM2CON0) & COMP_OUT_MASK) != 0x7FU) ok = 0U;
    if (EPIC_REG8(PIC_REG_ADCON0) != 0xFCU) ok = 0U;
    if (EPIC_REG8(PIC_REG_VRCON) != 0x00U) ok = 0U;

    /* Stimulus + blink pin as output; clears the analog block. C2
     * keeps its cleared mux and gets image-level proof only (the
     * host proves its driven behavior); the 1024-word budget fits
     * the C1 stimulus round only. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);

    /* Timer0 for the pass blink (example_blink shape). */
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.ClockEdge         = TIMER0_EDGE_RISING;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    EPIC_TIMER0_Init(&h);
    EPIC_TIMER0_Start(&h);
    last_t0 = EPIC_TIMER0_ReadCounter();

    /* Both on non-inverted, Vref tap 0xA low range (cleared base:
     * CM1CON0 = 0x28, CM2CON0 = 0x28, VRCON = 0xAA). */
    c.C2Enabled = true;
    c.VrefEnabled = true;
    c.VrefRange = true;
    c.VrefTap = 0x0AU;
    if (EPIC_COMP_Init(&c) != EPIC_OK) ok = 0U;
    /* C1+ is RB0 against the 0.6V reference (C1NREF stays 0 from
     * the clear, Register 7-2). */
    EPIC_REG8(PIC_REG_CM1CON0) |= PIC_CM1CON0_C1PREF;
    if ((EPIC_REG8(PIC_REG_CM1CON0) & COMP_OUT_MASK) != 0x2AU) ok = 0U;
    if ((EPIC_REG8(PIC_REG_CM2CON0) & COMP_OUT_MASK) != 0x28U) ok = 0U;
    if (EPIC_REG8(PIC_REG_VRCON) != 0xAAU) ok = 0U;

    /* C1 output follows the driven pin (settle per §7.3). */
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    comp_settle();
    if (EPIC_COMP_C1Out() != 1U) ok = 0U;
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    comp_settle();
    if (EPIC_COMP_C1Out() != 0U) ok = 0U;

    /* Clearing C1POL inverts both readings (polarity path on
     * silicon; the driver set-path is proven by the 0x2A image). */
    EPIC_REG8(PIC_REG_CM1CON0) &= (uint8_t)~PIC_CM1CON0_C1POL;
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    comp_settle();
    if (EPIC_COMP_C1Out() != 0U) ok = 0U;
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    comp_settle();
    if (EPIC_COMP_C1Out() != 1U) ok = 0U;

    /* DeInit returns the block to analog-off (host-proven asserts;
     * the post-halt CM readback shows ON clear). */
    if (EPIC_COMP_DeInit() != EPIC_OK) ok = 0U;

    if (ok == 0U)
    {
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        for (;;)
        {
        }
    }
    for (;;)
    {
        uint8_t t0 = EPIC_TIMER0_ReadCounter();
        if (t0 < last_t0)
        {
            EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        last_t0 = t0;
    }
#endif
}
