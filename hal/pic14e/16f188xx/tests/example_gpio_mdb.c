/**
 * GPIO, IOC, Timer0..Timer2, CCP and PWM register-image probe for the mdb gate,
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
 * hardware-gate-high mode holds while RC3 is low and runs once high,
 * and in rising-edge one-shot mode holds at zero while armed (MPLAB
 * SIM never starts it, see SIM gaps).
 * A full-field T2CON/T2HLT/T2RST image (1:128, 1:16, falling clock,
 * both sync bits, LC4 reset) pins every bit position.
 *
 * CCP and PWM follow Timer2: CCPxCON, CCPRxH:L, CCPxCAP and CCPTMRS
 * images (every mode, a capture source and timer couple, left-aligned PWM
 * duty), a compare match against Timer1 (polled, then vectored on CCP5),
 * CCP1 PWM against Timer2 and the PWM6/PWM7 images with PWM6 against
 * Timer2. The PWM proofs sample the OUT bit and compare high counts of a
 * quarter and three quarters duty. MPLAB SIM never captures (it does not
 * drive CCPxPPS from a latch) and does not follow the timer-clearing and
 * pulse compare modes, so those get images only (family MANUAL.md). The
 * PPS section also maps the CCPx inputs and the CCP and PWM outputs.
 *
 * PPS, EUSART and MSSP1 follow: PPS select images at their (0xFxx or
 * 0x1Fxx) addresses; the EUSART 9600 baud image, a polled frame and a
 * vectored one-shot TX interrupt; the MSSP1 SPI and I2C images, an I2C
 * START request, an SPI byte where the target shifts it (MPLAB SIM does
 * not) and a software-set SSP1IF that vectors. The PPS lock section runs
 * last; the gate config sets PPS1WAY=OFF so the unlock path is real.
 *
 * Data EEPROM, NCO1, WDT and Idle come before the PPS lock. The EEPROM
 * read leaves NVMADR at 0xF0xx with NVMCON1 = NVMREGS (0x40); a write
 * leaves its address and data in NVMADR/NVMDATL with NVMREGS and WREN set
 * (0x44) until polled complete (0x40), and the bytes read back through
 * NVM. NCO1 images: pulse mode, width 8, inverted, increment 0x12345 reads
 * NCO1CON 0x11 (N1OUT masked), NCO1CLK 0x60, NCO1INC 0x012345; the
 * accumulator takes 0xABCDE while stopped; DeInit restores NCO1INCL 0x01
 * and zeroes the rest. WDTCON0 reads (WDTPS << 1) | SWDTEN for 1 ms
 * (0x01), 2 s (0x17) and 256 s (0x25). Idle wakes on a WDT time-out and
 * leaves CPUDOZE<IDLEN> clear.
 */
#include "pic16f188xx.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "peripherals/pic16f188xx_timer0.h"
#include "peripherals/pic16f188xx_timer1.h"
#include "peripherals/pic16f188xx_timer2.h"
#include "peripherals/pic16f188xx_pps.h"
#include "peripherals/pic16f188xx_usart.h"
#include "peripherals/pic16f188xx_ssp.h"
#include "peripherals/pic16f188xx_eeprom.h"
#include "peripherals/pic16f188xx_nco.h"
#include "core/pic16f188xx_wdt_sleep.h"
#include "core/pic16f188xx_irq.h"
#include "peripherals/pic16f188xx_ccp.h"
#include "peripherals/pic16f188xx_pwm.h"
#include "core/epic_harness.h"

/** @brief Halt the target after the probe result is written (mdb gate). */
extern void pic16f188xx_harness_halt(void);

static volatile uint8_t s_ioc_seen = 0;
static volatile uint8_t s_ioc_iocbf = 0;
static volatile uint8_t s_t0_seen = 0;
static volatile uint8_t s_t1_seen = 0;
static volatile uint8_t s_t2_seen = 0;
static volatile uint8_t s_tx_seen = 0;
static volatile uint8_t s_ssp_seen = 0;
static volatile uint8_t s_ccp_seen = 0;

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
 * @brief EUSART TX callback: records that the vectored handler ran.
 */
static void on_tx(void)
{
    s_tx_seen = 1U;
}

/**
 * @brief SSP1 callback: records that the vectored handler ran.
 */
static void on_ssp(void)
{
    s_ssp_seen = 1U;
}

/**
 * @brief CCP and PWM selects of the PPS section: the five capture inputs
 *        and the outputs. Ports follow the routes MPLAB SIM accepts on the
 *        40-pin parts (CCP1 and CCP2 on PORTB and PORTC, CCP3 and CCP4 on
 *        PORTB, CCP5, PWM6 and PWM7 on PORTA), so it raises no W1600.
 * @return 1 when every readback matches, 0 otherwise.
 */
static int gate_pps_ccp(void)
{
    int ok = 1;

    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP1, PPS_PORTB, 2U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP2, PPS_PORTC, 6U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP3, PPS_PORTB, 4U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP4, PPS_PORTB, 7U) == EPIC_OK);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_CCP5, PPS_PORTA, 1U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP1PPS) == 0x0AU);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x16U);
    ok &= (EPIC_REG8(PIC_REG_CCP3PPS) == 0x0CU);
    ok &= (EPIC_REG8(PIC_REG_CCP4PPS) == 0x0FU);
    ok &= (EPIC_REG8(PIC_REG_CCP5PPS) == 0x01U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 2U, PPS_OUTPUT_CCP1) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 1U, PPS_OUTPUT_CCP2) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 6U, PPS_OUTPUT_CCP3) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 7U, PPS_OUTPUT_CCP4) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 3U, PPS_OUTPUT_CCP5) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 1U, PPS_OUTPUT_PWM6) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_PWM7) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 2U) == 0x09U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 1U) == 0x0AU);
    ok &= (EPIC_REG8(PIC_REG_RB0PPS + 6U) == 0x0BU);
    ok &= (EPIC_REG8(PIC_REG_RB0PPS + 7U) == 0x0CU);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 3U) == 0x0DU);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 1U) == 0x0EU);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 2U) == 0x0FU);
    return ok;
}

/**
 * @brief PPS register-image section: input and output selects land at their
 *        (0xFxx or 0x1Fxx) addresses, and an unmapped pin hands back to the latch.
 * @return 1 when every readback matches, 0 otherwise.
 */
static int gate_pps(void)
{
    int ok = 1;

    ok &= (EPIC_PPS_Unlock() == EPIC_OK);
    ok &= (EPIC_PPS_IsLocked() == 0U);
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
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 6U, PPS_OUTPUT_EUSART_TX) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 3U, PPS_OUTPUT_SSP1_SCK) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 0U, PPS_OUTPUT_SSP1_SDO) == EPIC_OK);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 5U, PPS_OUTPUT_SSP1_SDO) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 6U) == 0x10U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 3U) == 0x14U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 5U) == 0x15U);
    ok &= (EPIC_REG8(PIC_REG_RB0PPS + 0U) == 0x15U);
    ok &= gate_pps_ccp();
#if PIC16F188XX_FAMILY_HAS_PORTD
    /* PORTD takes no EUSART or MSSP output (MANUAL.md, PPS), so only the
     * hand-back to the latch is written; its RD7PPS address still reads. */
    ok &= (EPIC_PPS_MapOutput(PPS_PORTD, 7U, PPS_OUTPUT_LAT) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RD0PPS + 7U) == 0x00U);
#endif
    ok &= (EPIC_PPS_MapOutput(PPS_PORTB, 0U, PPS_OUTPUT_LAT) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RB0PPS + 0U) == 0x00U);
    return ok;
}

/**
 * @brief EUSART section: 9600 baud register image, a polled frame, the
 *        vectored one-shot TX interrupt and the POR restore.
 * @details TRMT is live read-only status, so it masks out of the TX1STA
 * images. The TX interrupt proof relies on TXIF being set while the
 * transmitter is on and TX1REG is empty; the handler masks TXIE before the
 * callback, so the slot runs once.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_usart(void)
{
    int ok = 1;
    unsigned int i;
    USART_HandleTypeDef u = USART_HANDLE_DEFAULT;

    EPIC_GPIO_Init(GPIOC, GPIO_PIN_6, GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_7, GPIO_MODE_INPUT);
    u.SPBRG = USART_ComputeSPBRG(32000000UL, 9600UL, USART_BRGH_HIGH, USART_BAUDGEN_8BIT);
    ok &= (u.SPBRG == 207U);
    ok &= (EPIC_USART_Init(&u) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGL) == 0xCFU);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGH) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_BAUD1CON) & PIC_BAUD1CON_BRG16) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_TX1STA) & (uint8_t)~PIC_TX1STA_TRMT) == 0x24U);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0x90U);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & (PIC_PIE3_TXIE | PIC_PIE3_RCIE)) == 0U);

    /* One polled frame: TXIF returns once the byte moves to the shifter,
     * TRMT once the frame has left. Polling TXIF first avoids reading
     * TRMT in the window before the byte is taken. */
    EPIC_USART_Transmit(0x55U);
    for (i = 0U; (i < 60000U) && (EPIC_USART_IsTxBufferEmpty() == 0U); i++)
    {
        epic_harness_tick();
    }
    for (i = 0U; (i < 60000U) && (EPIC_USART_IsTxShiftRegisterEmpty() == 0U); i++)
    {
        epic_harness_tick();
    }
    ok &= (EPIC_USART_IsTxShiftRegisterEmpty() == 1U);
    ok &= (EPIC_USART_IsTxBufferEmpty() == 1U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RC1STA) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_TX1STA) & (uint8_t)~PIC_TX1STA_TRMT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_SP1BRGL) == 0x00U);

    /* Vectored TX interrupt: armed one-shot, runs the slot once and
     * leaves TXIE masked. */
    u.TxCpltCallback = on_tx;
    ok &= (EPIC_USART_Init(&u) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_TXIE) == 0U);
    s_tx_seen = 0U;
    EPIC_IRQ_Restore(1U);
    EPIC_USART_EnableTxInterrupt();
    for (i = 0U; (i < 255U) && (s_tx_seen == 0U); i++)
    {
        epic_harness_tick();
    }
    ok &= (s_tx_seen == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_TXIE) == 0U);
    EPIC_IRQ_Restore(0U);
    ok &= (EPIC_USART_DeInit() == EPIC_OK);
    return ok;
}

/**
 * @brief MSSP1 section: SPI and I2C register images, an I2C START request,
 *        one SPI byte through the shifter where the target models it, and
 *        the vectored SSP1 interrupt.
 * @details MPLAB SIM does not shift SSP1 data (BF and SSP1IF never set
 * after a SSP1BUF write), so there the SPI byte check is skipped and the
 * gate covers the register images; the host sim and silicon complete it.
 * A software write to PIR3.SSP1IF does read back and vector on the SIM.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_ssp(void)
{
    int ok = 1;
    unsigned int i;
    SSP_HandleTypeDef s = SSP_HANDLE_DEFAULT;

    /* SCK on RC3 and SDO on RC5 as outputs, SDI on RC4 as input, the way
     * gate_pps routed them. */
    EPIC_GPIO_Init(GPIOC, (uint8_t)(GPIO_PIN_3 | GPIO_PIN_5), GPIO_MODE_OUTPUT);
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_INPUT);
    ok &= (SSP_ComputeADD(32000000UL, 400000UL) == 19U);
    ok &= (EPIC_SSP_Init(&s) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_WRITE_MASK) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x20U);
    ok &= (EPIC_SSP_WriteByte(0xA5U) == 0xA5U);
    for (i = 0U; (i < 40000U) && (EPIC_SSP_IsBufferFull() == 0U); i++)
    {
        epic_harness_tick();
    }
    if (EPIC_SSP_IsBufferFull() != 0U)
    {
        (void)EPIC_SSP_ReadByte();
        ok &= (EPIC_SSP_IsBufferFull() == 0U);
    }
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x00U);

    s.Mode = SSP_MODE_I2C_MASTER;
    s.SamplePhase = SSP_SPI_SMP_END;
    s.Add = (uint8_t)SSP_ComputeADD(32000000UL, 400000UL);
    ok &= (EPIC_SSP_Init(&s) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_SSP1STAT) & PIC_SSP1STAT_WRITE_MASK) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_SSP1CON1) == 0x28U);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 19U);
    ok &= (EPIC_SSP_I2C_Start() == EPIC_OK);
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_SSP1ADD) == 0x00U);

    /* Vectored SSP1 interrupt: the flag set by software vectors the core,
     * the handler clears it and runs the slot. */
    s = (SSP_HandleTypeDef)SSP_HANDLE_DEFAULT;
    s.TransferCallback = on_ssp;
    ok &= (EPIC_SSP_Init(&s) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_SSP1IE) != 0U);
    s_ssp_seen = 0U;
    EPIC_REG8(PIC_REG_PIR3) |= PIC_PIR3_SSP1IF;
    EPIC_IRQ_Restore(1U);
    for (i = 0U; (i < 255U) && (s_ssp_seen == 0U); i++)
    {
        epic_harness_tick();
    }
    EPIC_IRQ_Restore(0U);
    ok &= (s_ssp_seen == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR3) & PIC_PIR3_SSP1IF) == 0U);
    ok &= (EPIC_SSP_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE3) & PIC_PIE3_SSP1IE) == 0U);
    return ok;
}

/**
 * @brief Spin until the EEPROM write finishes, bounded.
 * @details MPLAB SIM 6.35 clears WR at once on a CPU-executed write to
 * this part and sets no NVMIF (2026-10, 16F18877), so the bound is never
 * reached there; it keeps the probe from hanging on a target where the
 * write never completes.
 * @return 1 when the write finished, 0 when the bound ran out.
 */
static int eeprom_wait(void)
{
    unsigned int spin;

    for (spin = 0U; spin < 60000U; spin++)
    {
        if (EPIC_EEPROM_IsWriteComplete() != 0U)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Data EEPROM section: read and write register images, with the
 *        written bytes read back through NVM.
 * @details The read image is NVMADR 0xF0xx with NVMCON1 = NVMREGS. A write
 * leaves its address, data and NVMCON1 = NVMREGS|WREN (0x44) until polled
 * complete, which drops WREN (0x40). Blank-cell contents are not asserted:
 * MPLAB SIM's initial EEPROM is its own.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_eeprom(void)
{
    int ok = 1;

    ok &= (EPIC_EEPROM_Init(NULL) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x00U);
    (void)EPIC_EEPROM_ReadByte(0x10U);
    ok &= (EPIC_REG8(PIC_REG_NVMADRL) == 0x10U);
    ok &= (EPIC_REG8(PIC_REG_NVMADRH) == 0xF0U);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x40U);

    ok &= (EPIC_EEPROM_WriteByte(0x42U, 0xA5U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NVMADRL) == 0x42U);
    ok &= (EPIC_REG8(PIC_REG_NVMADRH) == 0xF0U);
    ok &= (EPIC_REG8(PIC_REG_NVMDATL) == 0xA5U);
    ok &= ((EPIC_REG8(PIC_REG_NVMCON1) & (PIC_NVMCON1_NVMREGS | PIC_NVMCON1_WREN)) ==
           (PIC_NVMCON1_NVMREGS | PIC_NVMCON1_WREN));
    ok &= eeprom_wait();
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x40U);
    ok &= (EPIC_EEPROM_HasWriteError() == 0U);
    ok &= (EPIC_EEPROM_ReadByte(0x42U) == 0xA5U);

    ok &= (EPIC_EEPROM_WriteByte(0xFFU, 0x5AU) == EPIC_OK);
    ok &= eeprom_wait();
    ok &= (EPIC_EEPROM_ReadByte(0xFFU) == 0x5AU);
    ok &= (EPIC_EEPROM_ReadByte(0x42U) == 0xA5U);
    ok &= (EPIC_EEPROM_HasWriteError() == 0U);
    ok &= (EPIC_EEPROM_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NVMCON1) == 0x00U);
    return ok;
}

/**
 * @brief NCO1 section: register images only, no output timing.
 * @details N1OUT is live read-only status (MPLAB SIM keeps the last level
 * after the module is disabled), so it masks out of the NCO1CON images.
 * @return 1 when every readback matches, 0 otherwise.
 */
static int gate_nco(void)
{
    int ok = 1;
    NCO_HandleTypeDef h = NCO_HANDLE_DEFAULT;

    h.Mode = NCO_MODE_PFM;
    h.PulseWidth = NCO_PULSE_8;
    h.Invert = 1U;
    h.Increment = 0x12345UL;
    ok &= (EPIC_NCO_Init(&h) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_NCO1CON) & (uint8_t)~PIC_NCO1CON_N1OUT) == 0x11U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0x60U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCU) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCH) == 0x23U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x45U);
    ok &= (EPIC_NCO_GetIncrement() == 0x12345UL);
    ok &= (EPIC_NCO_WriteAccumulator(0xABCDEUL) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCU) == 0x0AU);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCH) == 0xBCU);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCL) == 0xDEU);
    ok &= (EPIC_NCO_ReadAccumulator() == 0xABCDEUL);
    ok &= (EPIC_NCO_Start() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_NCO1CON) & PIC_NCO1CON_N1EN) != 0U);
    ok &= (EPIC_NCO_WriteAccumulator(0UL) == EPIC_BUSY);
    ok &= (EPIC_NCO_Stop() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_NCO1CON) & PIC_NCO1CON_N1EN) == 0U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_NCO1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 2U) == 0x19U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTA, 2U, PPS_OUTPUT_LAT) == EPIC_OK);
    ok &= (EPIC_NCO_DeInit() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_NCO1CON) & (uint8_t)~PIC_NCO1CON_N1OUT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1CLK) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCU) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1INCL) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCU) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_NCO1ACCL) == 0x00U);
    return ok;
}

/**
 * @brief Idle section: EPIC_Idle_Enter wakes on a WDT time-out.
 * @details The gate config has WDTE=SWDTEN, so the watchdog runs only
 * between EPIC_WDT_Enable and EPIC_WDT_Disable. In Sleep and Idle a
 * time-out wakes the part without a Reset (DS40001802D §9.6), so execution
 * resumes after the SLEEP. MPLAB SIM does not run Timer0 in Idle (2026-10,
 * 16F18877: TMR0 stayed frozen under IDLEN), so no peripheral wake is
 * used. The host simulator does not stop at SLEEP and only counts it.
 * IDLEN must be clear again afterwards.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_idle(void)
{
    int ok = 1;
    uint8_t gie = EPIC_IRQ_Disable();

    ok &= (EPIC_WDT_Enable(WDT_PERIOD_1MS) == EPIC_OK);
    EPIC_Idle_Enter();
    ok &= (EPIC_WDT_Disable() == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CPUDOZE) & PIC_CPUDOZE_IDLEN) == 0U);
    EPIC_IRQ_Restore(gie);
    return ok;
}

/**
 * @brief WDT section: period images, validation and Refresh.
 * @details The gate config has WDTE=SWDTEN, so the watchdog runs between
 * EPIC_WDT_Enable and EPIC_WDT_Disable. Each Enable rewrites WDTCON0,
 * which clears the WDT (DS40001802D §9.5), so no period elapses before
 * the next image check; Refresh (read WDTCON0, clrwdt) runs once disabled.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_wdt(void)
{
    int ok = 1;

    ok &= (EPIC_WDT_Enable(WDT_PERIOD_1MS) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x01U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_2S) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x17U);
    ok &= (EPIC_WDT_Enable(WDT_PERIOD_256S) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x25U);
    ok &= (EPIC_WDT_Enable((WDT_PeriodTypeDef)19) == EPIC_INVALID);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x25U);
    ok &= (EPIC_WDT_Disable() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x24U);
    EPIC_WDT_Refresh();
    ok &= (EPIC_REG8(PIC_REG_WDTCON0) == 0x24U);
    return ok;
}

/**
 * @brief PPS lock section: PPSLOCKED sets, mapping and DeInit are refused
 *        without a write, and the unlock sequence reopens them.
 * @details This gate's config sets PPS1WAY=OFF so the unlock path is real.
 * With PPS1WAY set (the XC8 default) silicon and MPLAB SIM honour one
 * sequence per reset: a second one, or an Unlock before the first Lock,
 * leaves PPSLOCKED unchanged (2026-10, 16F18877), so applications there
 * map after reset and Lock once.
 * @return 1 when every check passes, 0 otherwise.
 */
static int gate_pps_lock(void)
{
    int ok = 1;

    ok &= (EPIC_PPS_Lock() == EPIC_OK);
    ok &= (EPIC_PPS_IsLocked() == 1U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTB, 1U) == EPIC_BUSY);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x17U);
    ok &= (EPIC_PPS_MapOutput(PPS_PORTC, 6U, PPS_OUTPUT_LAT) == EPIC_BUSY);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 6U) == 0x10U);
    ok &= (EPIC_PPS_DeInit() == EPIC_BUSY);
    ok &= (EPIC_PPS_Unlock() == EPIC_OK);
    ok &= (EPIC_PPS_IsLocked() == 0U);
    ok &= (EPIC_PPS_MapInput(PPS_INPUT_EUSART_RX, PPS_PORTB, 1U) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x09U);
    ok &= (EPIC_PPS_DeInit() == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_RXPPS) == 0x17U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 6U) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 3U) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCP1PPS) == 0x12U);
    ok &= (EPIC_REG8(PIC_REG_CCP2PPS) == 0x11U);
    ok &= (EPIC_REG8(PIC_REG_CCP3PPS) == 0x0DU);
    ok &= (EPIC_REG8(PIC_REG_CCP4PPS) == 0x08U);
    ok &= (EPIC_REG8(PIC_REG_CCP5PPS) == 0x04U);
    ok &= (EPIC_REG8(PIC_REG_RC0PPS + 2U) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_RA0PPS + 1U) == 0x00U);
    ok &= (EPIC_PPS_Lock() == EPIC_OK);
    return ok;
}

/**
 * @brief Spin n instruction ticks (bounded, never waits on a condition).
 * @param n Tick count.
 */
static void tick_n(unsigned int n)
{
    unsigned int i;

    for (i = 0U; i < n; i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Spin until a PIR4 flag sets, bounded at 40000 ticks.
 * @param mask PIR4 flag bit to wait for.
 */
static void wait_pir4(uint8_t mask)
{
    unsigned int i;

    for (i = 0U; (i < 40000U) && ((EPIC_REG8(PIC_REG_PIR4) & mask) == 0U); i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Spin until a callback marker is set, bounded at 40000 ticks.
 * @param seen Marker the vectored callback sets.
 */
static void wait_seen(volatile uint8_t *seen)
{
    unsigned int i;

    for (i = 0U; (i < 40000U) && (*seen == 0U); i++)
    {
        epic_harness_tick();
    }
}

/**
 * @brief Load the Timer1 POR handle (one copy of the default image).
 * @param h Handle to reset.
 */
static void t1_reset(TIMER1_HandleTypeDef *h)
{
    *h = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
}

/**
 * @brief Load the Timer2 POR handle (one copy of the default image).
 * @param h Handle to reset.
 */
static void t2_reset(TIMER2_HandleTypeDef *h)
{
    *h = (TIMER2_HandleTypeDef)TIMER2_HANDLE_DEFAULT;
}

/**
 * @brief Timer1 section: FOSC/4 free-run at reload 0xFFF8 (overflow after
 *        8 counts) as a callback-free setup proof then a vector proof,
 *        then the RB5 gate and the toggle plus single-pulse image.
 * @details RB5 (T1GPPS at POR) is an output so its latch is the gate
 * level: held while low, counting once high, held again when it falls.
 * @return 1 when every check passes, 0 otherwise.
 */
static uint8_t gate_timer1(void)
{
    uint8_t ok = 1U;
    uint16_t held;
    TIMER1_HandleTypeDef t1;

    t1_reset(&t1);
    t1.ReloadValue = 0xFFF8U;
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x00U);
    ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x02U);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x03U);
    wait_pir4(PIC_PIR4_TMR1IF);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) != 0U);
    ok &= (EPIC_TIMER1_ReadCounter() < 0x0100U);
    EPIC_TIMER1_Stop();
    EPIC_TIMER1_DeInit();

    t1.OverflowCallback = on_t1;
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) != 0U);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    wait_seen(&s_t1_seen);
    ok &= (s_t1_seen == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR1IF) == 0U);
    ok &= (EPIC_TIMER1_ReadCounter() < 0x0100U);
    EPIC_TIMER1_Stop();
    EPIC_TIMER1_DeInit();
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR1IE) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T1CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T1CLK) == 0x00U);

    t1_reset(&t1);
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    t1.Gate.Enable = 1U;
    t1.Gate.ActiveHigh = 1U;
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0xC0U);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    tick_n(200U);
    ok &= (EPIC_TIMER1_ReadCounter() == 0U);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    tick_n(200U);
    ok &= (EPIC_TIMER1_ReadCounter() != 0U);
    ok &= (EPIC_TIMER1_GateValue() == 1U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    tick_n(50U);
    held = EPIC_TIMER1_ReadCounter();
    tick_n(200U);
    ok &= (EPIC_TIMER1_ReadCounter() == held);
    ok &= (EPIC_TIMER1_GateValue() == 0U);
    EPIC_TIMER1_Stop();
    EPIC_TIMER1_DeInit();

    t1.Gate.Toggle = 1U;
    t1.Gate.SinglePulse = 1U;
    t1.Gate.Source = TIMER1_GATE_TMR2_POST;
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_T1GCON) & (uint8_t)~PIC_T1GCON_GVAL) == 0xF0U);
    ok &= (EPIC_REG8(PIC_REG_T1GATE) == 0x03U);
    EPIC_TIMER1_DeInit();
    return ok;
}

/**
 * @brief Timer2 section: FOSC/4 free-run at PR2 7 as a callback-free
 *        setup proof then a vector proof, the full-field T2CON, T2HLT and
 *        T2RST image, and the hardware gate on RC3.
 * @details RC3 (T2INPPS at POR) is an output so its latch is the reset
 * signal: mode 0x01 holds TMR2 while it is low and runs once it is high.
 * @return 1 when every check passes, 0 otherwise.
 */
static uint8_t gate_timer2(void)
{
    uint8_t ok = 1U;
    TIMER2_HandleTypeDef t2;

    t2_reset(&t2);
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
    wait_pir4(PIC_PIR4_TMR2IF);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 7U);
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();

    t2.OverflowCallback = on_t2;
    ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) != 0U);
    ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
    wait_seen(&s_t2_seen);
    ok &= (s_t2_seen == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U);
    ok &= (EPIC_TIMER2_ReadCounter() <= 7U);
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();
    ok &= ((EPIC_REG8(PIC_REG_PIE4) & PIC_PIE4_TMR2IE) == 0U);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 0xFFU);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);

    t2_reset(&t2);
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
    EPIC_TIMER2_DeInit();

    t2_reset(&t2);
    t2.Mode = TIMER2_MODE_FREE_GATE_HIGH;
    t2.Period = 7U;
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_3, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x01U);
    ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
    tick_n(200U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U);
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
    wait_pir4(PIC_PIR4_TMR2IF);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) != 0U);
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    return ok;
}

/**
 * @brief CCP1 callback: records that the vectored handler ran.
 */
static void on_ccp(void)
{
    s_ccp_seen = 1U;
}

/**
 * @brief Count how often CCP1CON.OUT reads high over 1000 samples, one
 *        harness tick apart (a no-op on target, where time runs on its own).
 * @return High samples, 0..1000.
 */
static uint16_t ccp1_out_high(void)
{
    uint16_t i;
    uint16_t hi = 0U;

    for (i = 0U; i < 1000U; i++)
    {
        epic_harness_tick();
        if ((EPIC_REG8(PIC_REG_CCP1CON) & PIC_CCP1CON_OUT) != 0U)
        {
            hi++;
        }
    }
    return hi;
}

/**
 * @brief Count how often PWM6CON.OUT reads high over 1000 samples, one
 *        harness tick apart.
 * @return High samples, 0..1000.
 */
static uint16_t pwm6_out_high(void)
{
    uint16_t i;
    uint16_t hi = 0U;

    for (i = 0U; i < 1000U; i++)
    {
        epic_harness_tick();
        if ((EPIC_REG8(PIC_REG_PWM6CON) & PIC_PWM6CON_OUT) != 0U)
        {
            hi++;
        }
    }
    return hi;
}

/**
 * @brief Start Timer2 from FOSC/4 with PR2 63 (a 256 count PWM period).
 */
static void t2_pwm_start(void)
{
    TIMER2_HandleTypeDef t2 = TIMER2_HANDLE_DEFAULT;

    t2.Period = 63U;
    (void)EPIC_TIMER2_Init(&t2);
    (void)EPIC_TIMER2_Start(&t2);
}

/**
 * @brief CCP section: register images of the modes, timer couples, capture
 *        source and PWM alignment, a compare match against Timer1 (polled,
 *        then vectored on CCP5), and CCP1 PWM duty against Timer2.
 * @details CCPxCON.OUT is live read-only status and masks out of the
 * images. MPLAB SIM models the compare flag, the output bit, the vector and
 * the PWM, but never captures (it does not drive CCPxPPS from a latch) and
 * does not follow the timer-clearing and pulse compare modes, so those get
 * images only. The PWM proof compares high-sample counts rather than
 * absolute ratios: sampling a free-running output aliases against its
 * period.
 * @return 1 when every check passes, 0 otherwise.
 */
static uint8_t gate_ccp(void)
{
    uint8_t ok = 1U;
    uint16_t i;
    uint16_t lo;
    uint16_t hi;
    CCP_HandleTypeDef c = CCP_HANDLE_DEFAULT;
    TIMER1_HandleTypeDef t1;

    c.Mode = CCP_MODE_COMPARE_SET;
    c.CompareValue = 0x1234U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP1CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x88U);
    ok &= (EPIC_REG8(PIC_REG_CCPR1H) == 0x12U);
    ok &= (EPIC_REG8(PIC_REG_CCPR1L) == 0x34U);
    ok &= (EPIC_REG8(PIC_REG_CCP1CAP) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == PIC_CCPTMRS0_POR_VALUE);
    ok &= ((EPIC_REG8(PIC_REG_PIE6) & PIC_PIE6_CCP1IE) == 0U);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_COMPARE_TOGGLE_CLR_TMR) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP1CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x81U);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_COMPARE_PULSE_CLR_TMR) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP1CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x8BU);
    ok &= (EPIC_CCP_SetMode(CCP_INSTANCE_1, CCP_MODE_OFF) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP1CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x00U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);

    c = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    c.Instance = CCP_INSTANCE_2;
    c.Mode = CCP_MODE_CAPTURE_RISING;
    c.Timer = CCP_TIMER_3_4;
    c.CaptureSource = CCP_CAPTURE_LC4OUT;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP2CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x85U);
    ok &= (EPIC_REG8(PIC_REG_CCP2CAP) == 0x07U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == 0x59U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP2CAP) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS0) == PIC_CCPTMRS0_POR_VALUE);

    c = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    c.Instance = CCP_INSTANCE_5;
    c.Mode = CCP_MODE_PWM;
    c.Timer = CCP_TIMER_5_6;
    c.PWMDuty = 0x155U;
    c.PWMLeftAligned = 1U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_CCP5CON) & (uint8_t)~PIC_CCP1CON_OUT) == 0x9FU);
    ok &= (EPIC_REG8(PIC_REG_CCPR5H) == 0x55U);
    ok &= (EPIC_REG8(PIC_REG_CCPR5L) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x17U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_5, 0x3FFU) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCPR5H) == 0xFFU);
    ok &= (EPIC_REG8(PIC_REG_CCPR5L) == 0xC0U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_CCP5CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == PIC_CCPTMRS1_POR_VALUE);

    /* Compare match against Timer1 at 200 counts: flag, output and the
     * image with OUT live. */
    t1_reset(&t1);
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    c = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    c.Mode = CCP_MODE_COMPARE_SET;
    c.CompareValue = 200U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    for (i = 0U; (i < 40000U) && ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) == 0U); i++)
    {
        epic_harness_tick();
    }
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) != 0U);
    ok &= (EPIC_CCP_GetOutput(CCP_INSTANCE_1) == 1U);
    ok &= (EPIC_CCP_GetCapture(CCP_INSTANCE_1) == 200U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) == 0U);

    /* The same match vectored on CCP5, whose flag is PIR6 bit 4. Timer1
     * restarts so the 200 count match is ahead of it again. */
    EPIC_TIMER1_Stop();
    EPIC_TIMER1_DeInit();
    ok &= (EPIC_TIMER1_Init(&t1) == EPIC_OK);
    ok &= (EPIC_TIMER1_Start(&t1) == EPIC_OK);
    c.Instance = CCP_INSTANCE_5;
    c.EventCallback = on_ccp;
    s_ccp_seen = 0U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE6) & PIC_PIE6_CCP5IE) != 0U);
    EPIC_IRQ_Restore(1U);
    wait_seen(&s_ccp_seen);
    EPIC_IRQ_Restore(0U);
    ok &= (s_ccp_seen == 1U);
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP5IF) == 0U);
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_5) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PIE6) & PIC_PIE6_CCP5IE) == 0U);
    EPIC_TIMER1_Stop();
    EPIC_TIMER1_DeInit();

    /* PWM on CCP1 against Timer2 (256 count period): 0 counts never raise
     * the output, a quarter duty is high for fewer samples than three
     * quarters, and the left-aligned format lands the same duty. */
    c = (CCP_HandleTypeDef)CCP_HANDLE_DEFAULT;
    c.Mode = CCP_MODE_PWM;
    c.PWMDuty = 0U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    t2_pwm_start();
    tick_n(1000U);
    ok &= ((EPIC_REG8(PIC_REG_PIR6) & PIC_PIR6_CCP1IF) != 0U);
    ok &= (ccp1_out_high() == 0U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_1, 64U) == EPIC_OK);
    tick_n(1000U);
    lo = ccp1_out_high();
    ok &= (lo > 0U);
    ok &= (EPIC_CCP_SetPWMDuty(CCP_INSTANCE_1, 192U) == EPIC_OK);
    tick_n(1000U);
    hi = ccp1_out_high();
    ok &= (hi > (uint16_t)(lo + 250U));
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    c.PWMDuty = 64U;
    c.PWMLeftAligned = 1U;
    ok &= (EPIC_CCP_Init(&c) == EPIC_OK);
    tick_n(1000U);
    i = ccp1_out_high();
    ok &= (i > (uint16_t)(lo / 2U)) && (i < (uint16_t)(lo * 2U));
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();
    ok &= (EPIC_CCP_DeInit(CCP_INSTANCE_1) == EPIC_OK);
    return ok;
}

/**
 * @brief PWM6 and PWM7 section: register images, then PWM6 duty against
 *        Timer2 sampled through PWM6CON.OUT.
 * @return 1 when every check passes, 0 otherwise.
 */
static uint8_t gate_pwm(void)
{
    uint8_t ok = 1U;
    uint16_t lo;
    uint16_t hi;
    PWM_HandleTypeDef p = PWM_HANDLE_DEFAULT;

    p.Instance = PWM_INSTANCE_7;
    p.Timer = PWM_TIMER_6;
    p.Duty = 0x2A5U;
    p.ActiveLow = 1U;
    ok &= (EPIC_PWM_Init(&p) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PWM7CON) & (uint8_t)~PIC_PWM6CON_OUT) == 0x90U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCH) == 0xA9U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCL) == 0x40U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == 0x35U);
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_7) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PWM7CON) & (uint8_t)~PIC_PWM6CON_OUT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PWM7DCH) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_CCPTMRS1) == PIC_CCPTMRS1_POR_VALUE);

    p = (PWM_HandleTypeDef)PWM_HANDLE_DEFAULT;
    p.Duty = 64U;
    ok &= (EPIC_PWM_Init(&p) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PWM6CON) & (uint8_t)~PIC_PWM6CON_OUT) == 0x80U);
    ok &= (EPIC_REG8(PIC_REG_PWM6DCH) == 0x10U);
    ok &= (EPIC_REG8(PIC_REG_PWM6DCL) == 0x00U);
    t2_pwm_start();
    tick_n(1000U);
    lo = pwm6_out_high();
    ok &= (lo > 0U);
    ok &= (EPIC_PWM_SetDuty(PWM_INSTANCE_6, 192U) == EPIC_OK);
    tick_n(1000U);
    hi = pwm6_out_high();
    ok &= (hi > (uint16_t)(lo + 250U));
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();
    ok &= (EPIC_PWM_DeInit(PWM_INSTANCE_6) == EPIC_OK);
    ok &= ((EPIC_REG8(PIC_REG_PWM6CON) & (uint8_t)~PIC_PWM6CON_OUT) == 0x00U);
    return ok;
}

/**
 * @brief Timer2 edge-start one-shot section: mode 0x09 register images
 *        and the armed hold on RC3.
 * @details RC3 (T2INPPS at POR) is an output so its latch is the start
 * signal. MPLAB SIM never starts this mode (see SIM gaps), so the live
 * part ends at the armed hold: the counter stays zero with no flag.
 * DeInit restores the POR images.
 * @return 1 when every check passes, 0 otherwise.
 */
static uint8_t gate_timer2_edge_start(void)
{
    uint8_t ok = 1U;
    TIMER2_HandleTypeDef t2;

    t2_reset(&t2);
    t2.Mode = TIMER2_MODE_ONESHOT_RISING;
    t2.Period = 7U;
    EPIC_GPIO_Init(GPIOC, GPIO_PIN_3, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    ok &= (EPIC_TIMER2_Init(&t2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x09U);
    ok &= (EPIC_TIMER2_Start(&t2) == EPIC_OK);
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x80U);
    tick_n(200U);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    ok &= ((EPIC_REG8(PIC_REG_PIR4) & PIC_PIR4_TMR2IF) == 0U);
    EPIC_TIMER2_Stop();
    EPIC_TIMER2_DeInit();
    ok &= (EPIC_REG8(PIC_REG_T2CON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2HLT) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2CLKCON) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_T2RST) == 0x00U);
    ok &= (EPIC_TIMER2_ReadPeriod() == 0xFFU);
    ok &= (EPIC_TIMER2_ReadCounter() == 0U);
    EPIC_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    return ok;
}

/**
 * @brief GPIO, IOC, Timer0..Timer2, PPS, EUSART, MSSP1 and interrupt smoke
 *        probe for the mdb gate.
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

    /* Timer1 and Timer2: images, polled and vectored proofs, gates; then
     * the CCP and PWM modules that run off them. */
    ok &= gate_timer1();
    ok &= gate_timer2();
    ok &= gate_ccp();
    ok &= gate_pwm();
    ok &= gate_timer2_edge_start();

    /* EUSART, MSSP and PPS register images and vectored interrupts. The
     * PPS lock goes last: while locked, mapping and DeInit are refused. */
    ok &= gate_pps();
    ok &= gate_usart();
    ok &= gate_ssp();
    ok &= gate_eeprom();
    ok &= gate_nco();
    ok &= gate_wdt();
    ok &= gate_idle();
    ok &= gate_pps_lock();

    int rc = epic_harness_report(ok);
    pic16f188xx_harness_halt();
    return rc;
}
