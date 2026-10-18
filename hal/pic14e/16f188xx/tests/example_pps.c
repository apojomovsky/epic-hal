/**
 * PPS host-sim probe. Host-only: the Timer0 section drives T0CKI pins
 * through the simulator, so it is not in the XC8 build. Covers argument
 * validation (ports, pins, the T0CKI PORTA/PORTB limit), the xxxPPS and
 * RxyPPS register images, the lock sequence and the BUSY refusal while
 * locked, the POR restore in DeInit, and Timer0 counting edges on the pin
 * T0CKIPPS selects (rising for T0CKIPPS, falling for the inverted row).
 *
 * Expected register image after the mappings below (16F18877, PORTD present):
 *   T0CKIPPS = 0x0D (RB5), SSP1CLKPPS = 0x13 (RC3), RXPPS = 0x17 (RC7)
 *   RC6PPS = 0x10 (TX), RC3PPS = 0x14 (SCK1), RD7PPS = 0x15 (SDO1)
 */
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_pps.h"
#include "peripherals/pic16f188xx_timer0.h"

/**
 * @brief Read one RxyPPS output select.
 * @param base Address of the port's R<x>0PPS register.
 * @param pin Pin number 0..7.
 * @return The register value.
 */
static uint8_t out_reg(uint16_t base, uint8_t pin)
{
    return EPIC_REG8((uint16_t)(base + pin));
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
 * @brief Drive PORTB bit 5 high then low, a full pulse, one tick each.
 */
static void pulse_rb5(void)
{
    pic16f188xx_sim_drive_input('B', 5U, 1U);
    step(1U);
    pic16f188xx_sim_drive_input('B', 5U, 0U);
    step(1U);
}

/**
 * @brief PPS checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
    uint8_t gie;

    epic_harness_init(10000UL);

    /* Validation: nothing is written for a bad argument. */
    ok &= (EPIC_PPS_MapInput((PPS_InputTypeDef)10, PPS_PORTA, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTC, 8U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, (PPS_PortTypeDef)4, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T0CKI, PPS_PORTC, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 8U, PPS_OUTPUT_EUSART_TX) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput((PPS_PortTypeDef)4, 0U, PPS_OUTPUT_EUSART_TX) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 0U, (PPS_OutputTypeDef)0x11) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 0U, (PPS_OutputTypeDef)0x08) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 0U, (PPS_OutputTypeDef)0x01) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 0U, (PPS_OutputTypeDef)0x16) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x17U);
    ok &= (EPIC_REG8(PIC_REG_T0CKIPPS) == 0x04U);
    ok &= (out_reg(PIC_REG_RC0PPS, 6U) == 0x00U);

    /* Register images: input selects hold (port << 3) | pin, output
     * selects the source code. */
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T0CKI, PPS_PORTB, 5U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_SSP1CLK, PPS_PORTC, 3U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_SSP1DAT, PPS_PORTC, 4U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_SSP1SS, PPS_PORTA, 5U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTC, 7U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CKIPPS) == 0x0DU);
    ok &= (EPIC_REG8(PIC_REG_SSP1CLKPPS) == 0x13U);
    ok &= (EPIC_REG8(PIC_REG_SSP1DATPPS) == 0x14U);
    ok &= (EPIC_REG8(PIC_REG_SSP1SSPPS) == 0x05U);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x17U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTB, 1U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x09U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 6U, PPS_OUTPUT_EUSART_TX) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 3U, PPS_OUTPUT_SSP1_SCK) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 0U, PPS_OUTPUT_SSP1_SDO) == EPIC_OK);
    ok &= (out_reg(PIC_REG_RC0PPS, 6U) == 0x10U);
    ok &= (out_reg(PIC_REG_RC0PPS, 3U) == 0x14U);
    ok &= (out_reg(PIC_REG_RB0PPS, 0U) == 0x15U);
    ok &= (out_reg(PIC_REG_RA0PPS, 0U) == 0x00U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP1, PPS_PORTB, 2U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP2, PPS_PORTC, 6U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP3, PPS_PORTC, 0U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP4, PPS_PORTB, 7U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP5, PPS_PORTA, 1U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1PPS) == 0x0AU);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x16U);
    ok &= (EPIC_REG8(PIC_REG_CCP3PPS) == 0x10U);
    ok &= (EPIC_REG8(PIC_REG_CCP4PPS) == 0x0FU);
    ok &= (EPIC_REG8(PIC_REG_CCP5PPS) == 0x01U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 1U, PPS_OUTPUT_CCP1) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 5U, PPS_OUTPUT_PWM7) == EPIC_OK);
    ok &= (out_reg(PIC_REG_RB0PPS, 1U) == 0x09U);
    ok &= (out_reg(PIC_REG_RC0PPS, 5U) == 0x0FU);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 4U, PPS_OUTPUT_CCP2) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 2U, PPS_OUTPUT_CCP3) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 1U, PPS_OUTPUT_CCP4) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 3U, PPS_OUTPUT_CCP5) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_PWM6) == EPIC_OK);
    ok &= (out_reg(PIC_REG_RC0PPS, 4U) == 0x0AU);
    ok &= (out_reg(PIC_REG_RC0PPS, 2U) == 0x0BU);
    ok &= (out_reg(PIC_REG_RC0PPS, 1U) == 0x0CU);
    ok &= (out_reg(PIC_REG_RA0PPS, 3U) == 0x0DU);
    ok &= (out_reg(PIC_REG_RA0PPS, 2U) == 0x0EU);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 3U, PPS_OUTPUT_LAT) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_LAT) == EPIC_OK);
#if PIC16F188XX_FAMILY_HAS_PORTD
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_SSP1DAT, PPS_PORTD, 2U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1DATPPS) == 0x1AU);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTD, 7U, PPS_OUTPUT_SSP1_SDO) == EPIC_OK);
    ok &= (out_reg(PIC_REG_RD0PPS, 7U) == 0x15U);
#else
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_SSP1DAT, PPS_PORTD, 2U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTD, 7U, PPS_OUTPUT_SSP1_SDO) == EPIC_INVALID);
#endif

    /* A pin handed back to the latch reads 0 again. */
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 0U, PPS_OUTPUT_LAT) == EPIC_OK);
    ok &= (out_reg(PIC_REG_RB0PPS, 0U) == 0x00U);

    /* Lock: the key sequence sets PPSLOCKED, mapping and DeInit are then
     * refused without a write, and Unlock reopens them. GIE survives the
     * sequence in both states. */
    EPIC_IRQ_Restore(1U);
    ok &= (EPIC_PPS_IsLocked() == 0U);
    ok &= (EPIC_PPS_Lock() == EPIC_OK);
    ok &= (EPIC_PPS_IsLocked() == 1U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE) != 0U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTC, 7U) == EPIC_BUSY);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 6U, PPS_OUTPUT_LAT) == EPIC_BUSY);
    ok &= (EPIC_PPS_DeInit() == EPIC_BUSY);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x09U);
    ok &= (out_reg(PIC_REG_RC0PPS, 6U) == 0x10U);
    EPIC_IRQ_Restore(0U);
    ok &= (EPIC_PPS_Unlock() == EPIC_OK);
    ok &= (EPIC_PPS_IsLocked() == 0U);
    gie = (uint8_t)(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_GIE);
    ok &= (gie == 0U);

    /* DeInit restores every select to its POR value. */
    ok &= (EPIC_PPS_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T0CKIPPS) == 0x04U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CLKPPS) == 0x13U);
    ok &= (EPIC_REG8(PIC_REG_SSP1DATPPS) == 0x14U);
    ok &= (EPIC_REG8(PIC_REG_SSP1SSPPS) == 0x05U);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x17U);
    ok &= (out_reg(PIC_REG_RC0PPS, 6U) == 0x00U);
    ok &= (out_reg(PIC_REG_RC0PPS, 3U) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCP1PPS) == 0x12U);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x11U);
    ok &= (EPIC_REG8(PIC_REG_CCP3PPS) == 0x0DU);
    ok &= (EPIC_REG8(PIC_REG_CCP4PPS) == 0x08U);
    ok &= (EPIC_REG8(PIC_REG_CCP5PPS) == 0x04U);
    ok &= (out_reg(PIC_REG_RB0PPS, 1U) == 0x00U);
    ok &= (out_reg(PIC_REG_RC0PPS, 5U) == 0x00U);

    /* Timer0 on T0CKI: edges on the mapped pin count, edges on the POR
     * pin (RA4) do not. Period 3 in 8-bit mode flags on the 4th edge. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T0CKI, PPS_PORTB, 5U) == EPIC_OK);
    t0.ClockSource = TIMER0_CS_T0CKIPPS;
    t0.Period = 3U;
    ok &= (EPIC_TIMER0_Init(&t0) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&t0) == EPIC_OK);
    step(20U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0300U);
    pic16f188xx_sim_drive_input('A', 4U, 1U);
    step(2U);
    pic16f188xx_sim_drive_input('A', 4U, 0U);
    step(2U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0300U);
    pulse_rb5();
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0301U);
    pulse_rb5();
    pulse_rb5();
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0303U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
    pulse_rb5();
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0300U);
    ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);

    /* The inverted row counts the falling edge: the rising half of a
     * pulse does nothing. */
    t0.ClockSource = TIMER0_CS_T0CKIPPS_INV;
    ok &= (EPIC_TIMER0_Init(&t0) == EPIC_OK);
    ok &= (EPIC_TIMER0_Start(&t0) == EPIC_OK);
    pic16f188xx_sim_drive_input('B', 5U, 1U);
    step(1U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0300U);
    pic16f188xx_sim_drive_input('B', 5U, 0U);
    step(1U);
    ok &= (EPIC_TIMER0_ReadCounter() == 0x0301U);
    ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    ok &= (EPIC_PPS_DeInit() == EPIC_OK);

    epic_harness_log("pps: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
