/**
 * CLC quadrature-decoder-assist probe: 2x J-K direction cells, TMR1
 * position counter, 10-LED bar graph. Host-only: A/B arrive through the Dx
 * hook; CW edges reach TMR1 through the T1CKI pulse hook (the board wire
 * from LC1OUT on silicon), CCW edges decrement through the driver (this
 * part has no down-counter). LEDs RC0..RC7, RA4, RA5 follow the Q40 layout.
 */
#include <stddef.h>
#include "pic16f1508.h"
#include "peripherals/pic16f1508_clc.h"
#include "peripherals/pic16f1508_clc_src.h"
#include "peripherals/pic16f1508_clc_gls.h"
#include "peripherals/pic16f1508_gpio.h"
#include "peripherals/pic16f1508_timer1.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

/** @brief Count one clockwise step on TMR1, saturating at 100. */
static void on_cw(void)
{
    if (EPIC_TIMER1_ReadCounter() < 100U)
    {
        pic16f1508_sim_timer1_external_pulse();
    }
}

/** @brief Count one counter-clockwise step on TMR1, saturating at 0. */
static void on_ccw(void)
{
    uint16_t v = EPIC_TIMER1_ReadCounter();

    if (v > 0U)
    {
        EPIC_TIMER1_WriteCounter((uint16_t)(v - 1U));
    }
}

/**
 * @brief Drive one A/B sample into both direction cells, then step once.
 * @param a Encoder A level (CLCIN0, D1 of both cells).
 * @param b Encoder B level (CLCIN1, D4 of both cells).
 */
static void drive_and_step(uint8_t a, uint8_t b)
{
    pic16f1508_sim_drive_clc_input(0U, 1U, a);
    pic16f1508_sim_drive_clc_input(0U, 4U, b);
    pic16f1508_sim_drive_clc_input(1U, 1U, a);
    pic16f1508_sim_drive_clc_input(1U, 4U, b);
    epic_harness_tick();
}

/**
 * @brief Read back the clockwise (CLC1) cell output.
 * @return 0 or 1.
 */
static uint8_t read_cw(void)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(CLC1, &level);
    return level;
}

/**
 * @brief Read back the counter-clockwise (CLC2) cell output.
 * @return 0 or 1.
 */
static uint8_t read_ccw(void)
{
    uint8_t level = 0U;

    EPIC_CLC_ReadOutput(CLC2, &level);
    return level;
}

/** @brief Repaint the 10-LED bar graph from the TMR1 position. */
static void show_bar(void)
{
    uint8_t n = (uint8_t)(EPIC_TIMER1_ReadCounter() / 10U);
    uint8_t c = 0U;
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        if (i < n)
        {
            c |= (uint8_t)(1U << i);
        }
    }
    EPIC_GPIO_WritePort(GPIOC, c);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_4, (n > 8U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    EPIC_GPIO_WritePin(GPIOA, GPIO_PIN_5, (n > 9U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Run one clockwise Gray cycle; the CW cell must pulse, CCW stay low.
 */
static void cycle_cw(void)
{
    drive_and_step(1U, 0U);
    drive_and_step(1U, 1U);
    drive_and_step(0U, 1U);
    drive_and_step(0U, 0U);
}

/**
 * @brief Run one counter-clockwise Gray cycle; CCW pulses, CW stays low.
 */
static void cycle_ccw(void)
{
    drive_and_step(0U, 1U);
    drive_and_step(1U, 1U);
    drive_and_step(1U, 0U);
    drive_and_step(0U, 0U);
}

/**
 * @brief Quadrature checks: images, direction sense, noise rejection, bar.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    CLC_HandleTypeDef cw;
    CLC_HandleTypeDef ccw;
    TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
    unsigned int i;

    epic_harness_init(10000UL);

    /* Position counter: T1CKI counter mode, 1:1, parked at mid-scale. */
    t1.ClockSource = TIMER1_CLOCK_EXTERNAL;
    t1.ReloadValue = 50U;
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    ok &= (EPIC_TIMER1_ReadCounter() == 50U);

    cw.Instance = CLC1;
    cw.Mode = 6U;
    cw.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_CLCIN0);
    cw.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_FOSC);
    cw.Source[2] = EPIC_CLC_SourceCode(3U, CLC_IN_LC1_OUT);
    cw.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN1);
    cw.Gls[0] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    cw.Gls[1] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    cw.Gls[2] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    cw.Gls[3] = 0U;
    cw.GatePolarity = 0x04U;
    cw.OutputPolarity = 0U;
    cw.OutputEnable = 0U;
    cw.InterruptRising = 1U;
    cw.InterruptFalling = 0U;
    cw.Callback = on_cw;
    ok &= (EPIC_CLC_Init(&cw) == EPIC_OK);

    /* CW cell: clock on B (D4T), J on A (D1T), R on NOT A (D1T plus G3POL). */
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC1SEL1) == 0x50U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS0) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS1) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS2) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC1GLS3) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC1POL) == 0x04U);
    ok &= (EPIC_REG8(PIC_REG_CLC1CON) == 0x96U);

    ccw.Instance = CLC2;
    ccw.Mode = 6U;
    ccw.Source[0] = EPIC_CLC_SourceCode(1U, CLC_IN_CLCIN0);
    ccw.Source[1] = EPIC_CLC_SourceCode(2U, CLC_IN_FOSC);
    ccw.Source[2] = EPIC_CLC_SourceCode(3U, CLC_IN_LC1_OUT);
    ccw.Source[3] = EPIC_CLC_SourceCode(4U, CLC_IN_CLCIN1);
    ccw.Gls[0] = EPIC_CLC_GLSInput(1U, 0U, 1U);
    ccw.Gls[1] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    ccw.Gls[2] = EPIC_CLC_GLSInput(4U, 0U, 1U);
    ccw.Gls[3] = 0U;
    ccw.GatePolarity = 0x04U;
    ccw.OutputPolarity = 0U;
    ccw.OutputEnable = 0U;
    ccw.InterruptRising = 1U;
    ccw.InterruptFalling = 0U;
    ccw.Callback = on_ccw;
    ok &= (EPIC_CLC_Init(&ccw) == EPIC_OK);

    /* CCW cell: clock on A (D1T), J on B (D4T), R on NOT B (D4T plus G3POL). */
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL0) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC2SEL1) == 0x50U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS0) == 0x02U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS1) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS2) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_CLC2GLS3) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CLC2POL) == 0x04U);
    ok &= (EPIC_REG8(PIC_REG_CLC2CON) == 0x96U);

    EPIC_GPIO_Init(GPIOC, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(GPIOA, (uint8_t)(GPIO_PIN_4 | GPIO_PIN_5), GPIO_MODE_OUTPUT);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC1);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC2);
    EPIC_REG8(PIC_REG_INTCON) |= PIC_INTCON_GIE;

    /* Idle: both cells low, counter parked. */
    drive_and_step(0U, 0U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 50U);

    /* Clockwise: only the CW cell pulses, once per Gray cycle. */
    cycle_cw();
    ok &= (EPIC_TIMER1_ReadCounter() == 51U);
    drive_and_step(1U, 0U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 51U);
    drive_and_step(1U, 1U);
    ok &= (read_cw() == 1U);
    ok &= (read_ccw() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 52U);
    drive_and_step(0U, 0U);
    cycle_cw();
    ok &= (EPIC_TIMER1_ReadCounter() == 53U);

    /* Climb to 80: the 8 RC LEDs light, RA4/RA5 stay off. */
    for (i = 0U; i < 27U; i++)
    {
        cycle_cw();
    }
    ok &= (EPIC_TIMER1_ReadCounter() == 80U);
    show_bar();
    ok &= (EPIC_REG8(PIC_REG_LATC) == 0xFFU);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & GPIO_PIN_4) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & GPIO_PIN_5) == 0U);

    /* Counter-clockwise: only the CCW cell pulses, counter walks back. */
    cycle_ccw();
    ok &= (EPIC_TIMER1_ReadCounter() == 79U);
    drive_and_step(0U, 1U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 79U);
    drive_and_step(1U, 1U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 78U);
    drive_and_step(0U, 0U);
    cycle_ccw();
    ok &= (EPIC_TIMER1_ReadCounter() == 77U);
    for (i = 0U; i < 27U; i++)
    {
        cycle_ccw();
    }
    ok &= (EPIC_TIMER1_ReadCounter() == 50U);
    show_bar();
    ok &= (EPIC_REG8(PIC_REG_LATC) == 0x1FU);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & (uint8_t)(GPIO_PIN_4 | GPIO_PIN_5)) == 0U);

    /* Simultaneous A/B step fires both cells: the counts cancel to no move. */
    drive_and_step(1U, 1U);
    ok &= (read_cw() == 1U);
    ok &= (read_ccw() == 1U);
    drive_and_step(0U, 0U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() == 50U);

    /* Ends saturate instead of wrapping. */
    EPIC_TIMER1_WriteCounter(99U);
    cycle_cw();
    ok &= (EPIC_TIMER1_ReadCounter() == 100U);
    cycle_cw();
    ok &= (EPIC_TIMER1_ReadCounter() == 100U);
    EPIC_TIMER1_WriteCounter(1U);
    cycle_ccw();
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    cycle_ccw();
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);

    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC1);
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC2);

    /* Disabled cells read 0 even with both encoder lines driven. */
    ok &= (EPIC_CLC_DeInit(CLC1) == EPIC_OK);
    ok &= (EPIC_CLC_DeInit(CLC2) == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    drive_and_step(1U, 1U);
    ok &= (read_cw() == 0U);
    ok &= (read_ccw() == 0U);

    return epic_harness_report(ok);
}
