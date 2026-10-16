/**
 * GPIO and IOC register-image probe for the mdb gate, ending in a live
 * interrupt smoke where the target allows it. Target-safe: only register
 * reads and bounded spins. Also runs on the host simulator through ctest.
 *
 * Expected register image: TRISB RB4 clears on output init; ANSELB RB4/5
 * clear on digital init; LATB follows WritePin then TogglePin; PORTA
 * full-port write lands every bit but RA0 (the harness result pin, held
 * low until PASS); WPUC follows SetPullups; IOCBP arms 0xFF; PIE0.IOCIE
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
 */
#include "pic16f188xx.h"
#include "peripherals/pic16f188xx_gpio.h"
#include "core/pic16f188xx_irq.h"
#include "core/epic_harness.h"

/** @brief Halt the target after the probe result is written (mdb gate). */
extern void pic16f188xx_harness_halt(void);

static volatile uint8_t s_ioc_seen = 0;
static volatile uint8_t s_ioc_iocbf = 0;

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
 * @brief GPIO, IOC and interrupt smoke probe for the mdb gate.
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

    int rc = epic_harness_report(ok);
    pic16f188xx_harness_halt();
    return rc;
}
