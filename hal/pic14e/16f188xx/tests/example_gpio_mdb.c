/**
 * GPIO, IOC and Timer0..Timer2 register-image probe for the mdb gate,
 * ending in live interrupt smokes where the target allows them.
 * Target-safe: only register reads and bounded spins. Also runs on the
 * host simulator through ctest.
 *
 * Expected register image: TRISB RB4 clears on output init; ANSELB RB4/5
 * clear on digital init; LATB follows WritePin then TogglePin; PORTA
 * full-port write lands every bit but RA0 (the harness result pin, held
 * low until PASS); WPUC follows SetPullups; LATD follows a PORTD
 * full-port write and TRISD/ANSELD clear on output init (40-pin parts);
 * LATE keeps only RE0..RE2 (40-pin parts); IOCBP arms 0xFF; PIE0.IOCIE
 * and INTCON.PEIE set on enable and IOCIE clears on disable.
 *
 * Live smoke: software-set IOCAF1, IOCBF0 and IOCCF2 with IOCIE, PEIE
 * and GIE armed vectors the core, and the dispatcher runs the
 * registered callback and clears the flags. MPLAB SIM ignores a
 * software write to PIR0.IOCIF, so the probe first checks the write
 * reads back: on targets where it does not (the SIM gate) the smoke is
 * skipped and the gate covers the register images only; where it does
 * (host sim, real silicon) the vectored path runs and asserts the
 * callback and the cleared flags.
 *
 * Timer0 8-bit free-run at FOSC/4, 1:1, 1:1, period 7, in two phases.
 * Setup proof runs without a callback, so no vector can stop the timer
 * between Start and the image reads: T0CON1 reads 0x40, T0CON0 reads
 * T0EN set with the config bits clear, TMR0H holds 7, and a bounded
 * spin reaches TMR0IF with the counter still inside the period. T0OUT
 * is live read-only status and masks out of both T0CON0 images. The
 * vector proof then re-inits with the slot armed and goes straight from
 * Start into the spin: the overflow vectors through the dispatcher, the
 * handler clears TMR0IF and runs the slot, and the callback stops the
 * timer so the flag and counter readbacks that follow are stable.
 * DeInit restores T0CON1 0x00, TMR0H 0xFF and T0CON0 masked the same
 * way.
 *
 * Timer1 and Timer2 follow the same two-phase shape (period 8 counts:
 * Timer1 reload 0xFFF8, Timer2 PR2 7). Register images: T1CLK 0x01,
 * T1GATE 0x00, T1GCON 0x00 and T1CON 0x02 (RD16) after Init, 0x03 after
 * Start; T2CLKCON 0x01, T2RST 0x00, T2HLT 0x00 and T2CON 0x00 with PR2 7
 * after Init, 0x80 after Start; DeInit restores the POR images. T1GCON
 * reads GVAL live, so it masks out of the images. Gate proofs use the
 * POR PPS pins as self-driven inputs (RB5 is T1G, RC3 is T2IN), set as
 * outputs so their latch is the pin level: Timer1 with an active-high
 * gate holds while RB5 is low and counts once it is high; Timer2 in
 * hardware-gate-high mode holds while RC3 is low and runs once high.
 * A full-field T2CON/T2HLT/T2RST image (1:128, 1:16, falling clock,
 * both sync bits, LC4 reset) pins every bit position.
 */
#include "pic16f188xx.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"

/** @brief Halt the target after the probe result is written (mdb gate). */
extern void pic16f188xx_harness_halt(void);

static volatile uint8_t s_ioc_seen = 0;
static volatile uint8_t s_ioc_iocbf = 0;
static volatile uint8_t s_t0_seen = 0;
static volatile uint8_t s_t1_seen = 0;
static volatile uint8_t s_t2_seen = 0;

/**
 * @brief Smoke callback: records that the vectored handler ran.
 * @param iocbf Captured IOCBF bits, expected RB0.
 * @param portb PORTB byte read with the flags (unused here).
 */
static void on_ioc(uint8_t iocbf, uint8_t portb)
{
    (void)portb;
    s_ioc_seen = 1U;
    s_ioc_iocbf = iocbf;
}

/**
 * @brief Overflow callback: records the vectored run and halts the timer.
 * @details Stopping here freezes the counter the moment the handler clears
 * TMR0IF, so no later overflow re-sets the flag before main reads it back.
 * Without this the flag readback races the free-running timer.
 */
static void on_t0(void)
{
    s_t0_seen = 1U;
    EPIC_TIMER0_Stop();
}

/**
 * @brief Timer1 overflow callback: records the vectored run and halts the timer.
 * @details Same shape as on_t0: the stop keeps the flag readback stable.
 */
static void on_t1(void)
{
    s_t1_seen = 1U;
    EPIC_TIMER1_Stop();
}

/**
 * @brief Timer2 match callback: records the vectored run and halts the timer.
 * @details Same shape as on_t0: the stop keeps the flag readback stable.
 */
static void on_t2(void)
{
    s_t2_seen = 1U;
    EPIC_TIMER2_Stop();
}

/**
 * @brief GPIO, IOC, Timer0..Timer2 and interrupt smoke probe for the mdb gate.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;
    unsigned int i;

    epic_harness_init(0UL);

    /* RB4 output: TRIS cleared, analog cleared, latch follows WritePin and TogglePin. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_4) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_4) == 0U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) != 0U);
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) == 0U);

    /* RB5 input: TRIS set, analog cleared. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_5) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_5) == 0U);

    /* Full-port write on PORTA: every latch bit lands but RA0, which the
     * harness holds low until the PASS marker. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOA, (uint8_t)~GPIO_PIN_0);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & PIC16F188XX_FAMILY_LATA_MASK) ==
           (uint8_t)(PIC16F188XX_FAMILY_LATA_MASK & (uint8_t)~GPIO_PIN_0));

    /* PORTC pull-ups: WPUC follows SetPullups both ways. */
    EPIC_GPIO_SetPullups(GPIOC, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_WPUC) & (GPIO_PIN_2 | GPIO_PIN_3)) == (GPIO_PIN_2 | GPIO_PIN_3));
    EPIC_GPIO_SetPullups(GPIOC, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_WPUC) & (GPIO_PIN_2 | GPIO_PIN_3)) == 0U);

#if PIC16F188XX_FAMILY_HAS_PORTD
    /* PORTD full-port path: direction and analog clear on output init,
     * the latch follows a full-port write. */
    EPIC_GPIO_Init(GPIOD, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISD) & PIC16F188XX_FAMILY_PORTD_MASK) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELD) & PIC16F188XX_FAMILY_PORTD_MASK) == 0U);
    EPIC_GPIO_WritePort(GPIOD, 0xA5U);
    ok &= (EPIC_REG8(PIC_REG_LATD) == 0xA5U);
#endif
#if PIC16F188XX_FAMILY_HAS_PORTE
    /* PORTE mask: RE3 is input-only, so only RE0..RE2 land in LATE. */
    EPIC_GPIO_Init(GPIOE, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOE, 0xFFU);
    ok &= ((EPIC_REG8(PIC_REG_LATE) & (uint8_t)~PIC16F188XX_FAMILY_PORTE_MASK) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_LATE) & PIC16F188XX_FAMILY_PORTE_MASK) ==
           PIC16F188XX_FAMILY_PORTE_MASK);
#endif

    /* IOC enable path: arming PORTB writes IOCBP in full, enabling sets
     * PIE0.IOCIE and PEIE, disabling clears IOCIE. */
    EPIC_GPIO_EnableChangeDetect(GPIOB, GPIO_PIN_All, 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == PIC16F188XX_FAMILY_IOCB_MASK);
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_IOCIE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    EPIC_IRQ_DisableSrc(PIC16F188XX_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_IOCIE) == 0U);

    /* Live interrupt smoke where the target honors a software flag: arm
     * one pin per port, re-enable, set GIE, then set the flag trio the
     * way edges would. The core vectors, the dispatcher runs the
     * callback, and the handler clears every flag. */
    EPIC_GPIO_RegisterChangeCallback(on_ioc);
    EPIC_GPIO_EnableChangeDetect(GPIOA, GPIO_PIN_1, 0U);
    EPIC_GPIO_EnableChangeDetect(GPIOB, GPIO_PIN_0, 0U);
    EPIC_GPIO_EnableChangeDetect(GPIOC, GPIO_PIN_2, 0U);
    EPIC_IRQ_Enable(PIC16F188XX_IRQ_IOC);
    EPIC_IRQ_Restore(1U);
    EPIC_REG8(PIC_REG_IOCAF) |= GPIO_PIN_1;
    EPIC_REG8(PIC_REG_IOCBF) |= GPIO_PIN_0;
    EPIC_REG8(PIC_REG_IOCCF) |= GPIO_PIN_2;
    EPIC_REG8(PIC_REG_PIR0) |= PIC_PIR0_IOCIF;
    if ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_IOCIF) != 0U)
    {
        for (i = 0U; (i < 255U) && (s_ioc_seen == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= (s_ioc_seen == 1U);
        ok &= (s_ioc_iocbf == GPIO_PIN_0);
        ok &= ((EPIC_REG8(PIC_REG_IOCAF) & GPIO_PIN_1) == 0U);
        ok &= ((EPIC_REG8(PIC_REG_IOCBF) & GPIO_PIN_0) == 0U);
        ok &= ((EPIC_REG8(PIC_REG_IOCCF) & GPIO_PIN_2) == 0U);
        ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_IOCIF) == 0U);
    }
    else
    {
        EPIC_REG8(PIC_REG_IOCAF) &= (uint8_t)~GPIO_PIN_1;
        EPIC_REG8(PIC_REG_IOCBF) &= (uint8_t)~GPIO_PIN_0;
        EPIC_REG8(PIC_REG_IOCCF) &= (uint8_t)~GPIO_PIN_2;
    }

    /* Timer0 8-bit free-run at FOSC/4, 1:1, 1:1, period 7, in two
     * phases. Setup proof runs callback-free (no vector can stop the
     * timer before the image reads); the vector proof goes straight
     * from the armed Start into the spin. T0OUT masks out of both
     * T0CON0 images; either spin exits on its condition, so the gate
     * never depends on SIM wall-clock speed. */
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
        t0.Period = 7U;
        t0.OverflowCallback = NULL;
        ok &= (EPIC_TIMER0_Init(&t0) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0x40U);
        ok &= (EPIC_REG8(PIC_REG_T0CON0) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_TMR0H) == 7U);
        ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) == 0U);
        ok &= (EPIC_TIMER0_Start(&t0) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_T0CON0) & PIC_T0CON0_T0EN) != 0U);
        ok &= ((EPIC_REG8(PIC_REG_T0CON0) &
               (uint8_t)~(PIC_T0CON0_T0EN | PIC_T0CON0_T0OUT)) == 0U);
        for (i = 0U; (i < 40000U) &&
            ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) != 0U);
        ok &= (EPIC_TIMER0_ReadCounter() <= 0x0707U);
        ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
    }
    {
        TIMER0_HandleTypeDef t0 = TIMER0_HANDLE_DEFAULT;
        t0.Period = 7U;
        t0.OverflowCallback = on_t0;
        ok &= (EPIC_TIMER0_Init(&t0) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE0) & PIC_PIE0_TMR0IE) != 0U);
        ok &= (EPIC_TIMER0_Start(&t0) == EPIC_OK);
        for (i = 0U; (i < 40000U) && (s_t0_seen == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= (s_t0_seen == 1U);
        ok &= ((EPIC_REG8(PIC_REG_PIR0) & PIC_PIR0_TMR0IF) == 0U);
        ok &= (EPIC_TIMER0_ReadCounter() <= 0x0707U);
        ok &= (EPIC_TIMER0_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER0_DeInit() == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_T0CON0) & (uint8_t)~PIC_T0CON0_T0OUT) == 0U);
        ok &= (EPIC_REG8(PIC_REG_T0CON1) == 0U);
        ok &= (EPIC_REG8(PIC_REG_TMR0H) == 0xFFU);
    }

    /* Timer1 free-run at FOSC/4, 1:1, reload 0xFFF8: overflow after 8
     * counts, callback-free setup proof then the vector proof. */
    {
        TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
        t1.ReloadValue = 0xFFF8U;
        ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x01U);
        ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x00U);
        ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x02U);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);
        ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x03U);
        for (i = 0U; (i < 40000U) &&
            ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) != 0U);
        ok &= (EPIC_TIMER1_ReadCounter() < 0x0100U);
        ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    }
    {
        TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
        t1.ReloadValue = 0xFFF8U;
        t1.OverflowCallback = on_t1;
        ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) != 0U);
        ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
        for (i = 0U; (i < 40000U) && (s_t1_seen == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= (s_t1_seen == 1U);
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U);
        ok &= (EPIC_TIMER1_ReadCounter() < 0x0100U);
        ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);
        ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x00U);
    }

    /* Timer1 gate on RB5 (T1GPPS at POR), active-high, counting from 0:
     * held while the pin is low, counting once it is high, held again
     * when it falls. The toggle and single-pulse bits pin the image. */
    {
        TIMER1_HandleTypeDef t1 = TIMER1_HANDLE_DEFAULT;
        uint16_t held;
        EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUTPUT);
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        t1.Gate.Enable = 1U;
        t1.Gate.ActiveHigh = 1U;
        ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0xC0U);
        ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
        for (i = 0U; i < 200U; i++)
        {
            epic_harness_tick();
        }
        ok &= (EPIC_TIMER1_ReadCounter() == 0U);
        ok &= (EPIC_TIMER1_GateValue() == 0U);
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
        for (i = 0U; (i < 40000U) && (EPIC_TIMER1_ReadCounter() == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= (EPIC_TIMER1_ReadCounter() != 0U);
        ok &= (EPIC_TIMER1_GateValue() == 1U);
        EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
        for (i = 0U; i < 50U; i++)
        {
            epic_harness_tick();
        }
        held = EPIC_TIMER1_ReadCounter();
        for (i = 0U; i < 200U; i++)
        {
            epic_harness_tick();
        }
        ok &= (EPIC_TIMER1_ReadCounter() == held);
        ok &= (EPIC_TIMER1_GateValue() == 0U);
        ok &= (EPIC_TIMER1_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);

        t1.Gate.Toggle = 1U;
        t1.Gate.SinglePulse = 1U;
        t1.Gate.Source = TIMER1_GATE_TMR2_POST;
        ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0xF0U);
        ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x03U);
        ok &= (EPIC_TIMER1_DeInit() == EPIC_OK);
    }

    /* Timer2 free-run at FOSC/4, 1:1, 1:1, PR2 7, in two phases like
     * Timer0. */
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
        t2.Period = 7U;
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x01U);
        ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
        ok &= (EPIC_TIMER2_ReadPeriod() == 7U);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x80U);
        for (i = 0U; (i < 40000U) &&
            ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U);
        ok &= (EPIC_TIMER2_ReadCounter() <= 7U);
        ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
    }
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
        t2.Period = 7U;
        t2.OverflowCallback = on_t2;
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) != 0U);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        for (i = 0U; (i < 40000U) && (s_t2_seen == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= (s_t2_seen == 1U);
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U);
        ok &= (EPIC_TIMER2_ReadCounter() <= 7U);
        ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
        ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);
        ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x00U);
        ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
        ok &= (EPIC_TIMER2_ReadPeriod() == 0xFFU);
        ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    }

    /* Timer2 full-field image, then the hardware gate on RC3 (T2INPPS
     * at POR): held while low, running once high. */
    {
        TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;
        t2.Prescaler = TIMER2_PRESCALER_1_128;
        t2.Postscaler = TIMER2_POSTSCALER_1_16;
        t2.Mode = TIMER2_MODE_FREE_GATE_HIGH;
        t2.ResetSource = TIMER2_RST_LC4OUT;
        t2.FallingEdge = 1U;
        t2.SyncOn = 1U;
        t2.SyncPrescaler = 1U;
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x7FU);
        ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0xE1U);
        ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x11U);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);

        t2 = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
        t2.Mode = TIMER2_MODE_FREE_GATE_HIGH;
        t2.Period = 7U;
        EPIC_GPIO_Init(GPIOC, GPIO_PIN_3, GPIO_MODE_OUTPUT);
        EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
        ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
        ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x01U);
        ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
        for (i = 0U; i < 200U; i++)
        {
            epic_harness_tick();
        }
        ok &= (EPIC_TIMER2_ReadCounter() == 0U);
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U);
        EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        for (i = 0U; (i < 40000U) &&
            ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U); i++)
        {
            epic_harness_tick();
        }
        ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U);
        ok &= (EPIC_TIMER2_Stop() == EPIC_OK);
        ok &= (EPIC_TIMER2_DeInit() == EPIC_OK);
        EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    }

    int rc = epic_harness_report(ok);
    pic16f188xx_harness_halt();
    return rc;
}
