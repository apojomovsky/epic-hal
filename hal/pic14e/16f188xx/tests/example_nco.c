/**
 * NCO1 host-sim probe. Host-only: advances the NCO model through
 * epic_harness_tick, so it is not in the XC8 build (the mdb gate probe is
 * example_gpio_mdb.c). Covers handle validation, the Init register image,
 * the accumulator walk and overflow, FDC toggling, PFM pulse width,
 * polarity, the double-buffered increment, accumulator write refusal while
 * running, the overflow callback through the shared dispatcher, the PPS
 * output source and the DeInit POR state.
 *
 * Expected register image (FOSC, FDC, increment 0x40000 = 2^18, after Init):
 *   NCO1CON = 0x00, NCO1CLK = 0x00
 *   NCO1INCU = 0x04, NCO1INCH = 0x00, NCO1INCL = 0x00
 * Overflow period is 2^20 / 2^18 = 4 ticks, so after Start: ticks 1..3 add
 * 0x40000 each (ACCU 0x04, 0x08, 0x0C), tick 4 wraps to 0 with NCO1IF and
 * N1OUT = 1 (NCO1CON = 0xA0); tick 8 toggles N1OUT back (0x80).
 * PFM, FOSC, width 4 (N1PWS = 2), increment 0x10000: NCO1CON = 0x01,
 * NCO1CLK = 0x40, overflow every 16 ticks, output high for ticks 17..20.
 * HFINTOSC, PFM, width 128, inverted: NCO1CLK = 0xE1, NCO1CON = 0x11.
 */
#include <stddef.h>
#include "pic16f188xx.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_nco.h"
#include "peripherals/pic16f188xx_pps.h"

static unsigned int s_calls;

/**
 * @brief Overflow callback that records its call.
 */
static void nco_cb(void)
{
    s_calls++;
}

/**
 * @brief Step the simulator n instruction ticks.
 * @param n Tick count.
 */
static void step(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief NCO1 checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    NCO_HandleTypeDef h = NCO_HANDLE_DEFAULT;

    epic_harness_init(10000UL);
    EPIC_IRQ_Restore(1U);

    /* POR: everything clear except the increment low byte. */
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x01U);
    ok &= (EPIC_NCO_GetIncrement() == 1UL);
    ok &= (EPIC_NCO_ReadAccumulator() == 0UL);

    /* Validation: NULL, reserved or out-of-range fields and an oversized
     * increment are rejected before anything is written. */
    ok &= (EPIC_NCO_Init(NULL) == EPIC_INVALID);
    h.ClockSource = (NCO_ClockSourceTypeDef)6;
    ok &= (EPIC_NCO_Init(&h) == EPIC_INVALID);
    h.ClockSource = NCO_CLK_FOSC;
    h.Mode = (NCO_ModeTypeDef)2;
    ok &= (EPIC_NCO_Init(&h) == EPIC_INVALID);
    h.Mode = NCO_MODE_FDC;
    h.PulseWidth = (NCO_PulseWidthTypeDef)8;
    ok &= (EPIC_NCO_Init(&h) == EPIC_INVALID);
    h.PulseWidth = NCO_PULSE_1;
    h.Increment = NCO_VALUE_MAX + 1UL;
    ok &= (EPIC_NCO_Init(&h) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x01U);
    ok &= (EPIC_NCO_SetIncrement(NCO_VALUE_MAX + 1UL) == EPIC_INVALID);
    ok &= (EPIC_NCO_WriteAccumulator(NCO_VALUE_MAX + 1UL) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x01U);

    /* Init stages the image without enabling. */
    h.Increment = 0x40000UL;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCU) == 0x04U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x00U);
    ok &= (EPIC_NCO_GetIncrement() == 0x40000UL);
    step(8U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0UL);

    /* FDC walk: three adds, then the wrap with the flag and N1OUT high. */
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x80U);
    step(1U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCU) == 0x04U);
    step(2U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCU) == 0x0CU);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_NCO1) == 0U);
    ok &= (EPIC_NCO_ReadOutput() == 0U);
    step(1U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0UL);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_NCO1) == 1U);
    ok &= (EPIC_NCO_ReadOutput() == 1U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0xA0U);
    step(4U);
    ok &= (EPIC_NCO_ReadOutput() == 0U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x80U);

    /* The accumulator loads only while stopped; Stop drops the output. */
    ok &= (EPIC_NCO_WriteAccumulator(0x12345UL) == EPIC_BUSY);
    ok &= (EPIC_NCO_Stop() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x00U);
    ok &= (EPIC_NCO_WriteAccumulator(0x12345UL) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCU) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCH) == 0x23U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCL) == 0x45U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0x12345UL);

    /* Double-buffered increment: a write while running applies from the
     * second clock after the NCO1INCL write (1, then 0x10 twice). */
    EPIC_IRQ_ClearFlag(PIC16F188XX_IRQ_NCO1);
    ok &= (EPIC_NCO_WriteAccumulator(0UL) == EPIC_OK);
    ok &= (EPIC_NCO_SetIncrement(1UL) == EPIC_OK);
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    ok &= (EPIC_NCO_SetIncrement(0x10UL) == EPIC_OK);
    ok &= (EPIC_NCO_GetIncrement() == 0x10UL);
    step(1U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0x01UL);
    step(1U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0x11UL);
    step(1U);
    ok &= (EPIC_NCO_ReadAccumulator() == 0x21UL);
    ok &= (EPIC_NCO_Stop() == EPIC_OK);

    /* PFM: pulse of 4 ticks after each overflow, 16 ticks apart. */
    h.Mode = NCO_MODE_PFM;
    h.PulseWidth = NCO_PULSE_4;
    h.Increment = 0x10000UL;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0x40U);
    ok &= (EPIC_NCO_WriteAccumulator(0UL) == EPIC_OK);
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    step(16U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_NCO1) == 1U);
    ok &= (EPIC_NCO_ReadOutput() == 0U);
    step(1U);
    ok &= (EPIC_NCO_ReadOutput() == 1U);
    step(3U);
    ok &= (EPIC_NCO_ReadOutput() == 1U);
    step(1U);
    ok &= (EPIC_NCO_ReadOutput() == 0U);
    ok &= (EPIC_NCO_Stop() == EPIC_OK);

    /* Polarity inverts the live output: idle high in FDC. */
    h.Mode = NCO_MODE_FDC;
    h.Invert = 1U;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x10U);
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0xB0U);
    ok &= (EPIC_NCO_ReadOutput() == 1U);
    ok &= (EPIC_NCO_Stop() == EPIC_OK);

    /* HFINTOSC, PFM, width 128, inverted: image only. */
    h.ClockSource = NCO_CLK_HFINTOSC;
    h.Mode = NCO_MODE_PFM;
    h.PulseWidth = NCO_PULSE_128;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0xE1U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x11U);

    /* Overflow callback: Init arms NCO1IE and PEIE; increment 0x80000
     * overflows every 2 ticks and the dispatcher runs the slot each time. */
    h.ClockSource = NCO_CLK_FOSC;
    h.Mode = NCO_MODE_FDC;
    h.Invert = 0U;
    h.Increment = 0x80000UL;
    h.OverflowCallback = nco_cb;
    s_calls = 0U;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= (EPIC_NCO_WriteAccumulator(0UL) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE7) & PIC_PIE7_NCO1IE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    step(4U);
    ok &= (s_calls == 2U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F188XX_IRQ_NCO1) == 0U);

    /* PPS: RA2 takes the NCO1 output source (PORTA reaches it on every part). */
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_NCO1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 2U) == 0x19U);

    /* DeInit: POR image, interrupt off, no further callbacks. */
    ok &= (EPIC_NCO_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0x00U);
    ok &= (EPIC_NCO_GetIncrement() == 1UL);
    ok &= (EPIC_NCO_ReadAccumulator() == 0UL);
    ok &= ((EPIC_REG8(PIC_REG_PIE7) & PIC_PIE7_NCO1IE) == 0U);
    step(8U);
    ok &= (s_calls == 2U);

    epic_harness_log("nco: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
