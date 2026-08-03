/**
 * @file    pic16_irq_dispatch.c
 * @brief   Fan-out from the single PIC16 interrupt vector to every
 *          peripheral IRQHandler. Shared by both builds.
 *
 * @details
 *   Single PIC16 vector (0x0004, DS39582B §14.11): the target's
 *   `__interrupt()` and the host's sim IRQ callback both call this one
 *   dispatcher. Each peripheral IRQHandler checks its own flag and
 *   returns immediately if not pending. Handlers are declared here
 *   with strong prototypes, not via their PIC8_WEAK headers, so the
 *   host linker is forced to pull every handler's object out of the
 *   static library instead of leaving an unreferenced weak symbol NULL.
 */

#include "core/pic16_irq.h"

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void COMP_IRQHandler(void);
extern void RB_IRQHandler(void);
#if PIC16F87XA_FAMILY_HAS_PSP
extern void PSP_IRQHandler(void);
#endif

void pic8_dispatch_all_irqs(void)
{
    TIMER0_IRQHandler();
    TIMER1_IRQHandler();
    TIMER2_IRQHandler();
    CCP1_IRQHandler();
    CCP2_IRQHandler();
    SSP_IRQHandler();
    USART_RX_IRQHandler();
    USART_TX_IRQHandler();
    ADC_IRQHandler();
    EEPROM_IRQHandler();
    COMP_IRQHandler();
    RB_IRQHandler();
#if PIC16F87XA_FAMILY_HAS_PSP
    PSP_IRQHandler();
#endif
}
