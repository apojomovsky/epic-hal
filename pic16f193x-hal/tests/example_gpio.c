/**
 * @file    example_gpio.c
 * @brief   GPIO + interrupt-on-change host-sim smoke test.
 *
 * @details
 *   Host-only: drives the simulator directly (pic16f193x_sim_*), so it
 *   is not in the XC8 Makefile's APP_SOURCES. Exercises the
 *   LAT/ANSEL/TRIS write path, the PORTx read path, and the PORTB
 *   interrupt-on-change (IOCBP/IOCBN/IOCBF/IOCIF) plumbing.
 *
 *   Expected register image (host sim, after init):
 *     RB0 output, ANSELB<0>=0, TRISB<0>=0, LATB<0> follows writes
 *     RB1 input,  ANSELB<1>=0, TRISB<1>=1, PORTB<1> follows drive_input
 *     RB2 IOC positive-edge: IOCBP<2>=1, IOCIE=1, GIE=1
 *   A rising edge on RB2 sets IOCBF<2> + IOCIF, the IOC_IRQHandler fires
 *   the registered callback with iocbf=0x04, and the test passes.
 */

#include "pic16f193x.h"
#include "pic16f193x_sfr.h"
#include "peripherals/pic16f193x_gpio.h"
#include "core/pic16f193x_irq.h"
#include "core/epic_harness.h"
#include "pic16f193x_sim.h"

static volatile uint8_t g_ioc_seen = 0;
static volatile uint8_t g_ioc_iocbf = 0;

static void on_ioc(uint8_t iocbf, uint8_t portb)
{
    (void)portb;
    g_ioc_seen  = 1;
    g_ioc_iocbf = iocbf;
}

int main(void)
{
    /* Bound generous; the test is event-driven, not time-driven. */
    epic_harness_init(10000UL);

    int ok = 1;

    /* 1. RB0 output: write high, observe via the sim's external view. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    ok &= (pic16f193x_sim_read_output('B', 0) == 1U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    ok &= (pic16f193x_sim_read_output('B', 0) == 0U);

    /* 2. RB1 input: drive it and read back through EPIC_GPIO_ReadPin.
     *    The sim refreshes PORTB from the input override each tick. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_INPUT);
    pic16f193x_sim_drive_input('B', 1, 1);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET);
    pic16f193x_sim_drive_input('B', 1, 0);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);

    /* 3. PORTB interrupt-on-change: positive edge on RB2. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_2, GPIO_MODE_INPUT);
    EPIC_GPIO_RegisterChangeCallback(on_ioc);
    EPIC_GPIO_EnableChangeDetect(GPIO_PIN_2, 0);
    EPIC_IRQ_Enable(PIC16F193X_IRQ_IOC);
    EPIC_IRQ_Restore(1);

    /* Drive RB2 low first so the sim's last-known level is low, then a
     * rising edge. Tick once after each drive to let sim_step_ioc run. */
    pic16f193x_sim_drive_input('B', 2, 0);
    epic_harness_tick();          /* no edge yet: iocbf stays 0. */
    ok &= (g_ioc_seen == 0U);
    pic16f193x_sim_drive_input('B', 2, 1);
    epic_harness_tick();          /* rising edge on RB2 -> IOCIF -> handler. */
    ok &= (g_ioc_seen  == 1U);
    ok &= (g_ioc_iocbf == GPIO_PIN_2);

    epic_harness_log("gpio/IOC smoke: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
