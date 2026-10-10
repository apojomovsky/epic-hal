/**
 * GPIO register-contract probe for the mdb gate. Target-safe: uses only the
 * public EPIC_GPIO_* and EPIC_IRQ_* contract, then reads back the SFRs it
 * configured. Edges need an external source, so the IOC check covers the
 * enable path only; the edge path is in example_gpio.c on the host sim.
 */
#include "pic16f1508.h"
#include "peripherals/pic16f1508_gpio.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"

/** @brief Halt the target after the probe result is written (mdb gate). */
extern void pic16f1508_harness_halt(void);

/**
 * @brief Register-level GPIO probe for the mdb gate.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(0UL);

    /* RB4 output: TRIS cleared, analog cleared, latch follows WritePin and TogglePin. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_4) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_4) == 0U);
    EPIC_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) != 0U);
    EPIC_GPIO_TogglePin(GPIOB, GPIO_PIN_4);
    ok &= ((EPIC_REG8(PIC_REG_LATB) & GPIO_PIN_4) == 0U);

    /* RB5 input: TRIS set, analog cleared. RB0..RB3 are not implemented on this part. */
    EPIC_GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_INPUT);
    ok &= ((EPIC_REG8(PIC_REG_TRISB) & GPIO_PIN_5) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_ANSELB) & GPIO_PIN_5) == 0U);

    /* LATA3 is unimplemented (RA3 is input-only): target drops the bit, host sim keeps it.
     * Compare only the implemented latch bits, which both builds must match. */
    EPIC_GPIO_Init(GPIOA, GPIO_PIN_All, GPIO_MODE_OUTPUT);
    EPIC_GPIO_WritePort(GPIOA, 0xFFU);
    ok &= ((EPIC_REG8(PIC_REG_LATA) & PIC16F1508_FAMILY_LATA_MASK) == PIC16F1508_FAMILY_LATA_MASK);

    /* Clear all pull-ups first: target reset is WPUA=0x3F and WPUB=0xF0, host sim is 0,
     * so the expectations below must not depend on the reset state. */
    EPIC_GPIO_SetPullups(GPIOA, GPIO_PIN_All, GPIO_PIN_RESET);
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_All, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) != 0U);

    /* RB4 and RB5 pull-ups: WPUB bits set, nWPUEN active low (cleared when enabled). */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_SET);
    ok &= ((EPIC_REG8(PIC_REG_WPUB) & (GPIO_PIN_4 | GPIO_PIN_5)) == (GPIO_PIN_4 | GPIO_PIN_5));
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) == 0U);
    /* RB5 cleared while RB4 is still pulled up: nWPUEN stays enabled. */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_WPUB) & GPIO_PIN_5) == 0U);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) == 0U);
    /* No pull-ups left: nWPUEN disabled again. */
    EPIC_GPIO_SetPullups(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    ok &= ((EPIC_REG8(PIC_REG_OPTION) & PIC_OPTION_nWPUEN) != 0U);

    /* IOC: only RB4..RB7 are implemented; the enable path sets IOCIE. */
    EPIC_GPIO_EnableChangeDetect(GPIO_PIN_All, 0U);
    ok &= (EPIC_REG8(PIC_REG_IOCBP) == PIC16F1508_FAMILY_IOCB_MASK);
    EPIC_IRQ_Enable(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) != 0U);
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) == 0U);

    int rc = epic_harness_report(ok);
    pic16f1508_harness_halt();
    return rc;
}
