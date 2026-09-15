/* Host-only control-space probe for the PIC16F5x exemplar: exercises
 * the GPIO and Timer0 drivers against exact expected byte images.
 *
 * The TRIS/OPTION control registers are write-only on this core
 * (DS41213D Table 12-1): there is no file-register readback, and mdb
 * does not expose OPTION by name on the 16F54 (probed: `print OPTION`
 * -> "Symbol does not exist"; `print TRISA` works and returns 0x1F
 * at POR). So the register-image checks run on the host sim, where
 * the platform header routes control writes into the sim shadow
 * registers for readback; the real-target gate (MODE=toggle on the
 * blink) proves the behavioral side.
 *
 * Expected values (hand-computed):
 *   EPIC_GPIO_Init(GPIOB, PIN_0, OUTPUT)      -> TRISB = 0xFE
 *   EPIC_GPIO_WritePort(GPIOB, 0x55)          -> PORTB reads 0x55
 *   EPIC_TIMER0_Init (internal, 1:2, reload 0x21)
 *                                             -> OPTION = 0x00 (1:2 is PS=000,
 *                                                PSA=0, T0CS=0, T0SE=0)
 *                                             -> TMR0 reads 0x21
 *   EPIC_TIMER0_Start()                       -> T0CS stays clear
 *   6 sim cycles at 1:2 (3 steps)             -> TMR0 = 0x24 */

#include "pic16f5x_hal.h"
#include "pic16f5x_sfr.h"
#include "peripherals/pic16f5x_gpio.h"
#include "peripherals/pic16f5x_timer0.h"
#include "pic16f5x_sim.h"
#include "core/epic_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 4000000UL
#endif

static uint8_t g_fail = 0U;
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
 * @brief Run the control-space access probes and report pass/fail.
 */
int main(void)
{
    uint8_t t0 = 0U;
    TIMER0_HandleTypeDef h;

    epic_harness_init(1000U);

    /* GPIO direction via the control-space TRIS write; the host
     * platform header routes it into the sim shadow. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    t0 = pic16f5x_sim_trisb;
    CHECK(t0 == 0xFEU, 0x00U);

    /* PORTB latch write/readback through the file register. */
    EPIC_GPIO_WritePort(GPIOB, 0x55U);
    t0 = EPIC_REG8(PIC_REG_PORTB);
    CHECK(t0 == 0x55U, 0x02U);

    /* Timer0: internal, 1:2 prescaler, reload 0x21. */
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.ClockEdge         = TIMER0_EDGE_RISING;
    h.Prescaler         = TIMER0_PRESCALER_1_2;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x21U;
    EPIC_TIMER0_Init(&h);
    t0 = EPIC_REG8(PIC_REG_TMR0);
    CHECK(t0 == 0x21U, 0x04U);
    EPIC_TIMER0_Start(&h);

    /* OPTION shadow: PS=000 (1:2), PSA=0, T0CS=0, T0SE=0. */
    t0 = pic16f5x_sim_option;
    CHECK(t0 == 0x00U, 0x03U);
    t0 = pic16f5x_sim_option;
    CHECK((t0 & PIC_OPTION_T0CS) == 0U, 0x05U);

    /* Timer0 counts: the sim steps at 1:2 (TMR0 starts at 0x21).
     * After 6 cycles (3 steps) TMR0 = 0x24. */
    pic16f5x_sim_step(6U);
    t0 = EPIC_REG8(PIC_REG_TMR0);
    CHECK(t0 == 0x24U, 0x06U);

    /* Simulated 1:2 counting continues: 200 more cycles (100 steps,
     * 0x24 + 100 = 0x88), proving the prescaler keeps dividing. */
    pic16f5x_sim_step(200U);
    t0 = EPIC_REG8(PIC_REG_TMR0);
    CHECK(t0 == 0x88U, 0x07U);

    /* The 16F54's 25-byte GPR surface (DS41213D §1.0) means the
     * probe's loop counter is byte-width; re-run the wrap detection
     * idiom the blink uses (t0 < last) across 512 ticks at 1:2
     * (0x88 + 512 steps wraps twice: at +120 and +376), exactly the
     * toggle gate's expectation. */
    {
        uint16_t j = 0U;
        uint8_t last = pic16f5x_sim_sfr[PIC_REG_TMR0];
        uint8_t over = 0U;
        for (j = 0U; j < 512U; j++)
        {
            t0 = EPIC_REG8(PIC_REG_TMR0);
            if (t0 < last)
            {
                over++;
            }
            last = t0;
            pic16f5x_sim_step(2U);
        }
        CHECK(over >= 2U, 0x08U);
    }

    (void)epic_harness_report(g_fail == 0U);
    return g_fail == 0U ? 0 : 1;
}
