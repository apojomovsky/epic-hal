/* PIC16F5x Timer0 implementation (DS41213D §5.0). The OPTION control
 * register is written via EPIC_OPTION_WRITE (the `option` instruction
 * on target, the sim shadow on host). No interrupt path exists on this
 * core, so nothing touches INTCON-equivalents. */

#include "peripherals/pic16f5x_timer0.h"

/* Last OPTION byte written by the driver (see the header); extern so
 * the inlined Init in the header can store to it from any TU. */
uint8_t pic16f5x_t0_option_shadow = PIC_OPTION_POR_VALUE;

/**
 * @brief Disable Timer0 counting and reset TMR0.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_DeInit(void)
{
    pic16f5x_t0_option_shadow = PIC_OPTION_POR_VALUE;
    EPIC_OPTION_WRITE(pic16f5x_t0_option_shadow);
    EPIC_REG8(PIC_REG_TMR0) = 0x00U;
    return EPIC_OK;
}

/**
 * @brief Start Timer0 counting from internal Fosc/4 (clears T0CS).
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Start(const TIMER0_HandleTypeDef *h)
{
    (void)h;
    pic16f5x_t0_option_shadow &= (uint8_t)~PIC_OPTION_T0CS;
    EPIC_OPTION_WRITE(pic16f5x_t0_option_shadow);
    return EPIC_OK;
}

/**
 * @brief Stop Timer0 counting by clearing T0CS.
 * @return EPIC_OK.
 */
EPIC_StatusTypeDef EPIC_TIMER0_Stop(void)
{
    pic16f5x_t0_option_shadow &= (uint8_t)~PIC_OPTION_T0CS;
    EPIC_OPTION_WRITE(pic16f5x_t0_option_shadow);
    return EPIC_OK;
}

/**
 * @brief Read the current TMR0 value.
 * @return the 8-bit counter value.
 */
uint8_t EPIC_TIMER0_ReadCounter(void)
{
    return EPIC_REG8(PIC_REG_TMR0);
}

/**
 * @brief Write the TMR0 counter (also clears the prescaler).
 * @param value the 8-bit value to load.
 */
void EPIC_TIMER0_WriteCounter(uint8_t value)
{
    EPIC_REG8(PIC_REG_TMR0) = value;
}

/**
 * @brief Convert a prescaler enum to its integer ratio.
 * @param p the prescaler enum value.
 * @return the ratio (2..256), or 1 for an out-of-range value.
 */
uint16_t EPIC_TIMER0_PrescalerToRatio(TIMER0_PrescalerTypeDef p)
{
    static const uint16_t ps_ratio[8] = { 2, 4, 8, 16, 32, 64, 128, 256 };
    if ((unsigned)p > 7U) return 1U;
    return ps_ratio[p];
}
