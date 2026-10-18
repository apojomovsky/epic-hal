/**
 * PPS timer-input host-sim probe for T1CKIPPS, T1GPPS and T2AINPPS (the
 * datasheet's T2IN). Host-only: Timer1 and Timer2 are driven through the
 * simulator, so it is not in the XC8 build. Covers the POR images, the
 * per-input port guard (EDC ppsports: T1CKI A/C, T1G B/C, T2IN A/C), the
 * register images, BUSY while locked, the DeInit restore, and each timer
 * following the pin its select names instead of the POR pin.
 *
 * Expected register image after the mappings below:
 *   T1CKIPPS = 0x03 (RA3), T1GPPS = 0x0A (RB2), T2AINPPS = 0x05 (RA5)
 */
#include "pic16f188xx.h"
#include "pic16f188xx_sim.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f188xx_pps.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"

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
 * @brief Check the three select registers against expected images.
 * @param t1cki Expected T1CKIPPS.
 * @param t1g Expected T1GPPS.
 * @param t2in Expected T2AINPPS.
 * @return 1 when all three match.
 */
static int images(uint8_t t1cki, uint8_t t1g, uint8_t t2in)
{
    return (EPIC_REG8(PIC_REG_T1CKIPPS) == t1cki) &&
           (EPIC_REG8(PIC_REG_T1GPPS) == t1g) &&
           (EPIC_REG8(PIC_REG_T2AINPPS) == t2in);
}

/**
 * @brief Timer1 clock and gate on remapped pins.
 * @return 1 on success.
 */
static int timer1_follows_pins(void)
{
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    int ok = 1;

    /* T1CKI on RA3: its edges count, edges on the POR pin RC0 do not. */
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTA, 3U) == EPIC_OK);
    h.ClockSource = TIMER1_CS_T1CKIPPS;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    pic16f188xx_sim_drive_input('C', 0U, 1U);
    step(3U);
    pic16f188xx_sim_drive_input('C', 0U, 0U);
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    pic16f188xx_sim_drive_input('A', 3U, 1U);
    step(3U);
    ok &= (EPIC_TIMER1_ReadCounter() == 1U);
    pic16f188xx_sim_drive_input('A', 3U, 0U);
    step(3U);
    pic16f188xx_sim_drive_input('A', 3U, 1U);
    step(1U);
    ok &= (EPIC_TIMER1_ReadCounter() == 2U);
    pic16f188xx_sim_drive_input('A', 3U, 0U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

    /* T1G on RB2: the gate counts while RB2 is high and ignores RB5. */
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTB, 2U) == EPIC_OK);
    h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    h.Gate.Enable = 1U;
    h.Gate.ActiveHigh = 1U;
    ok &= (EPIC_TIMER1_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&h) == EPIC_OK);
    pic16f188xx_sim_drive_input('B', 5U, 1U);
    step(4U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    pic16f188xx_sim_drive_input('B', 5U, 0U);
    pic16f188xx_sim_drive_input('B', 2U, 1U);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 5U);
    ok &= (EPIC_TIMER1_GateValue() == 1U);
    pic16f188xx_sim_drive_input('B', 2U, 0U);
    step(5U);
    ok &= (EPIC_TIMER1_ReadCounter() == 5U);
    ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    return ok;
}

/**
 * @brief Timer2 hardware gate on a remapped T2IN pin.
 * @return 1 on success.
 */
static int timer2_follows_pin(void)
{
    TIMER2_HandleTypeDef h = TIMER2_HANDLE_DEFAULT;
    int ok = 1;

    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTA, 5U) == EPIC_OK);
    h.Mode = TIMER2_MODE_FREE_GATE_HIGH;
    h.Period = 100U;
    ok &= (EPIC_TIMER2_Init(&h) == EPIC_OK);
    ok &= (EPIC_TIMER2_Start(&h) == EPIC_OK);
    pic16f188xx_sim_drive_input('C', 3U, 1U);
    step(4U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    pic16f188xx_sim_drive_input('C', 3U, 0U);
    pic16f188xx_sim_drive_input('A', 5U, 1U);
    step(3U);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    pic16f188xx_sim_drive_input('A', 5U, 0U);
    step(4U);
    ok &= (EPIC_TIMER2_ReadCounter() == 3U);
    ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
    ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    return ok;
}

/**
 * @brief PPS timer-input checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    /* POR images. */
    ok &= images(0x10U, 0x0DU, 0x13U);

    /* Validation: nothing is written for a bad argument. */
    ok &= (EPIC_PPS_MapInput((PPS_InputTypeDef)13, PPS_PORTA, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTA, 8U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, (PPS_PortTypeDef)4, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTB, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTD, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTA, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTD, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTB, 8U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTB, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTD, 0U) == EPIC_INVALID);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTA, 8U) == EPIC_INVALID);
    ok &= images(0x10U, 0x0DU, 0x13U);

    /* Register images: (port << 3) | pin, each input on both its ports. */
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTC, 7U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTC, 6U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTC, 4U) == EPIC_OK);
    ok &= images(0x17U, 0x16U, 0x14U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTA, 3U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTB, 2U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTA, 5U) == EPIC_OK);
    ok &= images(0x03U, 0x0AU, 0x05U);

    /* Locked: BUSY without a write; argument errors still report INVALID. */
    ok &= (EPIC_PPS_Lock() == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1CKI, PPS_PORTC, 0U) == EPIC_BUSY);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTB, 5U) == EPIC_BUSY);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T2IN, PPS_PORTC, 3U) == EPIC_BUSY);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_T1G, PPS_PORTA, 0U) == EPIC_INVALID);
    ok &= images(0x03U, 0x0AU, 0x05U);
    ok &= (EPIC_PPS_Unlock() == EPIC_OK);

    ok &= timer1_follows_pins();
    ok &= timer2_follows_pin();

    /* DeInit restores the POR images. */
    ok &= (EPIC_PPS_DeInit() == EPIC_OK);
    ok &= images(0x10U, 0x0DU, 0x13U);

    epic_harness_log("pps_inputs: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
