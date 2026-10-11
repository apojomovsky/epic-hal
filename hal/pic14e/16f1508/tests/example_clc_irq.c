/**
 * CLC interrupt routing host-sim probe. Host-only: drives PIR3, PIE3 and INTCON
 * through the flat simulator SFR array via EPIC_REG8, so it is not in the XC8
 * build. Covers enable/disable of CLC1..CLC4 through the shared IRQ contract,
 * flag read/clear independent of enable, dispatch gated on flag and enable,
 * and the PEIE fail-safe that acknowledges unowned PIR1/PIR2 sources.
 */
#include "pic16f1508.h"
#include "core/pic16f1508_irq.h"
#include "core/epic_harness.h"
#include "peripherals/pic16f1508_gpio.h"

/**
 * @brief IOC user callback that disables CLC2 while the dispatcher is running.
 * @param iocbf Captured IOCBF mask (unused).
 * @param portb PORTB byte (unused).
 */
static void disable_clc2_from_ioc(uint8_t iocbf, uint8_t portb)
{
    (void)iocbf;
    (void)portb;
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC2);
}

/**
 * @brief Run the CLC interrupt routing checks on the host simulator.
 * @return Harness result: 0 on PASS, 1 on FAIL.
 */
int main(void)
{
    int ok = 1;

    epic_harness_init(10000UL);

    EPIC_REG8(PIC_REG_INTCON) = 0U;
    EPIC_REG8(PIC_REG_PIE3) = 0U;
    EPIC_REG8(PIC_REG_PIR3) = 0U;

    /* Enabling a CLC source sets its PIE3 bit and PEIE, and leaves IOCIE alone. */
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC2);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x02U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) == 0U);

    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC1);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x03U);

    /* Disabling clears only that source's PIE3 bit; PEIE stays set. */
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_CLC2);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x01U);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_PEIE) != 0U);

    /* IOC still maps to INTCON.IOCIE and does not touch PIE3. */
    EPIC_IRQ_Enable(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) != 0U);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x01U);
    EPIC_IRQ_DisableSrc(PIC16F1508_IRQ_IOC);
    ok &= ((EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_IOCIE) == 0U);

    /* An out-of-range source changes nothing. */
    EPIC_IRQ_Enable((PIC16F1508_IRQn)7);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x01U);
    ok &= (EPIC_IRQ_GetFlag((PIC16F1508_IRQn)7) == 0U);

    /* GetFlag reads PIR3 regardless of the enable bit. */
    EPIC_REG8(PIC_REG_PIE3) = 0U;
    EPIC_REG8(PIC_REG_PIR3) = 0x04U;
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC3) == 1U);
    ok &= (EPIC_IRQ_GetFlag(PIC16F1508_IRQ_CLC1) == 0U);

    /* ClearFlag clears only the named CLC flag. */
    EPIC_REG8(PIC_REG_PIR3) = 0x0FU;
    EPIC_IRQ_ClearFlag(PIC16F1508_IRQ_CLC3);
    ok &= (EPIC_REG8(PIC_REG_PIR3) == 0x0BU);

    /* Dispatch acknowledges an enabled, pending CLC and leaves a disabled one pending. */
    EPIC_REG8(PIC_REG_PIR3) = 0x03U;
    EPIC_REG8(PIC_REG_PIE3) = 0x01U;
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIR3) == 0x02U);

    /* A second dispatch with the same state does not re-acknowledge anything. */
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIR3) == 0x02U);

    /* A disable from the IOC callback must stop the CLC flag being acknowledged. */
    EPIC_REG8(PIC_REG_INTCON) = PIC_INTCON_IOCIF | PIC_INTCON_IOCIE | PIC_INTCON_PEIE;
    EPIC_REG8(PIC_REG_PIR3) = 0x02U;
    EPIC_REG8(PIC_REG_PIE3) = 0x02U;
    EPIC_GPIO_RegisterChangeCallback(disable_clc2_from_ioc);
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIR3) == 0x02U);
    EPIC_GPIO_RegisterChangeCallback(0);
    EPIC_REG8(PIC_REG_INTCON) = 0U;

    /* CLC enable sets PEIE, which exposes PIR1 and PIR2. The dispatcher must acknowledge them. */
    EPIC_REG8(PIC_REG_PIR1) = 0x01U;
    EPIC_REG8(PIC_REG_PIE1) = 0x01U;
    EPIC_REG8(PIC_REG_PIR2) = 0x80U;
    EPIC_REG8(PIC_REG_PIE2) = 0x80U;
    EPIC_REG8(PIC_REG_PIR3) = 0x01U;
    EPIC_REG8(PIC_REG_PIE3) = 0x00U;
    EPIC_IRQ_Enable(PIC16F1508_IRQ_CLC1);
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIR1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIE1) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIR2) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIE2) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIR3) == 0x00U);
    ok &= (EPIC_REG8(PIC_REG_PIE3) == 0x01U);
    EPIC_REG8(PIC_REG_INTCON) = 0U;
    EPIC_REG8(PIC_REG_PIE3) = 0U;

    /* A flagged PIR1 source whose enable is clear stays pending for polling. */
    EPIC_REG8(PIC_REG_INTCON) = PIC_INTCON_PEIE;
    EPIC_REG8(PIC_REG_PIR1) = 0x02U;
    EPIC_REG8(PIC_REG_PIE1) = 0x00U;
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIR1) == 0x02U);
    EPIC_REG8(PIC_REG_PIR1) = 0x00U;
    EPIC_REG8(PIC_REG_INTCON) = 0U;

    /* Without PEIE, PIR1 sources cannot reach the vector and are left as they are. */
    EPIC_REG8(PIC_REG_PIR1) = 0x01U;
    EPIC_REG8(PIC_REG_PIE1) = 0x01U;
    epic_dispatch_all_irqs();
    ok &= (EPIC_REG8(PIC_REG_PIR1) == 0x01U);
    ok &= (EPIC_REG8(PIC_REG_PIE1) == 0x01U);
    EPIC_REG8(PIC_REG_PIR1) = 0x00U;
    EPIC_REG8(PIC_REG_PIE1) = 0x00U;

    epic_harness_log("clc/interrupt routing: %s\n", ok ? "PASS" : "FAIL");
    return epic_harness_report(ok);
}
