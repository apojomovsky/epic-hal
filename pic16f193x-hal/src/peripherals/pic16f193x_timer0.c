/**
 * Timer0 driver, implementation (DS41364B §15.0). OPTION_REG (Register
 * 2-2) holds T0CS/T0SE/PSA/PS<2:0>; TMR0 is at 0x15. XC8 auto-banks
 * both (TMR0 in bank 0, OPTION_REG in bank 1) on this core, so every
 * access is a plain literal `PIC_REG_*` write. The handle is copied
 * into owned static storage in EPIC_TIMER0_Init (the caller's handle is
 * typically a stack local that is gone by the time the ISR reads it
 * back; storing a pointer would dangle, the same shape as the classic
 * family).
 */

#include "peripherals/pic16f193x_timer0.h"
#include "core/pic16f193x_irq.h"

/* Prescaler ratios, DS41364B Register 2-2: 000 -> 1:2 ... 111 -> 1:256. */
static const uint16_t ps_ratio[8] = { 2, 4, 8, 16, 32, 64, 128, 256 };

/* Per-handle storage. The 193X has only one Timer0, so a single slot is
 * sufficient; the header-inlined EPIC_TIMER0_Init stores the caller's
 * handle here field by field, and the ISR reads the callback through
 * this named global (ADR-024). */
TIMER0_HandleTypeDef g_t0_storage;

/**
 * @brief Stop Timer0, disable its interrupt and clear the counter.
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    EPIC_IRQ_DisableSrc(PIC16F193X_IRQ_TMR0);
    EPIC_IRQ_ClearFlag(PIC16F193X_IRQ_TMR0);
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Disable TMR0 counting (Timer0 halted).
 * @return EPIC_OK on success
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_OPTION), PIC_OPTION_T0CS);
    return EPIC_OK;
}

/**
 * @brief Read the current TMR0 counter value.
 * @return the 8-bit counter value
 */
uint8_t EPIC_TIMER0_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR0);
}

/**
 * @brief Write a new value to the counter (also clears the prescaler).
 * @param value counter value to write, 0..255
 */
void EPIC_TIMER0_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR0) = value;
}

/**
 * @brief Convert a prescaler enum to its integer ratio (2, 4, ..., 256).
 * @param p prescaler selection
 * @return the divider ratio, or 1 for an out-of-range value
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    if ((unsigned)p > 7U) return 1U;
    return ps_ratio[p];
}

/**
 * @brief Timer0 overflow ISR: clears TMR0IF and invokes the callback.
 */
void TIMER0_IRQHandler(void)
{
    /* Direct flag ops (class-F). TMR0IF is INTCON bit 2. */
    if (!(EPIC_REG8(PIC_REG_INTCON) & PIC_INTCON_TMR0IF)) return;
    EPIC_BIT_CLR(EPIC_REG8(PIC_REG_INTCON), PIC_INTCON_TMR0IF);
    if (g_t0_storage.OverflowCallback) {
        g_t0_storage.OverflowCallback();
    }
}
