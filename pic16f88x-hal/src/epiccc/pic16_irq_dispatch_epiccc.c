/* Fan-out for the 887 smoke under epic-cc: only Timer0 and RB are
 * needed for blinky. The full dispatch pulls in every peripheral handler
 * (ccp, usart, adc, etc.) which hit isel gaps (flash GEP, indirect calls)
 * that are filed separately. Keeping the smoke's dispatch minimal lets
 * the toolchain be proven on a second PIC14 part without waiting for
 * every peripheral's isel gap to be closed. */

#include "core/pic16_irq.h"

/** @brief TIMER0_IRQHandler (weak).
 */
extern void TIMER0_IRQHandler(void);
/** @brief RB_IRQHandler (weak).
 */
extern void RB_IRQHandler(void);

/**
 * @brief Dispatch all pending IRQs (smoke minimal).
 */
void epic_dispatch_all_irqs(void)
{
    uint8_t intcon = EPIC_REG8(PIC_REG_INTCON);
    if (intcon & PIC_INTCON_TMR0IF) TIMER0_IRQHandler();
    if (intcon & PIC_INTCON_RBIF) RB_IRQHandler();
    /* Other PIR1/PIR2 flags are not used by the smoke; if any are set,
     * just clear them so they do not re-trigger, without calling the
     * handlers that would pull in the rest of the HAL. */
    uint8_t pir1 = EPIC_REG8(PIC_REG_PIR1);
    if (pir1 & PIC_PIR1_TMR1IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR1IF);
    if (pir1 & PIC_PIR1_TMR2IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR2IF);
    if (pir1 & PIC_PIR1_CCP1IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_CCP1IF);
    if (pir1 & PIC_PIR1_SSPIF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_SSPIF);
    if (pir1 & PIC_PIR1_RCIF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_RCIF);
    if (pir1 & PIC_PIR1_TXIF) {
        uint8_t txie;
        EPIC_PIE1_READ_TXIE(txie);
        if (!(txie & PIC_PIE1_TXIE)) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TXIF);
    }
    if (pir1 & PIC_PIR1_ADIF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_ADIF);

    uint8_t pir2 = EPIC_REG8(PIC_REG_PIR2);
    if (pir2 & PIC_PIR2_CCP2IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_CCP2IF);
    if (pir2 & PIC_PIR2_EEIF) {
        uint8_t eeie = 0u;
        EPIC_PIE2_READ_EEIE(eeie);
        if (!(eeie & PIC_PIE2_EEIE)) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_EEIF);
    }
    if (pir2 & PIC_PIR2_C1IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C1IF);
    if (pir2 & PIC_PIR2_C2IF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_C2IF);
    if (pir2 & PIC_PIR2_ULPWUIF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_ULPWUIF);
    if (pir2 & PIC_PIR2_OSFIF) EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_OSFIF);
}
