/**
 * GPIO and interrupt-on-change host-sim probe. Host-only: drives the
 * simulator (pic16f188xx_sim_*), so it is not in the XC8 build. Covers a
 * full-port write on PORTA, RB4 output, RB5 input, and an RB6 rising edge
 * through the registered callback, plus the 40-pin PORTD full-port path
 * and the RE0..RE2 PORTE mask. All of PORTA..PORTC are IOC-capable; the
 * shared contract reports PORTB.
 */
#include "pic16f188xx.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "pic16f188xx_sim.h"

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

    /* PORTA is 8 pins: a full-port write reaches every bit. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOA, 0xFFU);
    epic_harness_tick(); /* sim refreshes PORTx per step; reads are not live */
    ok &= (EPIC_GPIO_ReadPort(GPIOA) == PIC16F188XX_FAMILY_PORTA_MASK);

    /* RB4 output: the simulated pin follows the latch. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    ok &= (pic16f188xx_sim_read_output('B', 4) == 1U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    ok &= (pic16f188xx_sim_read_output('B', 4) == 0U);

    /* RB5 input: ReadPin follows the driven level. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    pic16f188xx_sim_drive_input('B', 5, 1);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET);
    pic16f188xx_sim_drive_input('B', 5, 0);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_RESET);

    /* IOC on RB6: arming one port must not disturb the others. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_6, GPIO_MODE_INPUT);
    EPIC_GPIO_RegisterChangeCallback(on_ioc);
    EPIC_GPIO_EnableChangeDetect(GPIOB, GPIO_PIN_6, 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == GPIO_PIN_6);
    ok &= (EPIC_REG8(PIC_REG_IOCAP) == 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCCP) == 0U);
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_IOCIE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    EPIC_IRQ_Restore(1U);

    pic16f188xx_sim_drive_input('B', 6, 0);
    epic_harness_tick();
    ok &= (g_ioc_seen == 0U);
    pic16f188xx_sim_drive_input('B', 6, 1);
    epic_harness_tick();
    ok &= (g_ioc_seen == 1U);
    ok &= (g_ioc_iocbf == GPIO_PIN_6);

#if PIC16F188XX_FAMILY_HAS_PORTD
    /* PORTD is 8 driven pins: full-port write lands every bit. */
    EPIC_GPIO_Init(GPIOD, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOD, 0xA5U);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPort(GPIOD) == 0xA5U);
    EPIC_GPIO_DeInit(GPIOD);
    ok &= (EPIC_REG8(PIC_REG_TRISD) == PIC16F188XX_FAMILY_PORTD_MASK);
    ok &= (EPIC_REG8(PIC_REG_ANSELD) == PIC16F188XX_FAMILY_PORTD_MASK);
    ok &= (EPIC_REG8(PIC_REG_LATD) == 0U);
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    /* PORTE is RE0..RE2 (RE3 is input-only MCLR): writes above bit 2
     * are ignored, reads mask to the driven pins. */
    EPIC_GPIO_Init(GPIOE, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOE, 0xFFU);
    epic_harness_tick();
    ok &= (EPIC_GPIO_ReadPort(GPIOE) == PIC16F188XX_FAMILY_PORTE_MASK);
    ok &= ((EPIC_REG8(PIC_REG_LATE) & (uint8_t)~PIC16F188XX_FAMILY_PORTE_MASK) == 0U);
    /* PORTD/E arm no edges: the PORTB/C IOC registers keep their values. */
    EPIC_GPIO_EnableChangeDetect(GPIOD, GPIO_PIN_All, GPIO_PIN_All);
    EPIC_GPIO_EnableChangeDetect(GPIOE, GPIO_PIN_All, GPIO_PIN_All);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == GPIO_PIN_6);
    ok &= (EPIC_REG8(PIC_REG_IOCCP) == 0U);
#endif

    epic_harness_log("gpio/IOC smoke: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
