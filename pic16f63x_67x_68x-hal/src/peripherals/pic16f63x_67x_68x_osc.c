/* Oscillator-fail handler implementation (DS40001262F §3.0). The
 * fail-safe clock monitor sets PIR2<OSFIF> when the external clock
 * fails; the shared dispatcher routes it here. */

#include "peripherals/pic16f63x_67x_68x_osc.h"
#include "core/pic16_irq.h"

/**
 * @brief Weak oscillator-fail ISR: clears OSFIF.
 */
void OSF_IRQHandler(void)
{
    /* Direct flag ops (class-F). OSFIF is PIR2 bit 7. */
    if (!(EPIC_REG8(PIC_REG_PIR2) & PIC_PIR2_OSFIF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR2), PIC_PIR2_OSFIF);
}
