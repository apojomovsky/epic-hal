/**
 * GPIO and interrupt-on-change host-sim probe. Host-only: drives the
 * simulator (pic16f1508_sim_*), so it is not in the XC8 build. Covers the
 * PORTA 6-pin width, RB4 output, RB5 input, and an RB6 rising edge through
 * the registered callback. RB4..RB7 is the only IOC-capable range.
 */
#include "pic16f1508.h"
#include "peripherals/pic16f1508_gpio.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "pic16f1508_sim.h"

static volatile uint8_t g_ioc_seen = 0;
static volatile uint8_t g_ioc_iocbf = 0;

/**
 * @brief PORTB interrupt-on-change callback: records the captured IOCBF mask.
 * @param iocbf Captured IOCBF bits.
 * @param portb PORTB value read with the flags.
 */
static void on_ioc(uint8_t iocbf, uint8_t portb)
{
    (void)portb;
    g_ioc_seen = 1U;
    g_ioc_iocbf = iocbf;
}

/**
 * @brief GPIO and IOC smoke test on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    /* PORTA is 6 pins: a full-port write must not reach RA6/RA7. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOA, 0xFFU);
    epic_harness_tick(); /* sim refreshes PORTx per step; reads are not live */
    ok &= (EPIC_GPIO_ReadPort(GPIOA) == PIC16F1508_FAMILY_PORTA_MASK);

    /* RB4 output: the simulated pin follows the latch. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    ok &= (pic16f1508_sim_read_output('B', 4) == 1U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    ok &= (pic16f1508_sim_read_output('B', 4) == 0U);

    /* RB5 input: ReadPin follows the driven level. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    pic16f1508_sim_drive_input('B', 5, 1);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET);
    pic16f1508_sim_drive_input('B', 5, 0);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_RESET);

    /* IOC on RB6: asking for RB0..RB7 must leave only RB4..RB7 armed. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_6, GPIO_MODE_INPUT);
    EPIC_GPIO_RegisterChangeCallback(on_ioc);
    EPIC_GPIO_EnableChangeDetect(GPIO_PIN_All, 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == PIC16F1508_FAMILY_IOCB_MASK);
    EPIC_GPIO_EnableChangeDetect(GPIO_PIN_6, 0U);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_IOC);
    EPIC_IRQ_Restore(1U);

    pic16f1508_sim_drive_input('B', 6, 0);
    epic_harness_tick();
    ok &= (g_ioc_seen == 0U);
    pic16f1508_sim_drive_input('B', 6, 1);
    epic_harness_tick();
    ok &= (g_ioc_seen == 1U);
    ok &= (g_ioc_iocbf == GPIO_PIN_6);

    epic_harness_log("gpio/IOC smoke: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
