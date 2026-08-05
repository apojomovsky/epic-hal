/**
 * @file    pic18_irq_dispatch.c
 * @brief   Fan-out from the PIC18 interrupt vectors to every peripheral
 *          IRQHandler. Shared by both builds.
 *
 * @details
 *   Both PIC18 interrupt vectors (0008h high, 0018h low, DS39632E §9.0)
 *   call this on target; the host harness registers it as the sim IRQ
 *   callback. Each peripheral IRQHandler checks its own flag and returns
 *   immediately if not pending, so calling them all in turn is correct.
 *   Prototypes are declared here as strong externs, not via the peripheral
 *   headers' `EPIC_WEAK` declaration, which would let the host linker drop
 *   the handler's object from the static library.
 */

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void TIMER3_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void COMP_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void RB_IRQHandler(void);
#if PIC18FXX5X_FAMILY_HAS_SPP
extern void SPP_IRQHandler(void);
#endif

void epic_dispatch_all_irqs(void)
{
    TIMER0_IRQHandler();
    TIMER1_IRQHandler();
    TIMER2_IRQHandler();
    TIMER3_IRQHandler();
    CCP1_IRQHandler();
    CCP2_IRQHandler();
    SSP_IRQHandler();
    USART_TX_IRQHandler();
    USART_RX_IRQHandler();
    COMP_IRQHandler();
    EEPROM_IRQHandler();
    ADC_IRQHandler();
    RB_IRQHandler();
#if PIC18FXX5X_FAMILY_HAS_SPP
    SPP_IRQHandler();
#endif
}
