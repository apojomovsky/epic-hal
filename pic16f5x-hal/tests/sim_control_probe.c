/* Control-space probe for the PIC16F5x exemplar: exercises the GPIO
 * and Timer0 drivers against exact expected byte images. Runs as a
 * HARNESS=sim target under MPLAB SIM (MODE=gpio, RA0 marker) and as a
 * host test.
 *
 * The TRIS/OPTION control registers are write-only on this core
 * (DS41213D Table 12-1): there is no file-register readback, and mdb
 * does not expose OPTION by name on the 16F54 (probed: `print OPTION`
 * -> "Symbol does not exist"; `print TRISA` works and returns 0x1F
 * at POR). So the shadow-register checks below are host-only
 * (guarded by PIC16F5X_HOST); the target build checks the
 * behaviorally observable state: PORT/PORTA latch round-trips and the
 * TMR0 reload value.
 *
 * Expected values (hand-computed):
 *   EPIC_GPIO_Init(GPIOB, PIN_0, OUTPUT)      -> TRISB = 0xFE
 *   EPIC_GPIO_Init(GPIOB, PIN_0, INPUT)       -> TRISB = 0xFF
 *   EPIC_GPIOWritePort(GPIOB, 0x55)           -> PORTB reads 0x55
 *   EPIC_TIMER0_Init (internal, 1:256)        -> OPTION = 0x07
 *   EPIC_TIMER0_Init ReloadValue = 0x21       -> TMR0 reads 0x21
 *   EPIC_TIMER0_Start()                       -> T0CS clears (0x06)
 *   host only: 300 sim cycles -> TMR0 0x22; 512 -> 0x23 (two
 *   prescaler periods at 1:256 from 0x21). */

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"
#include "core/epic_harness.h"

#ifdef PIC16F5X_HOST
#include "pic16f5x_sim.h"
#endif

/**
 * @brief Freeze-at-verdict hook, defined by the mdb harness (this probe
 *        only builds as the HARNESS=sim target, so the mdb harness is
 *        linked). No-op extern on the host build (pic16_harness_sim.c
 *        links instead); guarded so the host test still compiles.
 */
#ifdef PIC16F5X_HOST
#define pic16f5x_harness_halt() ((void)0)
#else
extern void pic16f5x_harness_halt(void);
#endif

#ifndef FOSC_HZ
#define FOSC_HZ 4000000UL
#endif

static uint16_t g_fail = 0u;
/**
 * @brief Bump the failure counter and log a marker line.
 * @param idx the check index (0x00..0x0F), logged as F characters.
 */
static void fail(uint8_t idx)
{
    uint8_t i = 0U;
    g_fail++;
    /* Raw-bytes channel (no printf): emit idx+1 F's so the failing
     * check is countable in the capture. */
    for (i = 0U; i <= idx; i++)
    {
        epic_harness_log("F");
    }
    epic_harness_log("\n");
}
#define CHECK(cond, idx) do {         \
    if (!(cond)) fail(idx);            \
} while (0)

/**
 * @brief Run the control-space access probes and report pass/fail on
 *        RA0 (target) / stdout (host).
 */
int main(void)
{
    uint8_t v = 0U;

    epic_harness_init(1000U);

    /* GPIO direction via the control-space TRIS write (host shadow
     * check; the target TRIS is write-only). */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
#ifdef PIC16F5X_HOST
    v = pic16f5x_sim_trisb;
    CHECK(v == 0xFEU, 0x00U);

    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_INPUT);
    v = pic16f5x_sim_trisb;
    CHECK(v == 0xFFU, 0x01U);
#endif

    /* PORTB latch write/readback through the file register (both
     * builds). */
    EPIC_GPIO_WritePort(GPIOB, 0x55U);
    v = EPIC_REG8(PIC_REG_PORTB);
    CHECK(v == 0x55U, 0x02U);

    /* Timer0 OPTION via EPIC_TIMER0_Init: internal, 1:256 assigned
     * (PS=111), T0CS=0, T0SE=0, PSA=0. Host shadow check; target
     * checks the reload value only. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x21U;
    EPIC_TIMER0_Init(&h);
#ifdef PIC16F5X_HOST
    v = pic16f5x_sim_option;
    CHECK(v == 0x07U, 0x03U);
#endif
    v = EPIC_REG8(PIC_REG_TMR0);
    CHECK(v == 0x21U, 0x04U);

    /* Start flips T0CS through the driver shadow (host check); the
     * target timer keeps counting (the blink example proves the
     * counting path under mdb). */
    EPIC_TIMER0_Start(&h);
#ifdef PIC16F5X_HOST
    v = pic16f5x_sim_option;
    CHECK((v & PIC_OPTION_T0CS) == 0U, 0x05U);

    /* Timer0 actually counts: the sim steps at 1:256 (TMR0 starts at
     * 0x21). After 300 cycles (one prescaler period of 256) TMR0 =
     * 0x22; after another 212 (512 total = two periods) TMR0 = 0x23. */
    pic16f5x_sim_step(300U);
    v = EPIC_REG8(PIC_REG_TMR0);
    CHECK(v == 0x22U, 0x06U);
    pic16f5x_sim_step(212U);   /* 512 total: exactly two prescaler periods */
    v = EPIC_REG8(PIC_REG_TMR0);
    CHECK(v == 0x23U, 0x07U);
#endif

    /* Report once, then halt (target): XC8 restarts main() on return,
     * and epic_harness_init() would drive RA0 low again, flickering
     * the marker across the mdb `print PORTA` readback window. One
     * verdict. */
    (void)epic_harness_report(g_fail == 0U);
    pic16f5x_harness_halt();
    return 0;
}
